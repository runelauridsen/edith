////////////////////////////////////////////////////////////////
// rune: Font backend

static yo_face yo_face_make(yo_font font, u16 fontsize) {
    yo_face ret = {
        .font = font,
        .fontsize = fontsize,
    };
    return ret;
}

static f32 yo_font_backend_get_advance(u32 codepoint, yo_face face) {
    yo_state *state = yo_state_get();
    yo_font_backend *backend = &state->font_backend;
    f32 ret = backend->get_advance(backend->userdata, face, codepoint);
    return ret;
}

static f32 yo_font_backend_get_lineheight(yo_face face) {
    yo_state *state = yo_state_get();
    yo_font_backend *backend = &state->font_backend;
    f32 ret = backend->get_lineheight(backend->userdata, face);
    return ret;
}

static yo_face yo_face_get(void) {
    yo_state *state = yo_state_get();
    return state->curr_face;
}

static void yo_face_set(yo_face face) {
    YO_PROFILE_BEGIN(yo_face_set);

    yo_state *state = yo_state_get();
    if (state->curr_face.u32 != face.u32) {
        state->curr_face = face;

        // rune: Cache lineheight.
        state->curr_lineheight = yo_font_backend_get_lineheight(face);

        // rune: Cache advance of ascii codepoints.
        for_n (u32, i, countof(state->curr_adv_ascii)) {
            state->curr_adv_ascii[i] = yo_font_backend_get_advance(i, face);
        }
    }

    YO_PROFILE_END(yo_face_set);
}

////////////////////////////////////////////////////////////////
// rune: Event list

static yo_event *yo_event_list_push(yo_event_list *list, arena *arena) {
    yo_event *node = arena_push_struct(arena, yo_event);
    dlist_push(list, node);
    list->count += 1;
    return node;
}

static void yo_event_list_remove(yo_event_list *list, yo_event *e) {
    dlist_remove(list, e);
    list->count -= 1;
}

static yo_event_list yo_events(void) {
    yo_state *state = yo_state_get();
    return state->events;
}

static bool yo_event_is_key_press(yo_event *e, yo_key key, yo_modifiers mods) {
    bool ret = e->kind == YO_EVENT_KIND_KEY_PRESS && e->key == key && e->mods == mods;
    return ret;
}

static bool yo_event_is_key_release(yo_event *e, yo_key key, yo_modifiers mods) {
    bool ret = e->kind == YO_EVENT_KIND_KEY_RELEASE && e->key == key && e->mods == mods;
    return ret;
}

static bool yo_event_eat(yo_event *e) {
    if (e->eaten == false) {
        yo_state *state = yo_state_get();
        yo_event_list_remove(&state->events, e);
        e->eaten = true;
        return true;
    } else {
        assert(false && "Event has already been eaten.");
        return false;
    }
}

static bool yo_event_eat_key_press(yo_key key, yo_modifiers mods) {
    for_list (yo_event, e, yo_events()) {
        if (yo_event_is_key_press(e, key, mods)) {
            yo_event_eat(e);
            return true;
        }
    }
    return false;
}

static bool yo_event_eat_key_release(yo_key key, yo_modifiers mods) {
    for_list (yo_event, e, yo_events()) {
        if (yo_event_is_key_press(e, key, mods)) {
            yo_event_eat(e);
            return true;
        }
    }
    return false;
}

////////////////////////////////////////////////////////////////
// rune: Mouse

static vec2 yo_mouse_pos(void) {
    yo_state *state = yo_state_get();
    vec2 mouse_pos_local = vec2_sub(state->mouse_pos, state->node_stack.top->rel_offset);
    return mouse_pos_local;
}

static vec2 yo_mouse_drag_delta(void) {
    yo_state *state = yo_state_get();
    vec2 delta = vec2_sub(state->mouse_pos, state->mouse_drag_start);
    return delta;
}

static vec2 yo_mouse_drag_start(void) {
    yo_state *state = yo_state_get();
    vec2 start = state->mouse_drag_start;
    return start;
}

////////////////////////////////////////////////////////////////
// rune: Context menus

static void yo_ctx_menu_open(yo_id id) {
    yo_state *state = yo_state_get();
    state->ctx_menu_id = id;
}

static void yo_ctx_menu_close(void) {
    yo_state *state = yo_state_get();
    state->ctx_menu_id = 0;
}

static bool yo_ctx_menu_begin(yo_id id) {
    yo_state *state = yo_state_get();
    bool is_open = state->ctx_menu_id == id;
    if (is_open) {
        state->ctx_menu_touched_this_frame = true;
        yo_node_begin(id);
    }
    return is_open;
}

static void yo_ctx_menu_end(void) {
    yo_state *state = yo_state_get();
    yo_node_end();
}

