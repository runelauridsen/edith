static ui_len ui_make_len(f32 min, f32 max, f32 rel) {
    ui_len ret = {
        .min = min,
        .max = max,
        .rel = rel,
    };
    return ret;
}

static ui_len ui_len_abs(f32 amount) { return ui_make_len(amount, amount, UI_LEN_NOT_REL); }
static ui_len ui_len_rel(f32 amount) { return ui_make_len(0, F32_MAX, amount); }
static ui_len ui_len_text(void) { return ui_make_len(UI_LEN_TEXT, UI_LEN_TEXT, UI_LEN_NOT_REL); }

#define ui_children() defer(ui_children_begin(), ui_children_end())

static void dump_box_tree(ui_box *box, u32 indent) {
    for_n (u32, i, indent) {
        print(" ");
    }

    println("box      \t calc_child_sum = (%, %) \t calc_dim = (%, %) \t calc_pos = (%, %) ",
            (i32)box->calc_child_sum.x, (i32)box->calc_child_sum.y,
            (i32)box->calc_dim.x, (i32)box->calc_dim.y,
            (i32)box->calc_pos.x, (i32)box->calc_pos.y);

    for_list (ui_box, child, box->children) {
        dump_box_tree(child, indent + 1);
    }
}

////////////////////////////////////////////////////////////////
// rune: Button

static bool ui_button_interaction(ui_id id, rect area) {
    ui_interact(id, UI_INTERACTION_ACTIVATE_HOLD | UI_INTERACTION_HOT_HOVER, area);
    bool was_clicked = (ui_widget_was_active() &&
                        ui_mouse_is_over(area) &&
                        ui_mouse_button_went_up(UI_MOUSE_BUTTON_LEFT));

    return was_clicked;
}

static bool ui_button_b(ui_id id, str text) {
    ui_new_box(id);
    ui_set_align(UI_ALIGN_LEFT);

    // Interaction.
    rect hitbox = ui_get_widget_hitbox(id);
    ui_interact(id, UI_INTERACTION_ACTIVATE_HOLD | UI_INTERACTION_HOT_HOVER, hitbox);
    bool was_clicked = (ui_widget_was_active() &&
                        ui_mouse_is_over(hitbox) &&
                        ui_mouse_button_went_up(UI_MOUSE_BUTTON_LEFT));

    u32 border_color = rgb(50, 50, 50);
    f32 border_thickness = 1;
    u32 fg = rgb(200, 200, 200);
    u32 bg = rgb(30, 30, 30);
    f32 corner_radius = 5;
    f32 padding = 5;

    unused(corner_radius, padding, fg);

    if (ui_widget_is_hot()) {
        bg = rgb(40, 40, 40);
        border_color = rgb(90, 90, 90);
    }

    if (ui_widget_is_active()) {
        bg = rgb(25, 25, 25);
        border_color = rgb(90, 90, 90);
    }

    ui_set_padding(border_thickness);

    ui_set_text(text);
    ui_set_padding(5);
    ui_set_dim_x_min(UI_LEN_TEXT);
    ui_set_dim_y_min(UI_LEN_TEXT);
    ui_set_background(bg);

    return was_clicked;
}

////////////////////////////////////////////////////////////////
// rune: Text

static void ui_text_b(str s) {
    ui_new_box(0);
    ui_set_text(s);
    ui_set_dim_x_min(UI_LEN_TEXT);
    ui_set_dim_y_min(UI_LEN_TEXT);
}

////////////////////////////////////////////////////////////////
// rune: Text field

static utf8_iter utf8_iter_by_advance_b(utf8_iter iter, f32 target_advance, ui_face face) {
    utf8_iter best_iter = iter;
    f32 best_dist = F32_MAX;
    f32 at = 0.0f;
    while (iter.valid) {
        f32 dist = f32_abs(target_advance - at);
        if (dist <= best_dist) {
            best_dist = dist;
            best_iter = iter;
        }

        if (dist > best_dist) {
            break;
        }

        f32 advance = ui_font_backend_get_advance(iter.codepoint, face);
        at += advance;
        iter = utf8_iter_next(iter, DIR_FORWARD);
    }

    f32 dist = f32_abs(target_advance - at);
    if (dist <= best_dist) {
        best_dist = dist;
        best_iter = iter;
    }

    return best_iter;
}

