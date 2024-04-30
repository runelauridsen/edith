////////////////////////////////////////////////////////////////
// rune: Globals

static f32 ui_dummy_font_backend_get_advance(void *userdata, ui_face face, u32 codepoint) { unused(userdata, face, codepoint); return 1; }
static f32 ui_dummy_font_backend_get_lineheight(void *userdata, ui_face face) { unused(userdata, face); return 1; }

static ui_ctx global_dummy_ui_ctx = {
    .font_backend.get_advance = ui_dummy_font_backend_get_advance,
    .font_backend.get_lineheight = ui_dummy_font_backend_get_lineheight,
};

static ui_ctx *global_ui_ctx = &global_dummy_ui_ctx;

////////////////////////////////////////////////////////////////
// rune: Ops

static char *ui_op_type_as_cstr(ui_op_type type);

#define UI_OP_MAX_DATA_SIZE 256

static void *ui_add_op(ui_op_type type, u32 data_size) {
    assert(data_size < UI_OP_MAX_DATA_SIZE);

    ui_ctx *ctx = global_ui_ctx;

    ui_op_def *def = buf_push_struct(&ctx->op_def_buf, ui_op_def);
    void *data = buf_push_size(&ctx->op_data_buf, data_size);

    static u8 fallback_data[UI_OP_MAX_DATA_SIZE];

    // @Todo Error-free API? Just return pointers to static memory in case of allocation failure.
    if (data && def) {
        u32 data_offset = data_size ? (u32)ptr_diff(data, ctx->op_data_buf.v) : 0;

        def->type = type;
        def->data_offset = data_offset;
    } else {
        // @Todo Log out-of-memory.
        data = fallback_data;
        if (def) def->type = UI_OP_TYPE_NONE;
    }

    return data;
}

static ui_op_glyph *ui_add_op_glyph(void) {
    return ui_add_op(UI_OP_TYPE_GLYPH, sizeof(ui_op_glyph));
}

static ui_op_text *ui_add_op_text(void) {
    return ui_add_op(UI_OP_TYPE_TEXT, sizeof(ui_op_text));
}

static ui_op_aabb *ui_add_op_aabb(void) {
    return ui_add_op(UI_OP_TYPE_AABB, sizeof(ui_op_aabb));
}

static ui_op_aabb_ex *ui_add_op_aabb_ex(void) {
    return ui_add_op(UI_OP_TYPE_AABB_EX, sizeof(ui_op_aabb_ex));
}

static ui_op_quad *ui_add_op_quad(void) {
    return ui_add_op(UI_OP_TYPE_QUAD, sizeof(ui_op_quad));
}

static ui_op_scissor *ui_add_op_scissor(void) {
    return ui_add_op(UI_OP_TYPE_SCISSOR, sizeof(ui_op_scissor));
}

static ui_op_transform *ui_add_op_transform(void) {
    return ui_add_op(UI_OP_TYPE_TRANSFORM, sizeof(ui_op_transform));
}

static char *ui_op_type_as_cstr(ui_op_type type) {
    static readonly char *table[] = {
        [UI_OP_TYPE_NONE] = "UI_OP_TYPE_NONE",
        [UI_OP_TYPE_GLYPH]          = "UI_OP_TYPE_GLYPH",
        [UI_OP_TYPE_TEXT]           = "UI_OP_TYPE_TEXT",
        [UI_OP_TYPE_AABB]           = "UI_OP_TYPE_AABB",
        [UI_OP_TYPE_AABB_EX]        = "UI_OP_TYPE_AABB_EX",
        [UI_OP_TYPE_QUAD]           = "UI_OP_TYPE_QUAD",
        [UI_OP_TYPE_SCISSOR]        = "UI_OP_TYPE_SCISSOR",
        [UI_OP_TYPE_PUSH_SCISSOR]   = "UI_OP_TYPE_PUSH_SCISSOR",
        [UI_OP_TYPE_POP_SCISSOR]    = "UI_OP_TYPE_POP_SCISSOR",
        [UI_OP_TYPE_TRANSFORM]      = "UI_OP_TYPE_TRANSFORM",
        [UI_OP_TYPE_PUSH_TRANSFORM] = "UI_OP_TYPE_PUSH_TRANSFORM",
        [UI_OP_TYPE_POP_TRANSFORM]  = "UI_OP_TYPE_POP_TRANSFORM",
    };

    char *ret = null;
    if (type >= 0 && type < countof(table)) {
        ret = table[type];
    } else {
        __nop();
    }

    ret = coalesce(ret, "INVALID");
    return ret;
}

#define ui_op_get_data(T, offset) ((T *)(global_ui_ctx->op_data_buf.v + offset))

////////////////////////////////////////////////////////////////
// rune: Font backend

static ui_face ui_make_face(ui_font font, u16 fontsize) {
    ui_face ret = {
        .font = font,
        .fontsize = fontsize,
    };
    return ret;
}

static f32 ui_font_backend_get_advance(u32 codepoint, ui_face face) {
    ui_ctx *ctx = global_ui_ctx;
    ui_font_backend *backend = &ctx->font_backend;
    f32 ret = backend->get_advance(backend->userdata, face, codepoint);
    return ret;
}

static f32 ui_font_backend_get_lineheight(ui_face face) {
    ui_ctx *ctx = global_ui_ctx;
    ui_font_backend *backend = &ctx->font_backend;
    f32 ret = backend->get_lineheight(backend->userdata, face);
    return ret;
}

////////////////////////////////////////////////////////////////
// rune: Events

static inline ui_events ui_empty_events(void) {
    return (ui_events) { 0 };
}

static inline bool ui_event_is_keypress(ui_event *e, ui_key key) {
    return e->type == UI_EVENT_TYPE_KEYPRESS && e->key == key;
}

static inline bool ui_event_is_keypress_with_modifiers(ui_event *e, ui_key key, ui_modifiers modifiers) {
    return e->type == UI_EVENT_TYPE_KEYPRESS && e->key == key && e->modifiers == modifiers;
}

static inline bool ui_event_is_key_release(ui_event *e, ui_key key) {
    return e->type == UI_EVENT_TYPE_KEY_RELEASE && e->key == key;
}

////////////////////////////////////////////////////////////////
// rune: Stacks

static void ui_face_changed(void) {
    YO_PROFILE_BEGIN(ui_face_changed);

    ui_ctx *ctx = global_ui_ctx;
    ui_face v = ui_stack_get(ui_face, &ctx->face_stack);

    // rune: Cache lineheight.
    ctx->current_lineheight = ui_font_backend_get_lineheight(v);

    // rune: Cache advance of ascii codepoints.
    for_n (u32, i, countof(ctx->current_advance_ascii)) {
        ctx->current_advance_ascii[i] = ui_font_backend_get_advance(i, v);
    }

    YO_PROFILE_END(ui_face_changed);
}

static ui_face ui_get_face(void) { return ui_stack_get(ui_face, &global_ui_ctx->face_stack); }
static void ui_set_face(ui_face v) { ui_stack_set(ui_face, &global_ui_ctx->face_stack, v); ui_face_changed(); }
static void ui_push_face(void) { ui_stack_push(ui_face, &global_ui_ctx->face_stack); }
static void ui_pop_face(void) { ui_stack_pop(ui_face, &global_ui_ctx->face_stack); ui_face_changed(); }

static bool ui_get_pixel_snap(void) { return ui_stack_get(b8, &global_ui_ctx->pixel_snap_stack); }
static void ui_set_pixel_snap(bool v) { ui_stack_set(b8, &global_ui_ctx->pixel_snap_stack, v); }
static void ui_push_pixel_snap(void) { ui_stack_push(b8, &global_ui_ctx->pixel_snap_stack); }
static void ui_pop_pixel_snap(void) { ui_stack_pop(b8, &global_ui_ctx->pixel_snap_stack); }