////////////////////////////////////////////////////////////////
// rune: Context creation and selection

static yo_state *yo_state_create(void) {
    arena *perm_arena  = edith_arena_create_default(str("yo perm arena"));
    arena *frame_arena = edith_arena_create_default(str("yo frame arena"));

    yo_state *state = arena_push_struct(perm_arena, yo_state);
    state->perm_arena = perm_arena;
    state->frame_arena = frame_arena;

    state->hash.slots_count = 1024; // TODO(rune): User configurable
    state->hash.slots = arena_push_array(perm_arena, yo_node *, state->hash.slots_count);

    state->root = arena_push_struct(perm_arena, yo_node);
    return state;
}

static void yo_state_destroy(yo_state *state) {
    arena_destroy(state->perm_arena);
    arena_destroy(state->frame_arena);
}

static void yo_state_select(yo_state *state) {
    yo_g_state = state;
}

static yo_state *yo_state_get(void) {
    return yo_g_state;
}

////////////////////////////////////////////////////////////////
// rune: Frame boundaries

static void yo_frame_begin(yo_input *input) {
    yo_state *state = yo_state_get();

    state->frame_counter += 1;

    arena_reset(state->frame_arena);
    yo_stack_init(yo_node_ptr, &state->node_stack, state->frame_arena, 0);
    yo_stack_init(yo_id, &state->id_stack, state->frame_arena, 0);

    state->root = yo_node_begin(yo_id_root());
    state->root->tag = "root";

    state->client_dim = rect_dim(input->client_rect);
    state->mouse_pos  = input->mouse_pos;
    state->events     = input->events;
    state->delta_time = input->delta_time;

    state->hot_id_found_this_frame = false;
}

static void yo_print_node_tree(yo_node *node, i64 level) {
    for_n (i64, i, level) {
        print("-> ");
    }

    if (node->tag) {
        print(node->tag);
    } else {
        print(node->id);
    }

    print("\n");

    for_list (yo_node, child, node->children) {
        yo_print_node_tree(child, level + 1);
    }
}

static void yo_check_node_tree(yo_node *node) {
    for_list (yo_node, child, node->children) { assert(child->parent == node); }
    for_list (yo_node, child, node->children) { yo_check_node_tree(child); }
}

static void yo_frame_end_animate(yo_node *node) {
    yo_state *state = yo_state_get();

    f32 target_hot_t    = node->id == state->hot_id ? 1.0f : 0.0f;
    f32 target_active_t = node->id == state->active_id ? 1.0f : 0.0f;

    yo_anim_f32(&node->hot_t, target_hot_t, 5.0f);
    yo_anim_f32(&node->active_t, target_active_t, 5.0f);

    for_list (yo_node, child, node->children) {
        yo_frame_end_animate(child);
    }
}

static void yo_frame_end(void) {
    yo_state *state = yo_state_get();
    yo_node_end();

    assert(state->node_stack.count == 1);

    // rune: Hot id managment
    {
        if (state->hot_id_found_this_frame == false) {
            state->hot_id = 0;
        }
    }

    // rune: Context menu id managment
    {
        if (state->ctx_menu_touched_this_frame == false) {
            yo_ctx_menu_close();
        }

        for_list (yo_event, e, yo_events()) {
            if (e->kind == YO_EVENT_KIND_MOUSE_PRESS) {
                yo_ctx_menu_close();
                break;
            }

            if (e->kind == YO_EVENT_KIND_KEY_PRESS && e->key == YO_KEY_ESCAPE) {
                yo_ctx_menu_close();
                break;
            }
        }
    }

    // rune: Calculate absolute positions from relative positions
    {
        for (yo_node *node = state->root; node; node = yo_node_iter_pre_order(node)) {
            if (node->parent) {
                node->abs_offset = vec2_add(node->rel_offset, node->parent->abs_offset);
                node->abs_rect = rect_offset(node->rel_rect, node->abs_offset);
            }
        }
    }
}

static arena *yo_frame_arena(void) {
    yo_state *state = yo_state_get();
    return state->frame_arena;
}

////////////////////////////////////////////////////////////////
// rune: Node allocation

static yo_node *yo_node_from_id(yo_id id) {
    yo_state *state = yo_state_get();
    yo_node *node = null;
    if (id) {
        i64 slot_idx = id % state->hash.slots_count;
        for (yo_node *it = state->hash.slots[slot_idx]; it; it = it->next) {
            if (it->id == id) {
                node = it;
                break;
            }
        }
    }
    return node;
}