static bool ui_text_field_cursor_should_stop_b(u32 prev, u32 at, move_by by) {
    switch (by) {
        case MOVE_BY_CHAR: {
            return true;
        }

        case MOVE_BY_WORD: {
            if (u32_is_whitespace(prev) && !u32_is_whitespace(at)) {
                return true;
            }

            if (at == ':' || prev == ':') return true;
            if (at == ';' || prev == ';') return true;
            if (at == '(' || prev == '(') return true;
            if (at == ')' || prev == ')') return true;
            if (at == '[' || prev == '[') return true;
            if (at == ']' || prev == ']') return true;
            if (at == '}' || prev == '}') return true;
            if (at == '{' || prev == '{') return true;
            if (at == '/' || prev == '/') return true;
            if (at == '\\' || prev == '\\') return true;
        }
    }

    return false;
}

typedef struct ui_text_field_cursor_b ui_text_field_cursor_b;
struct ui_text_field_cursor_b {
    i64 caret;
    i64 mark;
};

typedef struct ui_text_edit_data ui_text_edit_data;
struct ui_text_edit_data {
    ui_text_field_cursor_b cursor;
    ui_animated_f32 animated_caret_x;
};

static ui_text_field_cursor_b ui_text_field_cursor_move_b(ui_text_field_cursor_b cursor, u8 *buf, i64 len, move_by by, dir dir, bool expand_sel) {
    u64_range sel = u64_range(cursor.caret, cursor.mark);
    str s = str_make(buf, len);

    switch (by) {
        case MOVE_BY_HOME_END: {
            if (dir == DIR_BACKWARD) cursor.caret = 0;
            if (dir == DIR_FORWARD) cursor.caret = len;
        } break;

        case MOVE_BY_CHAR: {
            // Moving while any range is selected moves caret to end of selected range.
            if (!expand_sel && range_len(sel)) {
                if (dir == DIR_BACKWARD) cursor.caret = sel.min;
                if (dir == DIR_FORWARD) cursor.caret = sel.max;
                break;
            }
        } // Fallthrough

        case MOVE_BY_WORD: {
            if (dir == DIR_FORWARD) {
                utf8_iter iter = utf8_iter_begin(s, cursor.caret);
                utf8_iter prev = { 0 };
                while (iter.valid) {
                    prev = iter;
                    iter = utf8_iter_next(iter, 1);
                    if (ui_text_field_cursor_should_stop_b(prev.codepoint, iter.codepoint, by)) {
                        break;
                    }
                }

                cursor.caret = iter.idx;
            }

            if (dir == DIR_BACKWARD) {
                utf8_iter iter = utf8_iter_begin(s, cursor.caret);
                utf8_iter prev = utf8_iter_next(iter, -1);
                while (prev.valid) {
                    iter = prev;
                    prev = utf8_iter_next(prev, -1);
                    if (ui_text_field_cursor_should_stop_b(prev.codepoint, iter.codepoint, by)) {
                        break;
                    }
                }

                cursor.caret = iter.idx;
            }
        }
    }

    if (!expand_sel) {
        cursor.mark = cursor.caret;
    }

    clamp_assign(&cursor.caret, 0, len);
    clamp_assign(&cursor.mark, 0, len);

    return cursor;
}

