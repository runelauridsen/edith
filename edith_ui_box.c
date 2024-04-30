////////////////////////////////////////////////////////////////
// rune: Box

static void ui_children_begin(void) {
    ui_ctx *ctx = global_ui_ctx;
    ui_box *parent = ui_get_selected_box();
    ui_stack_push(ui_box_ptr, &ctx->box_parent_stack);
    ui_stack_set(ui_box_ptr, &ctx->box_parent_stack, parent);
}

static void ui_children_end(void) {
    ui_ctx *ctx = global_ui_ctx;
    ctx->selected_box = ui_stack_get(ui_box_ptr, &ctx->box_parent_stack);
    ui_stack_pop(ui_box_ptr, &ctx->box_parent_stack);
}

static ui_box *ui_get_selected_box(void) {
    ui_ctx *ctx = global_ui_ctx;
    ui_box *box = ctx->selected_box;
    return box;
}

static void ui_set_selected_box(ui_box *box) {
    ui_ctx *ctx = global_ui_ctx;
    ctx->selected_box = box;
}

static ui_box *ui_get_parent(void) {
    ui_ctx *ctx = global_ui_ctx;
    ui_box *parent = ui_stack_get(ui_box_ptr, &ctx->box_parent_stack);
    return parent;
}

static ui_box *ui_new_box(ui_id id) {
    ui_ctx *ctx = global_ui_ctx;
    ui_box *box = arena_push_struct(ui_arena(), ui_box); // @Todo Error error handling.
    box->id = id;

    // Add tree links.
    ui_box *parent = ui_get_parent();
    box->parent = parent;
    dlist_push(&parent->children, box);

    // Setup default values.
    box->dim[AXIS2_X].max = F32_MAX;
    box->dim[AXIS2_Y].max = F32_MAX;
    box->dim[AXIS2_X].rel = UI_LEN_NOT_REL;
    box->dim[AXIS2_Y].rel = UI_LEN_NOT_REL;
    box->text_color = rgb(200, 200, 200);
    box->face = ui_get_face(); // @Cleanup

    // Select.
    ctx->selected_box = box;

    // Add to hash table.
    ui_map *map = &ctx->this_frame.map;
    ui_box *with_existing_id = ui_map_get(map, id);
    assert(with_existing_id == null);
    ui_map_add(map, box);

    // Copy data from previous frame.
    ui_box *from_prev_frame = ui_map_get(&ctx->prev_frame.map, id);
    if (from_prev_frame) {
        if (from_prev_frame->user_data) {
            box->user_data      = arena_copy_size(ui_arena(), from_prev_frame->user_data, from_prev_frame->user_data_size, 0);
            box->user_data_size = from_prev_frame->user_data_size;
        }

        box->scroll          = from_prev_frame->scroll;
        box->animated_scroll = from_prev_frame->animated_scroll;
        box->hot_t           = clamp01(from_prev_frame->hot_t);
        box->active_t        = clamp01(from_prev_frame->active_t);
        box->focused_t       = clamp01(from_prev_frame->focused_t);
    }

    return box;
}

static void ui_calc_layout_dim(ui_box *root, axis2 axis) {
    f32 child_sum = 0.0f;
    f32 child_max = 0.0f;

    // NOTE(rune): Updward dependent -> visit children first.
    for_list (ui_box, child, root->children) {
        ui_calc_layout_dim(child, axis);
        child_sum += child->calc_child_sum.v[axis];
        child_max = max(child_max, child->calc_child_sum.v[axis]);
    }

    f32 padding = (root->padding.p0.v[axis] +
                   root->padding.p1.v[axis] +
                   root->draw.border.thickness.p0.v[axis] +
                   root->draw.border.thickness.p1.v[axis]);

    child_sum += padding;
    child_max += padding;

    ui_len len = root->dim[axis];
    if (len.min == UI_LEN_TEXT || len.max == UI_LEN_TEXT) {

        assert(root->children.first == null);

        ui_set_face(root->face); // @Cleanup
        vec2 text_dim = ui_measure_text(root->text);
        text_dim.y = f32_round(text_dim.y + 0.5f);
        if (len.min == UI_LEN_TEXT) len.min = text_dim.v[axis] + padding;
        if (len.max == UI_LEN_TEXT) len.max = text_dim.v[axis] + padding;

        root->dim[axis] = len;
    }

    f32 clamped_sum = clamp(child_sum, len.min, len.max);
    f32 clamped_max = clamp(child_max, len.min, len.max);

    if (root->child_axis == axis && root->use_child_axis) {
        root->calc_child_sum.v[axis] = clamped_sum;
    } else {
        root->calc_child_sum.v[axis] = clamped_max;
    }
}