static yo_node *yo_node_begin(yo_id id) {
    yo_state *state = yo_state_get();
    yo_node *node = 0;

    // rune: Lookup in hash table
    node = yo_node_from_id(id);

    // rune: Allocate a new node, if we didn't find a cached node in the hash table
    if (node == 0) {
        node = yo_node_alloc();
        node->first_frame_touched = state->frame_counter;
        if (id) {
            i64 slot_idx = id % state->hash.slots_count;
            DLSTACK_PUSH(state->hash.slots[slot_idx], hash_next, hash_prev, node);
        }
    }

    if (node->last_frame_touched == state->frame_counter) {
        assert(false);
    }

    node->last_frame_touched = state->frame_counter;

    // rune: Clear per frame data
    node->id = id;
    zero_struct(&node->children.first);
    zero_struct(&node->children.last);
    zero_struct(&node->parent);
    zero_struct(&node->next);
    zero_struct(&node->rel_rect);
    zero_struct(&node->rel_offset);
    zero_struct(&node->ops);
    zero_struct(&node->tag);

    if (state->node_stack.count > 1) {

        yo_node *parent = yo_stack_get(yo_node_ptr, &state->node_stack);
        node->parent = parent;

        slist_push(&parent->children, node);
    }

    yo_stack_push(yo_node_ptr, &state->node_stack);
    yo_stack_set(yo_node_ptr, &state->node_stack, node);

    if (id) {
        yo_stack_push(yo_id, &state->id_stack);
        yo_stack_set(yo_id, &state->id_stack, node->id);
    }

    return node;
}

static yo_node *yo_node_end(void) {
    yo_state *state = yo_state_get();
    yo_node *node = yo_stack_get(yo_node_ptr, &state->node_stack);
    yo_stack_pop(yo_node_ptr, &state->node_stack);

    if (node->id) {
        yo_stack_pop(yo_id, &state->id_stack);
    }

    return node;
}

static yo_node *yo_node_get(void) {
    yo_state *state = yo_state_get();
    yo_node *curr = yo_stack_get(yo_node_ptr, &state->node_stack);
    return curr;
}

static yo_node *yo_node_alloc(void) {
    yo_state *state = yo_state_get();
    yo_node *node = state->node_free_list;
    if (node) {
        slstack_pop(&state->node_free_list);
        zero_struct(node);
    } else {
        node = arena_push_struct(state->perm_arena, yo_node);
    }

    return node;
}

static void yo_node_free(yo_node *node) {
    yo_state *state = yo_state_get();

    yo_id id = node->id;
    if (id) {
        i64 slot_idx = id % state->hash.slots_count;
        DLSTACK_REMOVE(state->hash.slots[slot_idx], hash_next, hash_prev, node);
    }

    node->next      = 0;
    node->hash_next = 0;
    node->hash_prev = 0;

    slstack_push(&state->node_free_list, node);
}

static yo_node_list yo_children(void) {
    yo_node *node = yo_node_get();
    return node->children;
}

////////////////////////////////////////////////////////////////
// rune: Tree traversel

static yo_node *yo_node_iter_pre_order(yo_node *curr) {
    yo_node *ret = null;

    if (curr->children.first) {
        ret = curr->children.first;
    } else if (curr->next) {
        ret = curr->next;
    } else {
        ret = curr->parent->next;
    }

    return ret;
}

////////////////////////////////////////////////////////////////
// rune: Ops list construction

static void *yo_op_push(yo_op_kind type, u64 size) {
    yo_state *state = yo_state_get();
    yo_op *op = arena_push_size(state->frame_arena, size, 0);
    op->type = type;
    slist_push(&state->node_stack.top->ops, op);
    return op;
}

static yo_op_glyph *    yo_op_push_glyph(void) { return yo_op_push(YO_OP_KIND_GLYPH, sizeof(yo_op_glyph)); }
static yo_op_text *     yo_op_push_text(void) { return yo_op_push(YO_OP_KIND_TEXT, sizeof(yo_op_text)); }
static yo_op_aabb *     yo_op_push_aabb(void) { return yo_op_push(YO_OP_KIND_AABB, sizeof(yo_op_aabb)); }
static yo_op_aabb_ex *  yo_op_push_aabb_ex(void) { return yo_op_push(YO_OP_KIND_AABB_EX, sizeof(yo_op_aabb_ex)); }
static yo_op_quad *     yo_op_push_quad(void) { return yo_op_push(YO_OP_KIND_QUAD, sizeof(yo_op_quad)); }

////////////////////////////////////////////////////////////////
// rune: Hashing

static u64 yo_murmur_mix(u64 a) {
    a ^= a >> 33;
    a *= 0xff51afd7ed558ccd;
    a ^= a >> 33;
    a *= 0xc4ceb9fe1a85ec53;
    a ^= a >> 33;
    return a;
}