static ui_layout ui_get_layout(void) { return ui_stack_get(ui_pushed_layout, &global_ui_ctx->layout_stack).v; }
static void ui_set_layout(ui_layout v) { ui_stack_set(ui_pushed_layout, &global_ui_ctx->layout_stack, (ui_pushed_layout) { .v = v }); }
static void ui_push_layout(void) { ui_stack_push(ui_pushed_layout, &global_ui_ctx->layout_stack); }
static void ui_pop_layout(void) { ui_stack_pop(ui_pushed_layout, &global_ui_ctx->layout_stack); }

static rect ui_get_scissor(void) { return ui_stack_get(rect, &global_ui_ctx->scissor_stack); }
static void ui_push_scissor(void) { ui_stack_push(rect, &global_ui_ctx->scissor_stack); ui_add_op(UI_OP_TYPE_PUSH_SCISSOR, 0); }
static void ui_pop_scissor(void) { ui_stack_pop(rect, &global_ui_ctx->scissor_stack); ui_add_op(UI_OP_TYPE_POP_SCISSOR, 0); }
static void ui_set_scissor(rect v, ui_scissor_flags flags) {
#ifdef APP_DOES_SCISSOR_INTERSECT_AND_TRANSFORM_ADD
#else
    if (!(flags & UI_SCISSOR_FLAG_NO_INTERSECT)) {
        rect_offset_assign(&v, ui_get_transform());
        rect_intersect_assign(&v, ui_stack_get_prev(rect, &global_ui_ctx->scissor_stack));
    }
#endif

    ui_stack_set(rect, &global_ui_ctx->scissor_stack, v);
    ui_op_scissor *op = ui_add_op_scissor();
    op->rect = v;
    op->flags = flags;
}

static vec2 ui_get_transform(void) { return ui_stack_get(vec2, &global_ui_ctx->transform_stack); }
static void ui_push_transform(void) { ui_stack_push(vec2, &global_ui_ctx->transform_stack); ui_add_op(UI_OP_TYPE_PUSH_TRANSFORM, 0); }
static void ui_pop_transform(void) { ui_stack_pop(vec2, &global_ui_ctx->transform_stack); ui_add_op(UI_OP_TYPE_POP_TRANSFORM, 0); }
static ui_op_transform *ui_set_transform(vec2 v, ui_transform_flags flags) {
#ifdef APP_DOES_SCISSOR_INTERSECT_AND_TRANSFORM_ADD
#else
    if (!(flags & UI_TRANSFORM_FLAG_NO_RELATIVE)) {
        vec2_add_assign(&v, ui_stack_get_prev(vec2, &global_ui_ctx->transform_stack));
    }
#endif

    if (ui_get_pixel_snap()) {
        vec2_round_assign(&v);
    }

    ui_stack_set(vec2, &global_ui_ctx->transform_stack, v);
    ui_op_transform *op = ui_add_op_transform();
    op->vec = v;

    return op;
}

////////////////////////////////////////////////////////////////
// rune: Rect cut

static rect ui_cut_left(rect *r, f32 a) {
    f32 x0 = r->x0;
    r->x0 = min(r->x1, r->x0 + a);
    rect ret = { x0, r->y0, r->x0, r->y1 };
    return ret;
}

static rect ui_cut_right(rect *r, f32 a) {
    f32 x1 = r->x1;
    r->x1 = max(r->x0, r->x1 - a);
    rect ret = { r->x1, r->y0, x1, r->y1 };
    return ret;
}

static rect ui_cut_top(rect *r, f32 a) {
    f32 y0 = r->y0;
    r->y0 = min(r->y1, r->y0 + a);
    rect ret = { r->x0, y0, r->x1, r->y0 };
    return ret;
}

static rect ui_cut_bot(rect *r, f32 a) {
    f32 y1 = r->y1;
    r->y1 = max(r->y0, r->y1 - a);
    rect ret = { r->x0, r->y1, r->x1, y1 };
    return ret;
}

static rect ui_cut_dir(rect *r, f32 a, dir2 dir) {
    switch (dir) {
        case DIR2_LEFT:     return ui_cut_left(r, a);
        case DIR2_TOP:      return ui_cut_top(r, a);
        case DIR2_RIGHT:    return ui_cut_right(r, a);
        case DIR2_BOT:      return ui_cut_bot(r, a);
        default: {
            assert(false);
            return (rect) { 0 };
        }
    }
}

static void ui_rect(rect *r, f32 left, f32 right, f32 top, f32 bot) {
    ui_cut_left(r, left);
    ui_cut_right(r, right);
    ui_cut_top(r, top);
    ui_cut_bot(r, bot);
}

static void ui_cut_all(rect *r, f32 a) {
    ui_rect(r, a, a, a, a);
}

static void ui_cut_x(rect *r, f32 a) {
    ui_cut_left(r, a);
    ui_cut_right(r, a);
}

static void ui_cut_y(rect *r, f32 a) {
    ui_cut_top(r, a);
    ui_cut_bot(r, a);
}

static void ui_cut_xy(rect *r, f32 x, f32 y) {
    ui_cut_x(r, x);
    ui_cut_y(r, y);
}

static void ui_cut_vec2(rect *r, vec2 a) {
    ui_cut_x(r, a.x);
    ui_cut_y(r, a.y);
}

static void ui_cut_centered_x(rect *r, f32 a) {
    f32 excess = max(0, rect_dim_x(*r) - a);
    ui_cut_x(r, excess / 2);
}

static void ui_cut_centered_y(rect *r, f32 a) {
    f32 excess = max(0, rect_dim_y(*r) - a);
    ui_cut_y(r, excess / 2);
}

static void ui_cut_centered_xy(rect *r, f32 x, f32 y) {
    ui_cut_centered_x(r, x);
    ui_cut_centered_y(r, y);
}

static void ui_cut_centered_a(rect *r, f32 a, axis2 axis) {
    switch (axis) {
        case AXIS2_X:    ui_cut_centered_x(r, a);  break;
        case AXIS2_Y:    ui_cut_centered_y(r, a);  break;
        default:         assert(false);   break;
    }
}

static void ui_cut_centered(rect *r, vec2 dim) {
    ui_cut_centered_x(r, dim.x);
    ui_cut_centered_y(r, dim.y);
}

static void ui_cut_aligned(rect *r, vec2 pref_dim, ui_align align) {
    *r = ui_align_rect(*r, pref_dim, align);
}

static void ui_cut_border(rect *r, ui_border border) {
    ui_cut_left(r, border.thickness.x0);
    ui_cut_right(r, border.thickness.x1);
    ui_cut_top(r, border.thickness.y0);
    ui_cut_bot(r, border.thickness.y1);
}

static void ui_cut_a(rect *r, f32 a, axis2 axis) {
    switch (axis) {
        case AXIS2_X:    ui_cut_x(r, a);  break;
        case AXIS2_Y:    ui_cut_y(r, a);  break;
        default:         assert(false);   break;
    }
}

static void ui_cut_sides(rect *r, f32 left, f32 right, f32 top, f32 bot) {
    ui_cut_left(r, left);
    ui_cut_right(r, right);
    ui_cut_top(r, top);
    ui_cut_bot(r, bot);
}

static void ui_add_left(rect *r, f32 a) {
    r->x0 -= a;
}

static void ui_add_right(rect *r, f32 a) {
    r->x1 += a;
}

static void ui_add_top(rect *r, f32 a) {
    r->y0 -= a;
}