static void ui_calc_layout_pos(ui_box *root, axis2 axis, f32 avail) {
    avail = min(avail, root->calc_dim.v[axis]);

    f32 abs_child_sum = 0.0f;
    f32 rel_child_sum = 0.0f;
    for_list (ui_box, child, root->children) {
        ui_len len = child->dim[axis];
        if (len.rel == UI_LEN_NOT_REL) {
            abs_child_sum += child->calc_child_sum.v[axis];
        } else {
            rel_child_sum += len.rel;
        }
    }

    f32 padding_p0   = root->padding.p0.v[axis] + root->draw.border.thickness.p0.v[axis];
    f32 padding_p1   = root->padding.p1.v[axis] + root->draw.border.thickness.p1.v[axis];
    f32 padding      = padding_p0 + padding_p1;
    f32 padded_avail = avail - padding;

    if (root->child_axis == axis && root->use_child_axis) {

        ////////////////////////////////
        // Position along main axis.

        f32 at = 0.0f;
        for_list (ui_box, child, root->children) {
            if (child->allow_overflow) {
                __nop();
            }

            ui_len len = child->dim[axis];
            child->calc_pos.v[axis] = at;

            if (len.rel == UI_LEN_NOT_REL) {
                child->calc_dim.v[axis] = min(padded_avail, child->calc_child_sum.v[axis]);
            } else {
                child->calc_dim.v[axis] = (padded_avail - abs_child_sum) * (len.rel / rel_child_sum);
            }

            at += child->calc_dim.v[axis];

            // NOTE(rune): Downward dependent -> visit children after.
            ui_calc_layout_pos(child, axis, padded_avail);

        }
    } else {
        ////////////////////////////////
        // Position along cross axis.

        for_list (ui_box, child, root->children) {
            if (child->allow_overflow) {
                __nop();
            }

            ui_len len = child->dim[axis];

            if (len.rel == UI_LEN_NOT_REL) {
                child->calc_dim.v[axis] = child->calc_child_sum.v[axis];
            } else {
                child->calc_dim.v[axis] = padded_avail * len.rel;
            }

            ui_axis_align align = axis == AXIS2_X ? child->align_x : child->align_y;
            f32_range avail_for_align = f32_range(0, padded_avail);
            f32_range aligned = ui_align_on_single_axis(avail_for_align, child->calc_dim.v[axis], align);
            child->calc_pos.v[axis] = aligned.min;
            child->calc_dim.v[axis] = min(max(range_len(aligned), len.min), len.max);

            // NOTE(rune): Downward dependent -> visit children after.
            ui_calc_layout_pos(child, axis, padded_avail);
        }
    }
}

static void ui_calc_layout(ui_box *root, vec2 avail) {
    YO_PROFILE_BEGIN(ui_calc_layout_dim);
    ui_calc_layout_dim(root, AXIS2_X);
    ui_calc_layout_dim(root, AXIS2_Y);
    YO_PROFILE_END(ui_calc_layout_dim);

    YO_PROFILE_BEGIN(ui_calc_layout_pos);
    rect avail_rect = rect_make_dim(VEC2_ZERO, avail);
    rect aligned_rect = ui_align_rect(avail_rect, root->calc_child_sum, (ui_align) { root->align_x, root->align_y });
    vec2 aligned_dim = rect_dim(aligned_rect);
    root->calc_pos = aligned_rect.p0;
    root->calc_dim = aligned_dim;
    ui_calc_layout_pos(root, AXIS2_X, avail.x);
    ui_calc_layout_pos(root, AXIS2_Y, avail.y);
    YO_PROFILE_END(ui_calc_layout_pos);
}