static yo_id yo_hash_i8(i8 a) { return yo_murmur_mix(a); }
static yo_id yo_hash_i16(i16 a) { return yo_murmur_mix(a); }
static yo_id yo_hash_i32(i32 a) { return yo_murmur_mix(a); }
static yo_id yo_hash_i64(i64 a) { return yo_murmur_mix(a); }
static yo_id yo_hash_u8(u8 a) { return yo_murmur_mix(a); }
static yo_id yo_hash_u16(u16 a) { return yo_murmur_mix(a); }
static yo_id yo_hash_u32(u32 a) { return yo_murmur_mix(a); }
static yo_id yo_hash_u64(u64 a) { return yo_murmur_mix(a); }
static yo_id yo_hash_f32(f32 a) { return yo_murmur_mix(u32_from_f32(a)); }
static yo_id yo_hash_f64(f64 a) { return yo_murmur_mix(u64_from_f64(a)); }
static yo_id yo_hash_bool(bool a) { return yo_murmur_mix(a); }
static yo_id yo_hash_ptr(void *a) { return yo_murmur_mix(u64(a)); }

// Reference: https://stackoverflow.com/a/57960443
static yo_id yo_hash_str(str s) {
    YO_PROFILE_BEGIN(yo_hash_str);

    // TODO(rune): Profile different hash functions.
    u64 h = 525201411107845655;
    for_n (i64, i, s.len) {
        h ^= s.v[i];
        h *= 0x5bd1e9955bd1e995;
        h ^= h >> 47;
    }

    YO_PROFILE_END(yo_hash_str);
    return h;
}

// Reference: https://stackoverflow.com/a/57960443
static yo_id yo_hash_cstr(char *s) {
    YO_PROFILE_BEGIN(yo_hash_cstr);

    // TODO(rune): Profile different hash functions.
    u64 h = 525201411107845655;
    while (*s) {
        h ^= *s++;
        h *= 0x5bd1e9955bd1e995;
        h ^= h >> 47;
    }

    YO_PROFILE_END(yo_hash_cstr);
    return h;
}

////////////////////////////////////////////////////////////////
// rune: Ids

static yo_id yo_id_root(void) { return U64_MAX; }

static yo_id yo_id_combine(yo_id a, yo_id b) {
    yo_id ret = a ^ (b + 0x9e3779b9 + (a << 6) + (a >> 2));
    return ret;
}

static yo_id yo_id_combine_with_parent(yo_id child_id) {
    yo_state *state = yo_state_get();
    yo_id parent_id = yo_stack_get(yo_id, &state->id_stack);
    yo_id ret = yo_id_combine(parent_id, child_id);
    return ret;
}

static yo_id yo_active_id(void) {
    yo_state *state = yo_state_get();
    return state->active_id;
}

static yo_id yo_hot_id(void) {
    yo_state *state = yo_state_get();
    return state->hot_id;
}

////////////////////////////////////////////////////////////////
// rune: Drawing

static void yo_draw_rect(rect dst, u32 color) {
    yo_op_aabb *op = yo_op_push_aabb();
    op->color = color;
    op->rect = dst;
}

static void yo_draw_gradient(rect dst, u32 color0, u32 color1, u32 color2, u32 color3) {
    yo_op_aabb_ex *op = yo_op_push_aabb_ex();
    op->dst_rect         = dst;
    op->color[0]         = color0;
    op->color[1]         = color1;
    op->color[2]         = color2;
    op->color[3]         = color3;
}

static void yo_draw_rounded_rect(rect dst, u32 color, f32 roundness) {
    yo_op_aabb_ex *op = yo_op_push_aabb_ex();
    op->dst_rect         = dst;
    op->color[0]         = color;
    op->color[1]         = color;
    op->color[2]         = color;
    op->color[3]         = color;
    op->corner_radius[0] = roundness;
    op->corner_radius[1] = roundness;
    op->corner_radius[2] = roundness;
    op->corner_radius[3] = roundness;
}

static void yo_draw_soft_rect(rect dst, u32 color, f32 softness) {
    yo_op_aabb_ex *op = yo_op_push_aabb_ex();
    op->dst_rect         = dst;
    op->color[0]         = color;
    op->color[1]         = color;
    op->color[2]         = color;
    op->color[3]         = color;
    op->corner_radius[0] = softness;
    op->corner_radius[1] = softness;
    op->corner_radius[2] = softness;
    op->corner_radius[3] = softness;
    op->softness         = softness;
}

static void yo_draw_textured(rect dst, u32 color, rect uv) {
    yo_op_aabb_ex *op = yo_op_push_aabb_ex();
    op->dst_rect         = dst;
    op->tex_rect         = uv;
    op->tex_weight       = 1.0f;
    op->color[0]         = color;
    op->color[1]         = color;
    op->color[2]         = color;
    op->color[3]         = color;
}