static bool ui_text_edit_interaction_b(rect r, ui_text_edit_data *data, u8 *buffer, i64 cap, i64 *len) {
    YO_PROFILE_BEGIN(ui_text_edit);

    buf buf = {
        .v = buffer,
        .len = *len,
        .cap = cap,
    };
    bool read_only = false;
    bool text_changed = false;
    if (ui_widget_is_active()) {
        for_array (ui_event, e, ui_get_events()) {
            switch (e->type) {
                case UI_EVENT_TYPE_CODEPOINT: {
                    if (!read_only && buf_remaining_size(buf) > 0) {
                        if (e->codepoint && e->codepoint != '\n' && e->codepoint != '\t') {
                            i64_range sel = i64_range(data->cursor.caret, data->cursor.mark);
                            u8 insert[4] = { 0 };
                            i32 insert_len = encode_single_utf8_codepoint(e->codepoint, insert);
                            buf_replace(&buf, sel, str_make(insert, insert_len));
                            data->cursor.caret = sel.min + insert_len;
                            data->cursor.mark = data->cursor.caret;
                            text_changed = true;
                        }
                    }
                } break;

                case UI_EVENT_TYPE_KEYPRESS: {
                    i64_range sel = i64_range(data->cursor.caret, data->cursor.mark);
                    bool handled = true;

                    dir delete_dir = 0;
                    dir move_dir = 0;
                    move_by move_by = 0;
                    ui_modifiers modifiers = e->modifiers;
                    bool expand_sel = (modifiers & UI_MODIFIER_SHIFT);
                    modifiers &= ~UI_MODIFIER_SHIFT;

                    if (modifiers == UI_MODIFIER_NONE) move_by = MOVE_BY_CHAR;
                    if (modifiers == UI_MODIFIER_CTRL) move_by = MOVE_BY_WORD;

                    switch (e->key) {
                        default: {
                            handled = false;
                        } break;

                        case UI_KEY_LEFT: {
                            move_dir = DIR_BACKWARD;
                        } break;

                        case UI_KEY_RIGHT: {
                            move_dir = DIR_FORWARD;
                        } break;

                        case UI_KEY_DELETE: {
                            delete_dir = DIR_FORWARD;
                            if (!range_len(sel)) {
                                move_dir = DIR_FORWARD;
                                expand_sel = true;
                            }
                        } break;

                        case UI_KEY_BACKSPACE: {
                            delete_dir = DIR_BACKWARD;
                            if (!range_len(sel)) {
                                move_dir = DIR_BACKWARD;
                                expand_sel = true;
                            }
                        } break;

                        case UI_KEY_HOME: {
                            move_dir = DIR_BACKWARD;
                            move_by = MOVE_BY_HOME_END;
                        } break;

                        case UI_KEY_END: {
                            move_dir = DIR_FORWARD;
                            move_by = MOVE_BY_HOME_END;
                        } break;

                        case UI_KEY_A: {
                            if (modifiers == UI_MODIFIER_CTRL) {
                                data->cursor.mark = 0;
                                data->cursor.caret = buf.count;
                            }
                        } break;
                    }

                    if (move_by && move_dir) {
                        data->cursor = ui_text_field_cursor_move_b(data->cursor, buf.v, buf.count, move_by, move_dir, expand_sel);
                        sel = i64_range(data->cursor.caret, data->cursor.mark);
                    }

                    if (delete_dir && !read_only) {
                        buf_delete(&buf, sel);
                        data->cursor.caret = sel.min;
                        data->cursor.mark  = sel.min;
                        text_changed = true;
                    }
                } break;

                case UI_EVENT_TYPE_CLICK: {
                    f32 target_x = ui_mouse_pos().x - r.x0;
                    str string = buf_as_str(buf);
                    utf8_iter iter = utf8_iter_begin(string, 0);
                    iter = utf8_iter_by_advance_b(iter, target_x, ui_get_face());
                    data->cursor.caret = iter.idx;
                    if (!(e->modifiers & UI_MODIFIER_SHIFT)) {
                        data->cursor.mark  = iter.idx;
                    }
                } break;
            }
        }
    }

    ////////////////////////////////////////////////////////////////
    // Mouse selection.

    if (ui_widget_is_active()) {
        if (ui_mouse_button_is_down(UI_MOUSE_BUTTON_LEFT)) {
            f32 target_x = ui_mouse_pos().x - r.x0;
            str string = buf_as_str(buf);
            utf8_iter iter = utf8_iter_begin(string, 0);
            iter = utf8_iter_by_advance_b(iter, target_x, ui_get_face());
            data->cursor.caret = iter.idx;
        }
    }

    clamp_assign(&data->cursor.caret, 0, buf.len);
    clamp_assign(&data->cursor.mark, 0, buf.len);

    *len = buf.len;

    YO_PROFILE_END(ui_text_edit);

    return text_changed;
}