static void ui_draw_boxes_recurse(ui_box *root, vec2 pos) {
    ui_set_face(root->face); // @Cleanup.

    vec2 screen_pos = vec2_add(pos, root->calc_pos);

    rect screen_rect = rect_make_dim(screen_pos, root->calc_dim);
    root->screen_rect = screen_rect;
    root->clipped_screen_rect = rect_intersect(screen_rect, ui_get_scissor());

    ////////////////////////////////////////////////
    // Shadow.

    if (root->shadow) {
        ui_draw_rounded_shadow(screen_rect, root->shadow, root->draw.border.radius.x0, 0);
    }

    ////////////////////////////////////////////////
    // Border.

    if (root->draw.has_border) {
        ui_draw_border(screen_rect, root->draw.border);
    }

    ////////////////////////////////////////////////
    // Padding.

    rect padded_screen_rect = screen_rect;
    vec2_add_assign(&padded_screen_rect.p0, root->draw.border.thickness.p0);
    vec2_sub_assign(&padded_screen_rect.p1, root->draw.border.thickness.p1);
    vec2_add_assign(&padded_screen_rect.p0, root->padding.p0);
    vec2_sub_assign(&padded_screen_rect.p1, root->padding.p1);

    ////////////////////////////////////////////////
    // Background.

    if (root->draw.has_background) {
        rect dst_rect = {
            .p0 = vec2_add(screen_rect.p0, root->draw.border.thickness.p0),
            .p1 = vec2_sub(screen_rect.p1, root->draw.border.thickness.p1),
        };
        ui_op_aabb_ex *op = ui_add_op_aabb_ex();
        op->dst_rect         = dst_rect;
        op->tex_rect         = root->draw.uv;
        op->tex_weight       = root->draw.tex_weight;
        op->softness         = root->draw.softness;
        op->corner_radius[0] = root->draw.corner_radius[0];
        op->corner_radius[1] = root->draw.corner_radius[1];
        op->corner_radius[2] = root->draw.corner_radius[2];
        op->corner_radius[3] = root->draw.corner_radius[3];
        op->color[0]         = root->draw.color[0];
        op->color[1]         = root->draw.color[1];
        op->color[2]         = root->draw.color[2];
        op->color[3]         = root->draw.color[3];
    }

    ////////////////////////////////////////////////
    // Text.

    if (root->text.len) {
        ui_set_face(root->face); // @Cleanup
        rect text_rect = padded_screen_rect;
        vec2 text_dim = ui_measure_text(root->text);
        switch (root->text_align) {
            case UI_TEXT_ALIGN_LEFT: ui_cut_aligned(&text_rect, text_dim, UI_ALIGN_LEFT_CENTER); break;
            case UI_TEXT_ALIGN_RIGHT: ui_cut_aligned(&text_rect, text_dim, UI_ALIGN_RIGHT_CENTER); break;
            case UI_TEXT_ALIGN_CENTER: ui_cut_aligned(&text_rect, text_dim, UI_ALIGN_CENTER); break;
            case UI_TEXT_ALIGN_JUSTIFY: assert(!"Not implemented"); break;
        }
        ui_draw_text_r(text_rect, root->text, root->text_color);
    }

    ////////////////////////////////////////////////
    // Callback.

    if (root->callback) {
        ui_callback_param param = {
            .screen_rect = screen_rect,
            .padded_screen_rect = padded_screen_rect,
            .root = root,
        };
        root->callback(&param, root->callback_userdata);
    }

    ////////////////////////////////////////////////
    // Children.

    if (root->child_clip) {
        ui_push_scissor();
        ui_set_scissor(padded_screen_rect, 0);
    }

    ui_anim_vec2(&root->animated_scroll, root->scroll, 20.0f);
    vec2 child_pos = vec2_sub(padded_screen_rect.p0, root->animated_scroll.pos);
    for_list (ui_box, child, root->children) {
        ui_draw_boxes_recurse(child, child_pos);
    }

    if (root->child_clip) {
        ui_pop_scissor();
    }
}