static void yo_draw_rounded_shadow(rect dst, f32 shadow_radius, f32 roundness, f32 shadow_softness) {
    f32 softness = shadow_radius + roundness + shadow_softness;

    dst.x0 -= shadow_radius;
    dst.y0 -= shadow_radius;
    dst.x1 += shadow_radius;
    dst.y1 += shadow_radius;

    yo_op_aabb_ex *op = yo_op_push_aabb_ex();
    op->dst_rect         = dst;
    op->color[0]         = rgba(0, 0, 0, 100);
    op->color[1]         = rgba(0, 0, 0, 100);
    op->color[2]         = rgba(0, 0, 0, 100);
    op->color[3]         = rgba(0, 0, 0, 100);
    op->corner_radius[0] = softness;
    op->corner_radius[1] = softness;
    op->corner_radius[2] = softness;
    op->corner_radius[3] = softness;
    op->softness         = softness;
}

static void yo_draw_shadow(rect dst, f32 shadow_radius, f32 shadow_softness) {
    yo_draw_rounded_shadow(dst, shadow_radius, 0, shadow_softness);
}

static void yo_draw_rect_with_shadow(rect dst, u32 color, f32 shadow_radius, f32 shadow_softness) {
    yo_draw_shadow(dst, shadow_radius, shadow_softness);
    yo_draw_rect(dst, color);
}

static void yo_draw_rounded_rect_with_shadow(rect dst, u32 color, f32 roundness, f32 shadow_radius, f32 shadow_softness) {
    yo_draw_rounded_shadow(dst, shadow_radius, roundness, shadow_softness);
    yo_draw_rounded_rect(dst, color, roundness);
}

static f32 yo_adv_from_glyph_(u32 codepoint, yo_face face) {
    f32 ret = yo_font_backend_get_advance(codepoint, face);
    return ret;
}

static f32 yo_adv_from_glyph(u32 codepoint) {
    return yo_adv_from_glyph_(codepoint, yo_face_get());
}

static f32 yo_adv_from_text(str s) {
    YO_PROFILE_BEGIN(yo_adv_from_text);
    yo_state *state = yo_state_get();

    f32 adv_sum = 0.0f;
    i64 i = 0;
    while (i < s.len) {
        if (s.v[i] < 128) {
            f32 adv = state->curr_adv_ascii[s.v[i]];
            adv_sum += adv;
            i += 1;
        } else {
            unicode_codepoint decoded = decode_single_utf8_codepoint(str_make(s.v + i, s.len - i));
            if (decoded.len) {
                f32 advance = yo_adv_from_glyph(decoded.codepoint);
                adv_sum += advance;
                i += decoded.len;
            } else {
                // NOTE(rune): String is not well-formed utf8 -> just move forward and ignore.
                adv_sum = state->curr_adv_ascii['0'];
                i += 1;
            }
        }
    }

    YO_PROFILE_END(yo_adv_from_text);
    return adv_sum;
}

static vec2 yo_dim_from_glyph(u32 codepoint) {
    yo_state *state = yo_state_get();
    f32 adv = yo_adv_from_glyph(codepoint);
    vec2 ret = vec2(adv, state->curr_lineheight);
    return ret;
}

static vec2 yo_dim_from_text(str s) {
    yo_state *state = yo_state_get();
    f32 adv = yo_adv_from_text(s);
    vec2 ret = vec2(adv, state->curr_lineheight);
    return ret;
}

static void yo_draw_glyph(vec2 dst, u32 codepoint, u32 color) {
    yo_op_glyph *op = yo_op_push_glyph();
    op->codepoint = codepoint;
    op->color     = color;
    op->p         = dst;
    op->face      = yo_face_get();
}

static void yo_draw_text(vec2 dst, str s, u32 color) {
    yo_op_text *op = yo_op_push_text();
    op->color     = color;
    op->p         = dst;
    op->s         = s;
    op->face      = yo_face_get();
}

static void yo_draw_caret(vec2 dst, f32 thickness, u32 color) {
    yo_state *state = yo_state_get();
    vec2 dim = vec2(thickness, state->curr_lineheight);
    rect rect = rect_make_dim(dst, dim);

    yo_op_aabb *op = yo_op_push_aabb();
    op->color = color;
    op->rect = rect;
}

////////////////////////////////////////////////////////////////
// rune: Rect cut

static rect yo_rect_cut_left(rect *r, f32 a) {
    f32 x0 = r->x0;
    r->x0 = min(r->x1, r->x0 + a);
    rect ret = { x0, r->y0, r->x0, r->y1 };
    return ret;
}

static rect yo_rect_cut_right(rect *r, f32 a) {
    f32 x1 = r->x1;
    r->x1 = max(r->x0, r->x1 - a);
    rect ret = { r->x1, r->y0, x1, r->y1 };
    return ret;
}