typedef struct ui_text_field_draw_data_b ui_text_field_draw_data_b;
struct ui_text_field_draw_data_b {
    f32_range selection_x;
    f32 caret_x;
};

static void ui_text_field_draw_b(ui_callback_param *param, void *userdata) {
    ui_text_field_draw_data_b *data = userdata;
    rect area = param->padded_screen_rect;

    f32 caret_thickness = 2;
    u32 caret_color = rgb(100, 220, 200);
    u32 selection_color = rgba(200, 200, 200, 50);

    // Draw selection.
    rect selection_rect = { 0 };
    selection_rect.x0 = area.x0 + data->selection_x.min;
    selection_rect.x1 = area.x0 + data->selection_x.max;
    selection_rect.y0 = area.y0;
    selection_rect.y1 = area.y0 + ui_em(1);
    ui_draw_rect(selection_rect, selection_color);

    // Draw cursor.
    vec2 caret_p = vec2_add(area.p0, vec2(data->caret_x, 0.0f));
    ui_draw_caret(caret_p, caret_thickness, caret_color);
}

static bool ui_text_edit_b(ui_id id, u8 *buffer, i64 cap, i64 *len, ui_text_edit_data *data) {
    ui_new_box(id);
    ui_set_dim_y_min(UI_LEN_TEXT);
    ui_set_dim_x_min(UI_LEN_TEXT);

    ////////////////////////////////////////////////
    // Data.

    if (data == null) data = ui_get_data(ui_text_edit_data);

    ////////////////////////////////////////////////
    // Interaction.

    rect hitbox = ui_get_widget_hitbox(id);
    ui_interact(id, UI_INTERACTION_ACTIVATE_CLICK | UI_INTERACTION_HOT_HOVER, hitbox);

    bool text_changed = ui_text_edit_interaction_b(hitbox, data, buffer, cap, len);

    ui_set_text(str_make(buffer, *len));

    ////////////////////////////////////////////////
    // Custom draw.

    if (ui_id_is_active(id)) {
        f32_range selection_x = { 0 };
        f32 caret_x;
        {
            i64_range selection = i64_range(data->cursor.caret, data->cursor.mark);
            str s = str_make(buffer, *len);
            str_x3 tri = str_split_x3(s, selection.min, selection.max);
            selection_x.min = ui_measure_text(tri.v[0]).x;
            selection_x.max = ui_measure_text(tri.v[1]).x + selection_x.min;

            caret_x = data->cursor.caret < data->cursor.mark ? selection_x.min : selection_x.max;
            caret_x = ui_anim_f32(&data->animated_caret_x, caret_x, 30.0f);

            if (data->cursor.caret == selection.min) selection_x.min = caret_x;
            if (data->cursor.caret == selection.max) selection_x.max = caret_x;
        }

        ui_text_field_draw_data_b *draw_data = arena_push_struct(ui_arena(), ui_text_field_draw_data_b);
        draw_data->caret_x = caret_x;
        draw_data->selection_x = selection_x;
        ui_set_callback(ui_text_field_draw_b, draw_data);
    }

    return text_changed;
}

static void ui_field_frame_box(void) {
    ui_new_box(0);
    u32 field_border_color = rgb(40, 90, 90);
    u32 field_background_color = rgb(15, 15, 15);
    ui_set_padding(5);
    ui_set_border(1, field_border_color, 0);
    ui_set_background(field_background_color);
    ui_set_child_clip(true);
}