static void ui_add_bot(rect *r, f32 a) {
    r->y1 += a;
}

static void ui_add_dir(rect *r, f32 a, dir2 dir) {
    switch (dir) {
        case DIR2_LEFT:     ui_add_left(r, a);   break;
        case DIR2_TOP:      ui_add_top(r, a);    break;
        case DIR2_RIGHT:    ui_add_right(r, a);  break;
        case DIR2_BOT:      ui_add_bot(r, a);    break;
        default:            assert(false);       break;
    }
}

static void ui_add(rect *r, f32 left, f32 right, f32 top, f32 bot) {
    ui_add_left(r, left);
    ui_add_right(r, right);
    ui_add_top(r, top);
    ui_add_bot(r, bot);
}

static void ui_add_all(rect *r, f32 a) {
    ui_add(r, a, a, a, a);
}

static void ui_add_x(rect *r, f32 a) {
    ui_add_left(r, a);
    ui_add_right(r, a);
}

static void ui_add_y(rect *r, f32 a) {
    ui_add_top(r, a);
    ui_add_bot(r, a);
}

static void ui_add_axis(rect *r, f32 a, axis2 axis) {
    switch (axis) {
        case AXIS2_X:   ui_add_x(r, a);  break;
        case AXIS2_Y:   ui_add_y(r, a);  break;
        default:        assert(false);   break;
    }
}

static void ui_add_sides(rect *r, f32 left, f32 right, f32 top, f32 bot) {
    ui_add_left(r, left);
    ui_add_right(r, right);
    ui_add_top(r, top);
    ui_add_bot(r, bot);
}

static void ui_max_left(rect *r, f32 a) {
    *r = ui_cut_left(r, a);
}

static void ui_max_right(rect *r, f32 a) {
    *r = ui_cut_right(r, a);
}

static void ui_max_top(rect *r, f32 a) {
    *r = ui_cut_top(r, a);
}

static void ui_max_bot(rect *r, f32 a) {
    *r = ui_cut_bot(r, a);
}

static void ui_max_dir(rect *r, f32 a, dir2 dir) {
    switch (dir) {
        case DIR2_LEFT:     ui_max_left(r, a);   break;
        case DIR2_TOP:      ui_max_top(r, a);    break;
        case DIR2_RIGHT:    ui_max_right(r, a);  break;
        case DIR2_BOT:      ui_max_bot(r, a);    break;
        default:            assert(false);       break;
    }
}

static rect ui_get_left(rect *r, f32 a) {
    f32 x0 = r->x0;
    f32 x1 = min(r->x1, r->x0 + a);
    rect ret = { x0, r->y0, x1, r->y1 };
    return ret;
}

static rect ui_get_right(rect *r, f32 a) {
    f32 x1 = r->x1;
    f32 x0 = max(r->x0, r->x1 - a);
    rect ret = { x0, r->y0, x1, r->y1 };
    return ret;
}

static rect ui_get_top(rect *r, f32 a) {
    f32 y0 = r->y0;
    f32 y1 = min(r->y1, r->y0 + a);
    rect ret = { r->x0, y0, r->x1, y1 };
    return ret;
}

static rect ui_get_bot(rect *r, f32 a) {
    f32 y1 = r->y1;
    f32 y0 = max(r->y0, r->y1 - a);
    rect ret = { r->x0, y0, r->x1, y1 };
    return ret;
}

static rect ui_get_dir(rect *r, f32 a, dir2 dir) {
    switch (dir) {
        case DIR2_LEFT:     return ui_get_left(r, a);
        case DIR2_TOP:      return ui_get_top(r, a);
        case DIR2_RIGHT:    return ui_get_right(r, a);
        case DIR2_BOT:      return ui_get_bot(r, a);
        default: {
            assert(false);
            return (rect) { 0 };
        }
    }
}

static rect ui_get_x_centered(rect *r, f32 a) {
    rect ret = *r;
    ui_cut_centered_x(&ret, a);
    return ret;
}

static rect ui_get_y_centered(rect *r, f32 a) {
    rect ret = *r;
    ui_cut_centered_y(&ret, a);
    return ret;
}

static rect ui_get_centered(rect *r, vec2 dim) {
    rect ret = *r;
    ui_cut_centered(&ret, dim);
    return ret;
}

////////////////////////////////////////////////////////////////
// rune: Draw

static f32 ui_round_if_snap_to_pixel_f32(f32 v) {
    if (ui_get_pixel_snap()) {
        return f32_round(v);
    } else {
        return v;
    }
}

static vec2 ui_round_if_snap_to_pixel_vec2(vec2 v) {
    if (ui_get_pixel_snap()) {
        return vec2_round(v);
    } else {
        return v;
    }
}

static rect ui_round_if_snap_to_pixel_rect(rect v) {
    if (ui_get_pixel_snap()) {
        return rect_round(v);
    } else {
        return v;
    }
}

static void ui_draw_rect(rect dst, u32 color) {
    dst = ui_round_if_snap_to_pixel_rect(dst);
    ui_op_aabb *op = ui_add_op_aabb();
    op->color = color;
    op->rect = dst;
}

static void ui_draw_gradient(rect dst, u32 color0, u32 color1, u32 color2, u32 color3) {
    dst = ui_round_if_snap_to_pixel_rect(dst);
    ui_op_aabb_ex *op = ui_add_op_aabb_ex();
    op->dst_rect         = dst;
    op->color[0]         = color0;
    op->color[1]         = color1;
    op->color[2]         = color2;
    op->color[3]         = color3;
}