static rect yo_rect_cut_top(rect *r, f32 a) {
    f32 y0 = r->y0;
    r->y0 = min(r->y1, r->y0 + a);
    rect ret = { r->x0, y0, r->x1, r->y0 };
    return ret;
}

static rect yo_rect_cut_bot(rect *r, f32 a) {
    f32 y1 = r->y1;
    r->y1 = max(r->y0, r->y1 - a);
    rect ret = { r->x0, r->y1, r->x1, y1 };
    return ret;
}

static rect yo_rect_cut_dir(rect *r, f32 a, dir2 dir) {
    switch (dir) {
        case DIR2_LEFT:     return yo_rect_cut_left(r, a);
        case DIR2_TOP:      return yo_rect_cut_top(r, a);
        case DIR2_RIGHT:    return yo_rect_cut_right(r, a);
        case DIR2_BOT:      return yo_rect_cut_bot(r, a);
        default: {
            assert(false);
            return (rect) { 0 };
        }
    }
}

static void yo_rect_cut(rect *r, f32 left, f32 right, f32 top, f32 bot) {
    yo_rect_cut_left(r, left);
    yo_rect_cut_right(r, right);
    yo_rect_cut_top(r, top);
    yo_rect_cut_bot(r, bot);
}

static void yo_rect_cut_all(rect *r, f32 a) {
    yo_rect_cut(r, a, a, a, a);
}

static void yo_rect_cut_x(rect *r, f32 a) {
    yo_rect_cut_left(r, a);
    yo_rect_cut_right(r, a);
}

static void yo_rect_cut_y(rect *r, f32 a) {
    yo_rect_cut_top(r, a);
    yo_rect_cut_bot(r, a);
}

static void yo_rect_cut_xy(rect *r, f32 x, f32 y) {
    yo_rect_cut_x(r, x);
    yo_rect_cut_y(r, y);
}

static void yo_rect_cut_vec2(rect *r, vec2 a) {
    yo_rect_cut_x(r, a.x);
    yo_rect_cut_y(r, a.y);
}

static void yo_rect_cut_centered_x(rect *r, f32 a) {
    f32 excess = max(0, rect_dim_x(*r) - a);
    yo_rect_cut_x(r, excess / 2);
}

static void yo_rect_cut_centered_y(rect *r, f32 a) {
    f32 excess = max(0, rect_dim_y(*r) - a);
    yo_rect_cut_y(r, excess / 2);
}

static void yo_rect_cut_centered_xy(rect *r, f32 x, f32 y) {
    yo_rect_cut_centered_x(r, x);
    yo_rect_cut_centered_y(r, y);
}

static void yo_rect_cut_centered_a(rect *r, f32 a, yo_axis axis) {
    switch (axis) {
        case YO_AXIS_X: yo_rect_cut_centered_x(r, a);  break;
        case YO_AXIS_Y: yo_rect_cut_centered_y(r, a);  break;
        default:        assert(false);   break;
    }
}

static void yo_rect_cut_centered(rect *r, vec2 dim) {
    yo_rect_cut_centered_x(r, dim.x);
    yo_rect_cut_centered_y(r, dim.y);
}

#if 0
static void yo_rect_cut_aligned(rect *r, vec2 pref_dim, yo_align align) {
    *r = yo_align_rect(*r, pref_dim, align);
}
#endif

#if 0
static void yo_rect_cut_border(rect *r, yo_border border) {
    yo_rect_cut_left(r, border.thickness.x0);
    yo_rect_cut_right(r, border.thickness.x1);
    yo_rect_cut_top(r, border.thickness.y0);
    yo_rect_cut_bot(r, border.thickness.y1);
}
#endif

static void yo_rect_cut_a(rect *r, f32 a, yo_axis axis) {
    switch (axis) {
        case YO_AXIS_X: yo_rect_cut_x(r, a);  break;
        case YO_AXIS_Y: yo_rect_cut_y(r, a);  break;
        default:        assert(false);   break;
    }
}

static void yo_rect_add_left(rect *r, f32 a) {
    r->x0 -= a;
}

static void yo_rect_add_right(rect *r, f32 a) {
    r->x1 += a;
}

static void yo_rect_add_top(rect *r, f32 a) {
    r->y0 -= a;
}

static void yo_rect_add_bot(rect *r, f32 a) {
    r->y1 += a;
}

static void yo_rect_add_dir(rect *r, f32 a, dir2 dir) {
    switch (dir) {
        case DIR2_LEFT:     yo_rect_add_left(r, a);   break;
        case DIR2_TOP:      yo_rect_add_top(r, a);    break;
        case DIR2_RIGHT:    yo_rect_add_right(r, a);  break;
        case DIR2_BOT:      yo_rect_add_bot(r, a);    break;
        default:            assert(false);       break;
    }
}