#define ui_field_frame()   defer((ui_field_frame_box(), ui_children_begin()), ui_children_end())

static bool ui_text_field_b(ui_id id, u8 *buffer, i64 cap, i64 *len, str ghost_text) {
    ////////////////////////////////////////////////
    // Interaction.

    bool text_changed;
    ui_field_frame() {
        text_changed = ui_text_edit_b(id, buffer, cap, len, null);
        if (*len == 0) {
            ui_text_b(ghost_text);
            ui_set_text_color(rgb(100, 100, 100));
            ui_set_padding_x(2); // NOTE(rune): Add a small amount of padding, so caret doesn't occlude ghost text.
        }
    }
    return text_changed;
}

////////////////////////////////////////////////////////////////
// rune: Spacers

static void ui_space_x(f32 amount) {
    ui_new_box(0);
    ui_set_dim_x_min(amount);
}

static void ui_space_y(f32 amount) {
    ui_new_box(0);
    ui_set_dim_y_min(amount);
}

////////////////////////////////////////////////////////////////
// rune: Layout

static void ui_layout_begin(axis2 axis) {
    ui_new_box(0);
    ui_set_child_axis(axis);
    ui_children_begin();
}

static void ui_layout_end(void) {
    ui_children_end();
}

static void ui_padding_begin(f32 x, f32 y) {
    ui_new_box(0);
    ui_set_padding_x(x);
    ui_set_padding_y(y);
    ui_children_begin();
}

#define ui_padding(x,y)     defer(ui_padding_begin(x, y), ui_children_end())
#define ui_layout_x()       defer(ui_layout_begin(AXIS2_X), ui_children_end())
#define ui_layout_y()       defer(ui_layout_begin(AXIS2_Y), ui_children_end())

////////////////////////////////////////////////////////////////
// rune: Scrolled

static void ui_scrolled_callback(ui_callback_param *param, void *userdata) {
    unused(userdata);

    ui_box *root = param->root;

    // Keyboard navigation.
    bool scroll_to_focused = false;
    ui_box *focus = ui_get_box_by_id(ui_get_focused_id());
    if (focus) {
        ui_events events = ui_get_events();
        for_array (ui_event, e, events) {
            if (ui_event_is_keypress_with_modifiers(e, UI_KEY_UP, UI_MODIFIER_NONE)) {
                scroll_to_focused = true;
                if (focus->focus_prev) {
                    focus = focus->focus_prev;
                } else {
                    focus = focus->focus_parent->focus_children.last;
                }
            }

            if (ui_event_is_keypress_with_modifiers(e, UI_KEY_DOWN, UI_MODIFIER_NONE)) {
                scroll_to_focused = true;
                if (focus->focus_next) {
                    focus = focus->focus_next;
                } else {
                    focus = focus->focus_parent->focus_children.first;
                }
            }

            if (ui_event_is_keypress_with_modifiers(e, UI_KEY_PAGE_UP, UI_MODIFIER_NONE)) {
                scroll_to_focused = true;
                f32 page_dim = rect_dim_y(focus->focus_parent->screen_rect);
                f32 orig_pos = focus->calc_pos.y;
                f32 goto_pos = orig_pos - page_dim;
                while (focus->calc_pos.y > goto_pos && focus->prev) {
                    focus = focus->prev;
                }
            }

            if (ui_event_is_keypress_with_modifiers(e, UI_KEY_PAGE_DOWN, UI_MODIFIER_NONE)) {
                scroll_to_focused = true;
                f32 page_dim = rect_dim_y(focus->focus_parent->screen_rect);
                f32 orig_pos = focus->calc_pos.y;
                f32 goto_pos = orig_pos + page_dim;
                while (focus->calc_pos.y < goto_pos && focus->next) {
                    focus = focus->next;
                }
            }
        }

        ui_set_focused_id(focus->id);
    } else {
        focus = root->focus_children.first;
        scroll_to_focused = true;
    }

    // Make sure focused item is visible.
    if (scroll_to_focused && focus) {
        ui_box *parent = focus->focus_parent;
        assert(parent == root);
        f32 margin = 20.0f;
        f32 page_dim = rect_dim_y(parent->screen_rect);
        f32 min_visible = parent->scroll.y;
        f32 max_visible = min_visible + page_dim;

        f32 item_pos = focus->calc_pos.y;
        f32 item_dim = focus->calc_dim.y;

        f32 item_min = item_pos;
        f32 item_max = item_pos + item_dim;

        if (item_max > max_visible - margin) {
            parent->scroll.y = item_max - page_dim + margin;
        }

        if (item_min < min_visible + margin) {
            parent->scroll.y = item_min - margin;
        }
    }

    // Clamp scroll.
    axis2 axis = root->child_axis;
    f32 min_scroll = 0.0f;
    f32 max_scroll = root->calc_child_sum.v[axis] - rect_dim_a(param->padded_screen_rect, axis);
    clamp_assign(&root->animated_scroll.pos.y, min_scroll, max_scroll);
    clamp_assign(&root->scroll.y, min_scroll, max_scroll);
}