static void ui_draw_rounded_rect(rect dst, u32 color, f32 roundness) {
    dst = ui_round_if_snap_to_pixel_rect(dst);
    ui_op_aabb_ex *op = ui_add_op_aabb_ex();
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

static void ui_draw_soft_rect(rect dst, u32 color, f32 softness) {
    dst = ui_round_if_snap_to_pixel_rect(dst);
    ui_op_aabb_ex *op = ui_add_op_aabb_ex();
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

static void ui_draw_textured(rect dst, u32 color, rect uv) {
    dst = ui_round_if_snap_to_pixel_rect(dst);
    ui_op_aabb_ex *op = ui_add_op_aabb_ex();
    op->dst_rect         = dst;
    op->tex_rect         = uv;
    op->tex_weight       = 1.0f;
    op->color[0]         = color;
    op->color[1]         = color;
    op->color[2]         = color;
    op->color[3]         = color;
}

static void ui_draw_rounded_shadow(rect dst, f32 shadow_radius, f32 roundness, f32 shadow_softness) {
    f32 softness = shadow_radius + roundness + shadow_softness;

    dst.x0 -= shadow_radius;
    dst.y0 -= shadow_radius;
    dst.x1 += shadow_radius;
    dst.y1 += shadow_radius;

    dst = ui_round_if_snap_to_pixel_rect(dst);
    ui_op_aabb_ex *op = ui_add_op_aabb_ex();
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

static void ui_draw_shadow(rect dst, f32 shadow_radius, f32 shadow_softness) {
    dst = ui_round_if_snap_to_pixel_rect(dst);
    ui_draw_rounded_shadow(dst, shadow_radius, 0, shadow_softness);
}

static void ui_draw_rect_with_shadow(rect dst, u32 color, f32 shadow_radius, f32 shadow_softness) {
    dst = ui_round_if_snap_to_pixel_rect(dst);
    ui_draw_shadow(dst, shadow_radius, shadow_softness);
    ui_draw_rect(dst, color);
}

static void ui_draw_rounded_rect_with_shadow(rect dst, u32 color, f32 roundness, f32 shadow_radius, f32 shadow_softness) {
    dst = ui_round_if_snap_to_pixel_rect(dst);
    ui_draw_rounded_shadow(dst, shadow_radius, roundness, shadow_softness);
    ui_draw_rounded_rect(dst, color, roundness);
}

static f32 ui_measure_glyph_(u32 codepoint, ui_face face) {
    f32 ret = ui_font_backend_get_advance(codepoint, face);
    return ret;
}

static f32 ui_measure_glyph(u32 codepoint) {
    return ui_measure_glyph_(codepoint, ui_get_face());
}

static vec2 ui_measure_text(str s) {
    assert(is_well_formed_utf8(s));

    YO_PROFILE_BEGIN(ui_measure_text);
    ui_ctx *ctx = global_ui_ctx;

    f32 advance_sum = 0.0f;
    i64 i = 0;
    while (i < s.len) {
        if (s.v[i] < 128) {
            f32 advance = ctx->current_advance_ascii[s.v[i]];
            advance_sum += advance;
            i += 1;
        } else {
            unicode_codepoint decoded = decode_single_utf8_codepoint(str_make(s.v + i, s.len - i));
            f32 advance = ui_measure_glyph(decoded.codepoint);
            advance_sum += advance;
            i += decoded.len;
        }
    }

    YO_PROFILE_END(ui_measure_text);
    vec2 ret = vec2(advance_sum, ctx->current_lineheight);
    return ret;
}

static void ui_draw_glyph(vec2 p, u32 codepoint, u32 color) {
    ui_op_glyph *op = ui_add_op_glyph();
    op->codepoint = codepoint;
    op->color     = color;
    op->p         = p;
    op->face      = ui_get_face();
}

static void ui_draw_glyph_r(u32 codepoint, rect dst, u32 color) {
    ui_push_scissor();
    ui_set_scissor(dst, 0);
    ui_draw_glyph(dst.p0, codepoint, color);
    ui_pop_scissor();
}

static void ui_draw_text(vec2 p, str s, u32 color) {
    ui_op_text *op = ui_add_op_text();
    op->color     = color;
    op->p         = p;
    op->s         = s;
    op->face      = ui_get_face();
}

static void ui_draw_text_r(rect dst, str s, u32 color) {
    ui_push_scissor();
    ui_set_scissor(dst, 0);
    ui_draw_text(dst.p0, s, color);
    ui_pop_scissor();
}

static vec2 ui_draw_text_centered_r(rect dst, str s, u32 color) {
    vec2 dim = ui_measure_text(s);
    ui_cut_centered(&dst, dim);
    ui_draw_text_r(dst, s, color);
    return dim;
}

static void ui_draw_caret(vec2 p, f32 thickness, u32 color) {
    ui_ctx *ctx = global_ui_ctx;
    vec2 dim = vec2(thickness, ctx->current_lineheight);
    rect rect = rect_make_dim(p, dim);
    rect = ui_round_if_snap_to_pixel_rect(rect);

    ui_op_aabb *op = ui_add_op_aabb();
    op->color = color; // @Todo User specify color.
    op->rect = rect;
}

static void ui_draw_border(rect r, ui_border border) {
    ui_draw_rounded_rect(r, border.color, border.radius.v[0]); // @Todo Different radius per corner.

    // @Todo Draw hallow rectangles.
}

////////////////////////////////////////////////////////////////
// rune: Ids

static ui_id ui_id_root(void) { return 0xffff'ffff'ffff'ffff; }

static ui_id ui_id_combine(ui_id a, ui_id b) {
    ui_id ret = a ^ (b + 0x9e3779b9 + (a << 6) + (a >> 2));
    return ret;
}

static ui_id ui_hash_base(u64 a) {
    a ^= a >> 33;
    a *= 0xff51afd7ed558ccd;
    a ^= a >> 33;
    a *= 0xc4ceb9fe1a85ec53;
    a ^= a >> 33;
    return a;
}

static ui_id ui_hash_i8(i8 a) { return ui_hash_base(a); }
static ui_id ui_hash_i16(i16 a) { return ui_hash_base(a); }
static ui_id ui_hash_i32(i32 a) { return ui_hash_base(a); }
static ui_id ui_hash_i64(i64 a) { return ui_hash_base(a); }
static ui_id ui_hash_u8(u8 a) { return ui_hash_base(a); }
static ui_id ui_hash_u16(u16 a) { return ui_hash_base(a); }
static ui_id ui_hash_u32(u32 a) { return ui_hash_base(a); }
static ui_id ui_hash_u64(u64 a) { return ui_hash_base(a); }
static ui_id ui_hash_f32(f32 a) { return ui_hash_base(u32_from_f32(a)); }
static ui_id ui_hash_f64(f64 a) { return ui_hash_base(u64_from_f64(a)); }
static ui_id ui_hash_ptr(void *a) { return ui_hash_base((u64)a); }
static ui_id ui_hash_bool(bool a) { return ui_hash_base(a); }

static ui_id ui_hash_str(str s) {
    // @Speed Profile different hash functions.
    // Reference: https://stackoverflow.com/a/7666577.
    u64 hash = 5381;
    for_array (u8, c, s) {
        hash = ((hash << 5) + hash) + u64(*c);
    }

    ui_id combine_with = ui_stack_get(ui_box_ptr, &global_ui_ctx->box_parent_stack)->id;
    ui_id ret = ui_id_combine(combine_with, hash);
    return ret;
}

static ui_id ui_hash_cstr(char *s) {
    // @Speed Profile different hash functions.
    // Reference: https://stackoverflow.com/a/7666577.
    u64 hash = 5381;
    while (*s) {
        hash = ((hash << 5) + hash) + u64(*s);
        s++;
    }

    ui_id combine_with = ui_stack_get(ui_box_ptr, &global_ui_ctx->box_parent_stack)->id;
    ui_id ret = ui_id_combine(combine_with, hash);
    return ret;
}

static ui_id ui_hash_args(args args) {
    YO_PROFILE_BEGIN(ui_hash_args);

    ui_id ret = { 0 };
    for_array (any, it, args) {
        ui_id it_id = { 0 };
        switch (it->tag) {
            case ANY_TAG_I8:        ret = ui_hash_i8(it->_i8);          break;
            case ANY_TAG_I16:       ret = ui_hash_i16(it->_i16);        break;
            case ANY_TAG_I32:       ret = ui_hash_i32(it->_i32);        break;
            case ANY_TAG_I64:       ret = ui_hash_i64(it->_i64);        break;
            case ANY_TAG_U8:        ret = ui_hash_u8(it->_u8);          break;
            case ANY_TAG_U16:       ret = ui_hash_u16(it->_u16);        break;
            case ANY_TAG_U32:       ret = ui_hash_u32(it->_u32);        break;
            case ANY_TAG_U64:       ret = ui_hash_u64(it->_u64);        break;
            case ANY_TAG_F32:       ret = ui_hash_f32(it->_f32);        break;
            case ANY_TAG_F64:       ret = ui_hash_f64(it->_f64);        break;
            case ANY_TAG_BOOL:      ret = ui_hash_bool(it->_bool);      break;
            case ANY_TAG_STR:       ret = ui_hash_str(it->_str);        break;
            case ANY_TAG_PTR:       ret = ui_hash_ptr(it->_ptr);        break;
            default:                assert(false);                      break;
        }

        ret = ui_id_combine(ret, it_id);
    }

    YO_PROFILE_END(ui_hash_args);
    return ret;
}

////////////////////////////////////////////////////////////////
// rune: Layout

static f32_range ui_align_on_single_axis(f32_range avail, f32 want, ui_axis_align align) {

    static readonly f32 offset_table[] = {
        [UI_AXIS_ALIGN_STRETCH] = 0.0f,
        [UI_AXIS_ALIGN_CENTER]  = 0.5f,
        [UI_AXIS_ALIGN_MIN]     = 0.0f,
        [UI_AXIS_ALIGN_MAX]     = 1.0f,
    };

    clamp_assign(&want, 0, avail.max - avail.min);

    f32 dim = align == UI_AXIS_ALIGN_STRETCH ? avail.max - avail.min : want;
    f32 excess = (avail.max - avail.min - dim);
    assert_bounds(align, countof(offset_table));
    f32 offset = excess * offset_table[align];

    f32_range ret = { 0 };
    ret.min = avail.min + offset;
    ret.max = avail.min + offset + dim;
    return ret;
}

static rect ui_align_rect(rect avail, vec2 pref_dim, ui_align align) {
    assert(avail.x0 <= avail.x1);
    assert(avail.y0 <= avail.y1);
    f32_range range_x = { avail.x0, avail.x1 };
    f32_range range_y = { avail.y0, avail.y1 };
    f32_range aligned_x = ui_align_on_single_axis(range_x, pref_dim.x, align.x);
    f32_range aligned_y = ui_align_on_single_axis(range_y, pref_dim.y, align.y);
    rect ret = {
        .x0 = aligned_x.min,
        .x1 = aligned_x.max,
        .y0 = aligned_y.min,
        .y1 = aligned_y.max,
    };
    return ret;
}

static rect ui_place_rect_no_auto_spacing(vec2 pref_dim) {
    ui_ctx *ctx = global_ui_ctx;
    ui_pushed_layout *layout = &ctx->layout_stack.top;
    rect allocated = { 0 };
    switch (layout->v.cut) {
        case UI_CUT_LEFT: { allocated = ui_cut_left(&layout->v.rect, pref_dim.x); } break;
        case UI_CUT_RIGHT: { allocated = ui_cut_right(&layout->v.rect, pref_dim.x); } break;
        case UI_CUT_TOP: { allocated = ui_cut_top(&layout->v.rect, pref_dim.y); } break;
        case UI_CUT_BOT: { allocated = ui_cut_bot(&layout->v.rect, pref_dim.y); } break;
        case UI_CUT_NONE: { allocated = layout->v.rect; } break;
        default: assert(!"Invalid code path"); break;
    }
    rect ret = ui_align_rect(allocated, pref_dim, layout->v.align);
    return ret;
}

static rect ui_place_rect(vec2 pref_dim) {
    YO_PROFILE_BEGIN(ui_place_rect);
    ui_ctx *ctx = global_ui_ctx;
    ui_pushed_layout *top = &ctx->layout_stack.top;
    f32 spacing = top->v.spacing;
    if (spacing && top->next_auto_spacing) {
        ui_place_rect_no_auto_spacing(vec2(spacing, spacing));
    }
    top->next_auto_spacing = true;
    rect ret = ui_place_rect_no_auto_spacing(pref_dim);
    YO_PROFILE_END(ui_place_rect);
    return ret;
}

static rect ui_place_rect_x(f32 x) { return ui_place_rect(vec2(x, F32_MAX)); }
static rect ui_place_rect_y(f32 y) { return ui_place_rect(vec2(F32_MAX, y)); }
static rect ui_place_rect_xy(f32 x, f32 y) { return ui_place_rect(vec2(x, y)); }

static vec2 ui_measure_border(ui_border border, vec2 content_dim) {
    vec2 ret = { 0 };
    vec2_add_assign(&ret, content_dim);
    vec2_add_assign(&ret, border.thickness.p0);
    vec2_add_assign(&ret, border.thickness.p1);
    return ret;
}

static rect ui_place_border(ui_border border, vec2 content_pref_dim) {
    vec2 pref_dim = ui_measure_border(border, content_pref_dim);
    rect ret = ui_place_rect(pref_dim);
    return ret;
}

static rect ui_place_spacing(f32 amount) {
    ui_ctx *ctx = global_ui_ctx;
    ui_pushed_layout *top = &ctx->layout_stack.top;
    top->next_auto_spacing = false; // NOTE(rune): If spacing was placed manually it overrides auto spacing.
    rect r = ui_place_rect_no_auto_spacing(vec2(amount, amount));
    return r;
}

static ui_sides_f32 ui_all_sides(f32 v) {
    ui_sides_f32 ret = {
        .v[0] = v,
        .v[1] = v,
        .v[2] = v,
        .v[3] = v,
    };
    return ret;
}

static ui_layout ui_make_layout(rect area, ui_cut cut, ui_align align, f32 spacing) {
    ui_layout ret = {
        .rect = area,
        .cut = cut,
        .align = align,
        .spacing = spacing,
    };
    return ret;
}

static ui_layout ui_make_layout_rect(rect area) { return ui_make_layout(area, UI_CUT_NONE, UI_ALIGN_NONE, 0); }
static ui_layout ui_make_layout_stack(rect area, ui_align align) { return ui_make_layout(area, UI_CUT_NONE, align, 0); }
static ui_layout ui_make_layout_x(rect area, ui_align align, f32 spacing) { return ui_make_layout(area, UI_CUT_LEFT, align, spacing); }
static ui_layout ui_make_layout_y(rect area, ui_align align, f32 spacing) { return ui_make_layout(area, UI_CUT_TOP, align, spacing); }
static ui_layout ui_make_layout_a(rect area, ui_align align, f32 spacing, axis2 axis) { return axis == AXIS2_X ? ui_make_layout_x(area, align, spacing) : ui_make_layout_y(area, align, spacing); }

static void ui_set_layout_rect(rect area) { ui_set_layout(ui_make_layout_rect(area)); }
static void ui_set_layout_stack(rect area, ui_align align) { ui_set_layout(ui_make_layout_stack(area, align)); }
static void ui_set_layout_x(rect area, ui_align align, f32 spacing) { ui_set_layout(ui_make_layout_x(area, align, spacing)); }
static void ui_set_layout_y(rect area, ui_align align, f32 spacing) { ui_set_layout(ui_make_layout_y(area, align, spacing)); }
static void ui_set_layout_a(rect area, ui_align align, f32 spacing, axis2 axis) { ui_set_layout(ui_make_layout_a(area, align, spacing, axis)); }

static ui_border ui_make_border(f32 thickness, u32 color, f32 radius) {
    ui_border ret = {
        .thickness.v[0] = thickness,
        .thickness.v[1] = thickness,
        .thickness.v[2] = thickness,
        .thickness.v[3] = thickness,
        .radius.v[0] = radius,
        .radius.v[1] = radius,
        .radius.v[2] = radius,
        .radius.v[3] = radius,
        .color = color,
    };
    return ret;
}

////////////////////////////////////////////////////////////////
// rune: Units

static f32 ui_px(f32 v) { return v; } // @Todo DPI scaling.
static f32 ui_dpx(f32 v) { return v; }
static f32 ui_em(f32 v) { return f32_round(v * global_ui_ctx->current_lineheight + 0.5f); }

////////////////////////////////////////////////////////////////
// rune: Widget

static void ui_free_ctx(ui_ctx ctx) {
    arena_destroy(ctx.this_frame.arena);
    arena_destroy(ctx.prev_frame.arena);
}

static void ui_select_ctx(ui_ctx *ctx) {
    global_ui_ctx = ctx;
}

static void ui_invalidate_next_frame(void) {
    global_ui_ctx->invalidate_next_frame = true;
}

// @Cleanup Declaration order.
static ui_box *ui_create_box(ui_id id);
static bool ui_mouse_button_went_down(ui_mouse_buttons button);
static bool ui_mouse_button_went_up(ui_mouse_buttons button);

static void ui_next_frame(ui_input *input) {
    ui_ctx *ctx = global_ui_ctx;
    ctx->frame_idx++;

    // First time initialization.
    if (!ctx->op_data_buf.v) {
        u64 op_def_size  = kilobytes(64);
        u64 op_data_size = kilobytes(256);

        ctx->op_def_buf  = make_buf(heap_alloc(op_def_size), op_def_size, 0);
        ctx->op_data_buf = make_buf(heap_alloc(op_data_size), op_data_size, 0);

        ctx->this_frame.arena = arena_create_default();
        ctx->prev_frame.arena = arena_create_default();
    }

    // Clear ui ops.
    memset(ctx->op_def_buf.v, 0, ctx->op_def_buf.len);
    memset(ctx->op_data_buf.v, 0, ctx->op_data_buf.len);
    ctx->op_def_buf.len = 0;
    ctx->op_data_buf.len = 0;

    // Clear command list.
    ctx->cmds.first = null;
    ctx->cmds.last = null;

    // Swap frame state.
    swap(ui_frame, &ctx->prev_frame, &ctx->this_frame);

    // Setup frame state for upcoming frame.
    arena_reset(ctx->this_frame.arena);
    ctx->this_frame.active_id = ctx->prev_frame.active_id;
    ctx->this_frame.hot_id = 0;
    ctx->this_frame.input = *input;
    ctx->this_frame.map.slot_count = 128;
    ctx->this_frame.map.slots = arena_push_array(ui_arena(), ui_box *, ctx->this_frame.map.slot_count);
    ctx->invalidate_next_frame = false;

    // Mouse buttons.
    ui_input *this_input = &ctx->this_frame.input;
    ui_input *prev_input = &ctx->prev_frame.input;
    ctx->this_frame.is_down_mouse_buttons   =  this_input->mouse_buttons;
    ctx->this_frame.went_down_mouse_buttons =  this_input->mouse_buttons & ~prev_input->mouse_buttons;
    ctx->this_frame.went_up_mouse_buttons   = ~this_input->mouse_buttons &  prev_input->mouse_buttons;

    // Clear stacks.
    ui_stack_init(ui_face, &ctx->face_stack, ui_arena(), (ui_face) { 0 });
    ui_stack_init(u32, &ctx->foreground_stack, ui_arena(), rgb(255, 0, 255));
    ui_stack_init(u32, &ctx->background_stack, ui_arena(), rgb(255, 0, 255));
    ui_stack_init(b8, &ctx->pixel_snap_stack, ui_arena(), true);
    ui_stack_init(ui_pushed_layout, &ctx->layout_stack, ui_arena(), (ui_pushed_layout) { .v.rect = input->client_rect });
    ui_stack_init(rect, &ctx->scissor_stack, ui_arena(), rect_make(VEC2_MIN, VEC2_MAX));
    ui_stack_init(vec2, &ctx->transform_stack, ui_arena(), VEC2_ZERO);

    // Update hot id
    ctx->this_frame.hot_id = 0;
    ctx->this_frame.hot_id_interactions = 0;

    // Update active id
    ctx->this_frame.active_id              = ctx->prev_frame.active_id;
    ctx->this_frame.active_id_interactions = ctx->prev_frame.active_id_interactions;
    if (ctx->this_frame.active_id != 0) {
        if (ctx->this_frame.active_id_interactions & UI_INTERACTION_ACTIVATE_HOLD) {
            if (ui_mouse_button_went_down(UI_MOUSE_BUTTON_LEFT) ||
                ui_mouse_button_went_up(UI_MOUSE_BUTTON_LEFT)) {
                ctx->this_frame.active_id = 0;
                ctx->this_frame.active_id_interactions = 0;
            }
        }

        if (ctx->this_frame.active_id_interactions & UI_INTERACTION_ACTIVATE_CLICK) {
            if (ui_mouse_button_went_down(UI_MOUSE_BUTTON_LEFT)) {
                ctx->this_frame.active_id = 0;
                ctx->this_frame.active_id_interactions = 0;
            }
        }
    }

    // Update focus id.
    ctx->this_frame.focused_id = ctx->prev_frame.focused_id;

    // Setup box tree
    zero_struct(&ctx->root_box);
    ctx->root_box.id = ui_id_root();
    ui_stack_init(ui_box_ptr, &ctx->box_parent_stack, ui_arena(), &ctx->root_box);

    // Setup focus tree.
    ctx->current_focus_group = &ctx->root_box;
}

static ui_events ui_get_events(void); // @UiApi // @Cleanup

static ui_box *ui_map_get(ui_map *map, ui_id id); // @Cleanup

static void ui_end_frame(void) {
    YO_PROFILE_BEGIN(ui_end_frame);

    ui_ctx *ctx = global_ui_ctx;
    ui_box *focus = ui_map_get(&global_ui_ctx->this_frame.map, global_ui_ctx->this_frame.focused_id);
    unused(focus);

    // rune: Hash table lookups.
    ui_box *this_hot_box = ui_map_get(&ctx->this_frame.map, ctx->this_frame.hot_id);
    ui_box *this_active_box = ui_map_get(&ctx->this_frame.map, ctx->this_frame.active_id);
    ui_box *this_focused_box = ui_map_get(&ctx->this_frame.map, ctx->this_frame.focused_id);

    // rune: Clear ids that haven't been used this frame.
    if (!this_hot_box)      ctx->this_frame.hot_id = 0;
    if (!this_active_box)   ctx->this_frame.active_id = 0;
    if (!this_focused_box)  ctx->this_frame.focused_id = 0;

    for_n (u64, i, ctx->this_frame.map.slot_count) {
        for (ui_box *box = ctx->this_frame.map.slots[i]; box; box = box->next_hash) {
            bool is_hot     = box->id == ctx->this_frame.hot_id;
            bool is_active  = box->id == ctx->this_frame.active_id;
            bool is_focused = box->id == ctx->this_frame.focused_id;

            if (!is_hot)        clamp01_assign(&box->hot_t);
            if (!is_active)     clamp01_assign(&box->active_t);
            if (!is_focused)    clamp01_assign(&box->focused_t);

            box->hot_t          += (is_hot ? 1.0f : -1.0f)      * ui_dt() * 10;
            box->active_t       += (is_active ? 1.0f : -1.0f)   * ui_dt() * 10;
            box->focused_t      += (is_focused ? 1.0f : -1.0f)  * ui_dt() * 10;
        }
    }

    YO_PROFILE_END(ui_end_frame);
}

static f32 global_deltatime;
static i64 global_tick;

static f32 ui_deltatime(void) {
    return global_deltatime;
}

static f32 ui_dt(void) {
    return ui_deltatime();
}

static i64 ui_tick(void) {
    return global_tick;
}

static arena *ui_arena(void) {
    return global_ui_ctx->this_frame.arena;
}

#define ui_fmt(...) ui_fmt_args(argsof(__VA_ARGS__))

static str ui_fmt_args(args args) {
    return arena_print_args(ui_arena(), args);
}

static void ui_map_add(ui_map *map, ui_box *w) {
    u64 slot_idx = w->id % map->slot_count;
    SLSTACK_PUSH(map->slots[slot_idx], next_hash, w);
}

static ui_box *ui_map_get(ui_map *map, ui_id id) {
    YO_PROFILE_BEGIN(ui_map_get);
    ui_box *ret = null;

    if (id && map->slot_count) {
        u64 slot_idx = id % map->slot_count;
        ui_box *slot = map->slots[slot_idx];
        for (ui_box *it = slot; it; it = it->next_hash) {
            if (it->id == id) {
                ret = it;
                break;
            }
        }
    }

    YO_PROFILE_END(ui_map_get);
    return ret;
}

static ui_box *ui_get_box_by_id(ui_id id) {
    ui_box *ret = ui_map_get(&global_ui_ctx->this_frame.map, id);
    assert(!ret || ret->id == id);
    return ret;
}

static ui_box *ui_get_widget_from_prev_frame(ui_id id) {
    ui_box *ret = ui_map_get(&global_ui_ctx->prev_frame.map, id);
    assert(!ret || ret->id == id);
    return ret;
}

static rect ui_get_widget_hitbox(ui_id id) {
    ui_box *from_prev_frame = ui_get_widget_from_prev_frame(id);
    rect hitbox = rect_make(VEC2_MIN, VEC2_MIN);
    if (from_prev_frame) {
        hitbox = from_prev_frame->clipped_screen_rect;
    }

    return hitbox;
}

static ui_box * ui_get_selected_box(void); // @Cleanup

static void ui_interact(ui_id id, ui_interactions flags, rect hitbox) {
    YO_PROFILE_BEGIN(ui_interact);

    ui_ctx *ctx = global_ui_ctx;
    ui_frame *frame = &ctx->this_frame;

    if (id != 0) {
        if (flags & UI_INTERACTION_HOT_HOVER) {
            if (rect_contains(hitbox, frame->input.mouse_pos)) {
                if (!(frame->active_id_interactions & UI_INTERACTION_ACTIVATE_HOLD)) {
                    frame->hot_id = id;
                    frame->hot_id_interactions = flags;
                }
            }
        }

        if (flags & (UI_INTERACTION_ACTIVATE_CLICK|UI_INTERACTION_ACTIVATE_HOLD)) {
            for_array (ui_event, e, frame->input.events) {
                if (e->type == UI_EVENT_TYPE_CLICK &&
                    e->mousebutton == UI_MOUSE_BUTTON_LEFT) {
                    if (rect_contains(hitbox, e->pos)) {
                        frame->active_id = id;
                        frame->active_id_interactions = flags;
                        break;
                    }
                }
            }
        }
    }

    YO_PROFILE_END(ui_interact);
}

#define ui_get_data(T) ((T *)ui_get_userdata_(sizeof(T), null))

static void *ui_get_userdata_(u64 size, void *initial_value) {
    ui_box *w = ui_get_selected_box();
    if (!w->user_data) {
        if (initial_value) {
            w->user_data      = arena_copy_size(ui_arena(), initial_value, size, 0);
            w->user_data_size = size;
        } else {
            w->user_data      = arena_push_size(ui_arena(), size, 0);
            w->user_data_size = size;
        }
    } else {
        assert(w->user_data_size == size && "Widget data size does not match size from previous frame.");
    }
    return w->user_data;
}

static bool ui_widget_was_active(void) {
    ui_ctx *ctx = global_ui_ctx;
    ui_box *w = ui_get_selected_box();
    bool ret = (ctx->prev_frame.active_id == w->id);
    return ret;
}

static bool ui_widget_was_hot(void) {
    ui_ctx *ctx = global_ui_ctx;
    ui_box *w = ui_get_selected_box();
    bool ret = (ctx->prev_frame.hot_id == w->id);
    return ret;
}

static bool ui_widget_is_active(void) {
    ui_ctx *ctx = global_ui_ctx;
    ui_box *w = ui_get_selected_box();
    bool ret = (ctx->this_frame.active_id == w->id);
    return ret;
}

static bool ui_widget_is_hot(void) {
    ui_ctx *ctx = global_ui_ctx;
    ui_box *w = ui_get_selected_box();
    bool ret = (ctx->this_frame.hot_id == w->id);
    return ret;
}

static bool ui_id_is_active(ui_id id) { return (global_ui_ctx->this_frame.active_id == id); }
static bool ui_id_is_hot(ui_id id) { return (global_ui_ctx->this_frame.hot_id == id); }
static bool ui_id_was_active(ui_id id) { return (global_ui_ctx->prev_frame.active_id == id); }
static bool ui_id_was_hot(ui_id id) { return (global_ui_ctx->prev_frame.hot_id == id); }

static ui_id ui_get_hot_id(void) { return global_ui_ctx->this_frame.hot_id; }
static ui_id ui_get_active_id(void) { return global_ui_ctx->this_frame.active_id; }
static ui_id ui_get_focused_id(void) { return global_ui_ctx->this_frame.focused_id; }

static void ui_set_active_id(ui_id id) {
    ui_ctx *ctx = global_ui_ctx;
    ctx->this_frame.active_id = id;
}

static void ui_set_hot_id(ui_id id) {
    ui_ctx *ctx = global_ui_ctx;
    ctx->this_frame.hot_id = id;
}

static void ui_set_focused_id(ui_id id) {
    ui_ctx *ctx = global_ui_ctx;
    ctx->this_frame.focused_id = id;
}

////////////////////////////////////////////////////////////////
// rune: Mouse

static vec2 ui_mouse_pos(void) { // @UiApi
    ui_ctx *ctx = global_ui_ctx;
    vec2 pos = ctx->this_frame.input.mouse_pos;
    vec2 ret = vec2_sub(pos, ui_get_transform());
    return ret;
}

static bool ui_mouse_is_over(rect r) {
    vec2 mouse_pos = ui_mouse_pos();
    bool ret = rect_contains(r, mouse_pos);
    return ret;
}

static bool ui_mouse_button_is_down(ui_mouse_buttons button) {
    ui_ctx *ctx = global_ui_ctx;
    bool ret = (ctx->this_frame.is_down_mouse_buttons & button) != 0;
    return ret;
}

static bool ui_mouse_button_went_down(ui_mouse_buttons button) {
    ui_ctx *ctx = global_ui_ctx;
    bool ret = (ctx->this_frame.went_down_mouse_buttons & button) != 0;
    return ret;
}

static bool ui_mouse_button_went_up(ui_mouse_buttons button) {
    ui_ctx *ctx = global_ui_ctx;
    bool ret = (ctx->this_frame.went_up_mouse_buttons & button) != 0;
    return ret;
}

static ui_events ui_get_events(void) { // @UiApi
    ui_ctx *ctx = global_ui_ctx;
    return ctx->this_frame.input.events;
}

static bool ui_clicked_on_rect(rect r) { // @UiApi
    rect_offset_assign(&r, ui_get_transform());

    bool ret = false;
    ui_events events = ui_get_events();
    for_array (ui_event, e, events) {
        if (e->type == UI_EVENT_TYPE_CLICK) {
            if (rect_contains(r, e->pos)) {
                ret = true;
                break;
            }
        }
    }
    return ret;
}

////////////////////////////////////////////////////////////////
// rune: Commands

#define ui_push_cmd(tag, T) ((T*)ui_push_cmd_and_data(tag, sizeof(T)))

static void *ui_push_cmd_and_data(i32 tag, u64 data_size) {
    ui_cmd *cmd = arena_push_struct(ui_arena(), ui_cmd);
    void *data = arena_push_size(ui_arena(), data_size, 0);
    cmd->tag = tag;
    cmd->data = data;
    slist_push(&global_ui_ctx->cmds, cmd);
    global_ui_ctx->cmds.count++;
    return data;
}

static ui_cmds ui_get_cmds(void) {
    return global_ui_ctx->cmds;
}

static void ui_clear_cmds(void) {
    global_ui_ctx->cmds.first = null;
    global_ui_ctx->cmds.last = null;
    global_ui_ctx->cmds.count = 0;
}

////////////////////////////////////////////////////////////////
// rune: Animation

static void ui_anim_f32_base(f32 *value, f32 target, f32 rate, bool started) {
    const f32 epsilon = 0.01f;

    if (started) {
        f32 origin = *value;
        f32 next = target;
        next = origin + (target - origin) * rate * ui_dt();
        next = clamp(next, min(origin, target), max(origin, target));

        if (f32_abs(origin - target) < epsilon) {
            next = target;
        } else {
            ui_invalidate_next_frame();
        }

        *value = next;
    } else {
        *value = target;
    }
}

static f32 ui_anim_f32(ui_animated_f32 *anim, f32 target, f32 rate) {
    ui_anim_f32_base(&anim->pos, target, rate, anim->started);
    anim->started = true;
    return anim->pos;
}

static vec2 ui_anim_vec2(ui_animated_vec2 *anim, vec2 target, f32 rate) {
    ui_anim_f32_base(&anim->pos.x, target.x, rate, anim->started);
    ui_anim_f32_base(&anim->pos.y, target.y, rate, anim->started);
    anim->started = true;
    return anim->pos;
}

static vec3 ui_anim_vec3(ui_animated_vec3 *anim, vec3 target, f32 rate) {
    ui_anim_f32_base(&anim->pos.x, target.x, rate, anim->started);
    ui_anim_f32_base(&anim->pos.y, target.y, rate, anim->started);
    ui_anim_f32_base(&anim->pos.z, target.z, rate, anim->started);
    anim->started = true;
    return anim->pos;
}

static vec4 ui_anim_vec4(ui_animated_vec4 *anim, vec4 target, f32 rate) {
    ui_anim_f32_base(&anim->pos.x, target.x, rate, anim->started);
    ui_anim_f32_base(&anim->pos.y, target.y, rate, anim->started);
    ui_anim_f32_base(&anim->pos.z, target.z, rate, anim->started);
    ui_anim_f32_base(&anim->pos.w, target.w, rate, anim->started);
    anim->started = true;
    return anim->pos;
}

static rect ui_anim_rect(ui_animated_rect *anim, rect target, f32 rate) {
    ui_anim_f32_base(&anim->pos.x0, target.x0, rate, anim->started);
    ui_anim_f32_base(&anim->pos.y1, target.y1, rate, anim->started);
    ui_anim_f32_base(&anim->pos.x0, target.x0, rate, anim->started);
    ui_anim_f32_base(&anim->pos.y1, target.y1, rate, anim->started);
    anim->started = true;
    return anim->pos;
}

static void ui_anim_damped_base(f32 *pos, f32 *vel, f32 target, f32 rate, f32 epsilon, bool started) {
    assert(!f32_is_nan(*pos));
    assert(!f32_is_nan(*vel));

    if (started) {
        *pos = f32_smooth_damp(*pos, target, vel, 1 / rate, F32_MAX, ui_deltatime());
        if (f32_abs(*pos - target) < epsilon) {
            *pos = target;
        } else {
            ui_invalidate_next_frame();
        }
    } else {
        *pos = target;
    }

    assert(!f32_is_nan(*pos));
    assert(!f32_is_nan(*vel));
}

static f32 ui_anim_damped_f32(ui_animated_f32 *anim, f32 target, f32 rate, f32 epsilon) {
    ui_anim_damped_base(&anim->pos, &anim->vel, target, rate, epsilon, anim->started);
    anim->started = true;
    anim->completed = anim->pos == target;
    return anim->pos;
}

static void ui_reset_damped_f32(ui_animated_f32 *anim) {
    anim->started = false;
}

static vec2 ui_anim_damped_vec2(ui_animated_vec2 *anim, vec2 target, f32 rate, f32 epsilon) {
    ui_anim_damped_base(&anim->pos.x, &anim->vel.x, target.x, rate, epsilon, anim->started);
    ui_anim_damped_base(&anim->pos.y, &anim->vel.y, target.y, rate, epsilon, anim->started);
    anim->started = true;
    anim->completed = vec2_eq(anim->pos, target);
    return anim->pos;
}

static void ui_reset_damped_vec2(ui_animated_vec2 *anim) {
    anim->started = false;
}

static vec4 ui_anim_damped_vec4(ui_animated_vec4 *anim, vec4 target, f32 rate, f32 epsilon) {
    ui_anim_damped_base(&anim->pos.x, &anim->vel.x, target.x, rate, epsilon, anim->started);
    ui_anim_damped_base(&anim->pos.y, &anim->vel.y, target.y, rate, epsilon, anim->started);
    ui_anim_damped_base(&anim->pos.z, &anim->vel.z, target.z, rate, epsilon, anim->started);
    ui_anim_damped_base(&anim->pos.w, &anim->vel.w, target.w, rate, epsilon, anim->started);
    anim->started = true;
    anim->completed = vec4_eq(anim->pos, target);
    return anim->pos;
}

static rect ui_anim_damped_rect(ui_animated_rect *anim, rect target, f32 rate, f32 epsilon) {
    ui_anim_damped_base(&anim->pos.x0, &anim->vel.x0, target.x0, rate, epsilon, anim->started);
    ui_anim_damped_base(&anim->pos.y0, &anim->vel.y0, target.y0, rate, epsilon, anim->started);
    ui_anim_damped_base(&anim->pos.x1, &anim->vel.x1, target.x1, rate, epsilon, anim->started);
    ui_anim_damped_base(&anim->pos.y1, &anim->vel.y1, target.y1, rate, epsilon, anim->started);
    anim->started = true;
    anim->completed = rect_eq(anim->pos, target);
    return anim->pos;
}

static u32 ui_anim_damped_rgba(ui_animated_vec4 *anim, u32 target, f32 rate) {
    vec4 before = unpack_rgba(target);
    vec4 after = ui_anim_damped_vec4(anim, before, rate, 0.01f);
    u32 ret = pack_rgba(after);
    return ret;
}

static void ui_reset_damped_vec4(ui_animated_vec4 *anim) {
    anim->started = false;
}

static u32 ui_anim_lerp_rgba(u32 a, u32 b, f32 t) {
    clamp01_assign(&t);
    if (t < 1) {
        ui_invalidate_next_frame();
    }

    vec4 av = unpack_rgba(a);
    vec4 bv = unpack_rgba(b);
    vec4 cv = vec4_lerp(av, bv, t);
    u32 c = pack_rgba(cv);
    return c;
}

static u32 ui_anim_lerp_alpha(u32 color, f32 t) {
    clamp01_assign(&t);
    if (t < 1) {
        ui_invalidate_next_frame();
    }

    u32 a = (u32)(t * 255.0f);
    u32 ret = (color & 0x00ffffff) | (a << 24);
    return ret;
}

////////////////////////////////////////////////////////////////
// rune: Keyboard focus

static void ui_focus_group_begin(void) {
    ui_ctx *ctx = global_ui_ctx;


    ui_box *parent = ctx->current_focus_group;
    ui_box *child = ctx->selected_box;

    child->focus_parent = parent;
    DLIST_PUSH_BACK(parent->focus_children.first,
                    parent->focus_children.last,
                    focus_next, focus_prev,
                    child);

    ctx->current_focus_group = child;
}

static void ui_focus_group_end(void) {
    ui_ctx *ctx = global_ui_ctx;
    assert(ctx->current_focus_group->focus_parent != null);
    ctx->current_focus_group = ctx->current_focus_group->focus_parent;
}

////////////////////////////////////////////////////////////////
// rune: Box

// TODO(rune): This depencecy can be removed, once the UI rewrite is complete
#include "../edith_ui_box.c"
#include "../edith_ui_box_widgets.c"