static void yo_add(rect *r, f32 left, f32 right, f32 top, f32 bot) {
    yo_rect_add_left(r, left);
    yo_rect_add_right(r, right);
    yo_rect_add_top(r, top);
    yo_rect_add_bot(r, bot);
}

static void yo_rect_add_all(rect *r, f32 a) {
    yo_add(r, a, a, a, a);
}

static void yo_rect_add_x(rect *r, f32 a) {
    yo_rect_add_left(r, a);
    yo_rect_add_right(r, a);
}

static void yo_rect_add_y(rect *r, f32 a) {
    yo_rect_add_top(r, a);
    yo_rect_add_bot(r, a);
}

static void yo_rect_add_axis(rect *r, f32 a, yo_axis axis) {
    switch (axis) {
        case YO_AXIS_X: yo_rect_add_x(r, a);  break;
        case YO_AXIS_Y: yo_rect_add_y(r, a);  break;
        default:        assert(false);   break;
    }
}

static void yo_rect_add(rect *r, f32 left, f32 right, f32 top, f32 bot) {
    yo_rect_add_left(r, left);
    yo_rect_add_right(r, right);
    yo_rect_add_top(r, top);
    yo_rect_add_bot(r, bot);
}

static void yo_rect_max_left(rect *r, f32 a) {
    *r = yo_rect_cut_left(r, a);
}

static void yo_rect_max_right(rect *r, f32 a) {
    *r = yo_rect_cut_right(r, a);
}

static void yo_rect_max_top(rect *r, f32 a) {
    *r = yo_rect_cut_top(r, a);
}

static void yo_rect_max_bot(rect *r, f32 a) {
    *r = yo_rect_cut_bot(r, a);
}

static void yo_rect_max_dir(rect *r, f32 a, dir2 dir) {
    switch (dir) {
        case DIR2_LEFT:     yo_rect_max_left(r, a);   break;
        case DIR2_TOP:      yo_rect_max_top(r, a);    break;
        case DIR2_RIGHT:    yo_rect_max_right(r, a);  break;
        case DIR2_BOT:      yo_rect_max_bot(r, a);    break;
        default:            assert(false);       break;
    }
}

static rect yo_rect_get_left(rect *r, f32 a) {
    f32 x0 = r->x0;
    f32 x1 = min(r->x1, r->x0 + a);
    rect ret = { x0, r->y0, x1, r->y1 };
    return ret;
}

static rect yo_rect_get_right(rect *r, f32 a) {
    f32 x1 = r->x1;
    f32 x0 = max(r->x0, r->x1 - a);
    rect ret = { x0, r->y0, x1, r->y1 };
    return ret;
}

static rect yo_rect_get_top(rect *r, f32 a) {
    f32 y0 = r->y0;
    f32 y1 = min(r->y1, r->y0 + a);
    rect ret = { r->x0, y0, r->x1, y1 };
    return ret;
}

static rect yo_rect_get_bot(rect *r, f32 a) {
    f32 y1 = r->y1;
    f32 y0 = max(r->y0, r->y1 - a);
    rect ret = { r->x0, y0, r->x1, y1 };
    return ret;
}

static rect yo_rect_get_dir(rect *r, f32 a, dir2 dir) {
    switch (dir) {
        case DIR2_LEFT:     return yo_rect_get_left(r, a);
        case DIR2_TOP:      return yo_rect_get_top(r, a);
        case DIR2_RIGHT:    return yo_rect_get_right(r, a);
        case DIR2_BOT:      return yo_rect_get_bot(r, a);
        default: {
            assert(false);
            return (rect) { 0 };
        }
    }
}

static rect yo_rect_get_centered_x(rect *r, f32 a) {
    rect ret = *r;
    yo_rect_cut_centered_x(&ret, a);
    return ret;
}

static rect yo_rect_get_centered_y(rect *r, f32 a) {
    rect ret = *r;
    yo_rect_cut_centered_y(&ret, a);
    return ret;
}

static rect yo_rect_get_centered(rect *r, vec2 dim) {
    rect ret = *r;
    yo_rect_cut_centered(&ret, dim);
    return ret;
}

////////////////////////////////////////////////////////////////
// rune: Animation

static f32 yo_delta_time(void) {
    yo_state *state = yo_state_get();
    return state->delta_time;
}

static void yo_anim_base(f32 *value, f32 target, f32 rate) {
    const f32 epsilon = 0.01f;

    f32 origin = *value;
    f32 next = target;
    next = origin + (target - origin) * rate * yo_delta_time();
    next = clamp(next, min(origin, target), max(origin, target));

    if (f32_abs(origin - target) < epsilon) {
        next = target;
    } else {
        // TODO(rune): yo_invalidate_next_frame();
    }

    *value = next;
}