static void ui_scrolled_x(ui_id id) {
    ui_new_box(id);

    ui_set_dim_x(ui_len_rel(1));
    ui_set_child_axis(AXIS2_X);
    ui_set_child_clip(true);
    ui_set_callback(ui_scrolled_callback, 0);
}

static void ui_scrolled_y(ui_id id) {
    ui_box *root = ui_new_box(id);

    ui_events events = ui_get_events();
    for_array (ui_event, e, events) {
        if (e->type == UI_EVENT_TYPE_SCROLL) {
            root->scroll.y += e->scroll.y * 100.0f;
        }
    }

    ui_set_dim_y(ui_len_rel(1));
    ui_set_child_axis(AXIS2_Y);
    ui_set_child_clip(true);
    ui_set_callback(ui_scrolled_callback, 0);
    ui_set_overflow(true);
}

////////////////////////////////////////////////////////////////
// rune: List

typedef struct ui_list_data ui_list_data;
struct ui_list_data {
    u64 selected_idx;
};

static void ui_list_begin(ui_id id) {
    ui_scrolled_y(id);
    ui_children_begin();
    ui_focus_group_begin();
}

static void ui_list_end(void) {
    ui_children_end();
    ui_focus_group_end();
}

typedef struct ui_list_item_signal ui_list_item_signal;
struct ui_list_item_signal {
    bool clicked;
};

static void ui_list_item_begin(ui_id id, ui_list_item_signal *signal) {
    ui_box *box = ui_new_box(id);
    ui_children_begin();
    ui_focus_group_begin();

    bool clicked = ui_button_interaction(id, ui_get_widget_hitbox(id));
    if (id == ui_get_focused_id()) {
        ui_events events = ui_get_events();
        for_array (ui_event, e, events) {
            if (ui_event_is_keypress_with_modifiers(e, UI_KEY_ENTER, UI_MODIFIER_NONE)) {
                clicked = true;
            }
        }
    }

    u32 bg = ui_anim_lerp_alpha(rgb(60, 60, 60), box->hot_t);
    ui_set_background(bg);

    bg = ui_anim_lerp_rgba(bg, rgb(50, 50, 50), box->active_t);
    ui_set_background(bg);

    if (ui_get_focused_id() == id) {
        ui_set_background(rgb(20, 60, 60));
    }

    if (signal) {
        signal->clicked = clicked;
    }
}

static void ui_list_item_end(void) {
    ui_children_end();
    ui_focus_group_end();
}

#define ui_list(id)                 defer(ui_list_begin(id),              ui_list_end())
#define ui_list_item(id, signal)    defer(ui_list_item_begin(id, signal), ui_list_item_end())