static vec2 ui_draw_boxes(ui_box *root, rect area, rect scissor) {
    YO_PROFILE_BEGIN(ui_calc_layout);
    ui_calc_layout(root, rect_dim(area));
    YO_PROFILE_END(ui_calc_layout);

    ////////////////////////////////////////////////
    // Draw.

    YO_PROFILE_BEGIN(ui_draw_boxes_recurse);
    ui_push_scissor();
    ui_set_scissor(scissor, 0);
    ui_draw_boxes_recurse(root, area.p0);
    vec2 used_dim = rect_dim(area);
    ui_pop_scissor();
    YO_PROFILE_END(ui_draw_boxes_recurse);

    return used_dim;
}

static inline void ui_set_dim_a(ui_len dim_a, axis2 a) { global_ui_ctx->selected_box->dim[a] = dim_a; }
static inline void ui_set_dim_a_min(f32 min, axis2 a) { global_ui_ctx->selected_box->dim[a].min = min; }
static inline void ui_set_dim_a_max(f32 max, axis2 a) { global_ui_ctx->selected_box->dim[a].max = max; }

static inline void ui_set_dim_x(ui_len dim_x) { ui_set_dim_a(dim_x, AXIS2_X); }
static inline void ui_set_dim_x_min(f32 min_x) { ui_set_dim_a_min(min_x, AXIS2_X); }
static inline void ui_set_dim_x_max(f32 max_x) { ui_set_dim_a_max(max_x, AXIS2_X); }

static inline void ui_set_dim_y(ui_len dim_y) { ui_set_dim_a(dim_y, AXIS2_Y); }
static inline void ui_set_dim_y_min(f32 min_y) { ui_set_dim_a_min(min_y, AXIS2_Y); }
static inline void ui_set_dim_y_max(f32 max_y) { ui_set_dim_a_max(max_y, AXIS2_Y); }

static inline void ui_set_dim(ui_len dim_x, ui_len dim_y) { ui_set_dim_x(dim_x); ui_set_dim_y(dim_y); }
static inline void ui_set_align(ui_align align) { global_ui_ctx->selected_box->align_x = align.x; global_ui_ctx->selected_box->align_y = align.y; }
static inline void ui_set_child_axis(axis2 child_axis) { global_ui_ctx->selected_box->child_axis = child_axis; global_ui_ctx->selected_box->use_child_axis = true; }
static inline void ui_set_child_clip(bool child_clip) { global_ui_ctx->selected_box->child_clip = child_clip; }
static inline void ui_set_text(str text) { global_ui_ctx->selected_box->text = text; }
static inline void ui_set_text_color(u32 text_color) { global_ui_ctx->selected_box->text_color = text_color; }
static inline void ui_set_text_align(ui_text_align text_align) { global_ui_ctx->selected_box->text_align = text_align; }
static inline void ui_set_face_b(ui_face face) { global_ui_ctx->selected_box->face = face; }

static inline void ui_set_padding(f32 amount) { global_ui_ctx->selected_box->padding = ui_all_sides(amount); }
static inline void ui_set_padding_x(f32 amount) { global_ui_ctx->selected_box->padding.x0 = amount; global_ui_ctx->selected_box->padding.x1 = amount; }
static inline void ui_set_padding_y(f32 amount) { global_ui_ctx->selected_box->padding.y0 = amount; global_ui_ctx->selected_box->padding.y1 = amount; }

static inline void ui_set_border(f32 thickness, u32 color, f32 radius) { global_ui_ctx->selected_box->draw.border = ui_make_border(thickness, color, radius); global_ui_ctx->selected_box->draw.has_border = true; }
static inline void ui_set_shadow(f32 shadow) { global_ui_ctx->selected_box->shadow = shadow; }

static inline void ui_set_scroll(vec2 scroll) { global_ui_ctx->selected_box->scroll = scroll; }
static inline void ui_set_overflow(bool allow) { global_ui_ctx->selected_box->allow_overflow = allow; }

static void ui_set_background(u32 bg) {
    global_ui_ctx->selected_box->draw.color[0] = bg;
    global_ui_ctx->selected_box->draw.color[1] = bg;
    global_ui_ctx->selected_box->draw.color[2] = bg;
    global_ui_ctx->selected_box->draw.color[3] = bg;
    global_ui_ctx->selected_box->draw.has_background = true;
}

static void ui_set_callback(ui_callback_fn *callback, void *userdata) {
    global_ui_ctx->selected_box->callback = callback;
    global_ui_ctx->selected_box->callback_userdata = userdata;
}