static f32 yo_anim_f32(f32 *pos, f32 target, f32 rate) {
    yo_anim_base(pos, target, rate);
    return *pos;
}

static vec2 yo_anim_vec2(vec2 *pos, vec2 target, f32 rate) {
    yo_anim_base(&pos->x, target.x, rate);
    yo_anim_base(&pos->y, target.y, rate);
    return *pos;
}

static vec3 yo_anim_vec3(vec3 *pos, vec3 target, f32 rate) {
    yo_anim_base(&pos->x, target.x, rate);
    yo_anim_base(&pos->y, target.y, rate);
    yo_anim_base(&pos->z, target.z, rate);
    return *pos;
}

static vec4 yo_anim_vec4(vec4 *pos, vec4 target, f32 rate) {
    yo_anim_base(&pos->x, target.x, rate);
    yo_anim_base(&pos->y, target.y, rate);
    yo_anim_base(&pos->z, target.z, rate);
    yo_anim_base(&pos->w, target.w, rate);
    return *pos;
}

static rect yo_anim_rect(rect *pos, rect target, f32 rate) {
    yo_anim_base(&pos->x0, target.x0, rate);
    yo_anim_base(&pos->y1, target.y1, rate);
    yo_anim_base(&pos->x0, target.x0, rate);
    yo_anim_base(&pos->y1, target.y1, rate);
    return *pos;
}

static void yo_invalidate_next_frame(void) {
    yo_state *state = yo_state_get();
    state->invalidate_next_frame = true;
}

static void yo_anim_damped_base(f32 *pos, f32 *vel, f32 target, f32 rate, f32 epsilon, bool started) {
    assert(!f32_is_nan(*pos));
    assert(!f32_is_nan(*vel));

    if (started) {
        *pos = f32_smooth_damp(*pos, target, vel, 1 / rate, F32_MAX, yo_delta_time());
        if (f32_abs(*pos - target) < epsilon) {
            *pos = target;
        } else {
            yo_invalidate_next_frame();
        }
    } else {
        *pos = target;
    }

    assert(!f32_is_nan(*pos));
    assert(!f32_is_nan(*vel));
}

static f32 yo_anim_damped_f32(f32 *pos, f32 *vel, f32 target, f32 rate, f32 epsilon, bool *started, bool *completed) {
    yo_anim_damped_base(pos, vel, target, rate, epsilon, *started);
    *started = true;
    *completed = *pos == target;
    return *pos;
}

static vec2 yo_anim_damped_vec2(vec2 *pos, vec2 *vel, vec2 target, f32 rate, f32 epsilon, bool *started, bool *completed) {
    yo_anim_damped_base(&pos->x, &vel->x, target.x, rate, epsilon, *started);
    yo_anim_damped_base(&pos->y, &vel->y, target.y, rate, epsilon, *started);
    *started = true;
    *completed = vec2_eq(*pos, target);
    return *pos;
}

static vec4 yo_anim_damped_vec4(vec4 *pos, vec4 *vel, vec4 target, f32 rate, f32 epsilon, bool *started, bool *completed) {
    yo_anim_damped_base(&pos->x, &vel->x, target.x, rate, epsilon, *started);
    yo_anim_damped_base(&pos->y, &vel->y, target.y, rate, epsilon, *started);
    yo_anim_damped_base(&pos->z, &vel->z, target.z, rate, epsilon, *started);
    yo_anim_damped_base(&pos->w, &vel->w, target.w, rate, epsilon, *started);
    *started = true;
    *completed = vec4_eq(*pos, target);
    return *pos;
}

// TODO(rune): Remove
static f32 yo_em(f32 a) {
    yo_state *state = yo_state_get();
    return state->curr_lineheight;
}

////////////////////////////////////////////////////////////////
// rune: String conversions

static str yo_str_from_op_kind(yo_op_kind type) {
    switch (type) {
        case YO_OP_KIND_NONE:    return str("YO_OP_KIND_NONE");
        case YO_OP_KIND_GLYPH:   return str("YO_OP_KIND_GLYPH");
        case YO_OP_KIND_TEXT:    return str("YO_OP_KIND_TEXT");
        case YO_OP_KIND_AABB:    return str("YO_OP_KIND_AABB");
        case YO_OP_KIND_AABB_EX: return str("YO_OP_KIND_AABB_EX");
        case YO_OP_KIND_QUAD:    return str("YO_OP_KIND_QUAD");
        default:                 return str("INVALID");
    }
}

#define yo_fmt(...) yo_fmt_args(argsof(__VA_ARGS__))

static str yo_fmt_args(args args) {
    return arena_print_args(yo_frame_arena(), args);
}


