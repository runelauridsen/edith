////////////////////////////////////////////////////////////////
// rune: UTF8 iterator

static bool yo_utf8_iter_init(str s, i64 *pos, u32 *codepoint) {
    bool ret = false;
    clamp_assign(pos, 0, s.len);
    if (*pos < s.len) {
        assert(!utf8_is_continuation(s.v[*pos]));
        if (codepoint) {
            unicode_codepoint cp = decode_single_utf8_codepoint(substr_idx(s, *pos));
            *codepoint = cp.codepoint;
        }
        ret = true;
    }

    return ret;
}

static bool yo_utf8_iter_next(str s, i64 *pos, u32 *codepoint) {
    do { *pos += 1; } while (*pos < s.len && utf8_is_continuation(s.v[*pos]));
    bool ret = yo_utf8_iter_init(s, pos, codepoint);
    return ret;
}

static bool yo_utf8_iter_prev(str s, i64 *pos, u32 *codepoint) {
    do { *pos -= 1; } while (*pos > 0 && utf8_is_continuation(s.v[*pos]));
    bool ret = yo_utf8_iter_init(s, pos, codepoint);
    return ret;
}

////////////////////////////////////////////////////////////////
// rune: Signal

typedef struct yo_signal yo_signal;
struct yo_signal {
    bool clicked : 1;
    bool pressed : 1;
    bool dragging : 1;
    bool focused : 1;
    bool double_clicked : 1;
    bool right_clicked : 1;
    bool right_pressed : 1;
    bool right_released : 1;
    bool right_dragging : 1;
    bool hovering : 1;
    bool mouse_is_over : 1;
    bool keyboard_pressed : 1;

    bool first_frame_active : 1;
};

static yo_signal yo_signal_from_node(yo_node *node) {
    YO_PROFILE_BEGIN(yo_signal_from_node);

    yo_signal sig = { 0 };

    yo_state *state = yo_state_get();
    bool mouse_is_over = false;

    assert(node->id != 0); // TODO(rune): Log error

    if (rect_contains(node->abs_rect, state->mouse_pos)) {
        sig.mouse_is_over = true;
        mouse_is_over = true;
    }

    if (node->flags & YO_NODE_FLAG_MOUSE_CLICKABLE) {
        // rune: Find first mouse press/release events for each mouse button.
        yo_event *e_mouse_press[YO_MOUSE_BUTTON_COUNT] = { 0 };
        yo_event *e_mouse_release[YO_MOUSE_BUTTON_COUNT] = { 0 };
        for_n (i64, i, YO_MOUSE_BUTTON_COUNT) {
            if (i != YO_MOUSE_BUTTON_NONE) {
                yo_event_list events = yo_events();
                for_list (yo_event, e, events) {
                    if (e->mouse_button == i && e->kind == YO_EVENT_KIND_MOUSE_PRESS && e_mouse_press[i] == null) {
                        e_mouse_press[i] = e;
                    }

                    if (e->mouse_button == i && e->kind == YO_EVENT_KIND_MOUSE_RELEASE && e_mouse_release[i] == null) {
                        e_mouse_release[i] = e;
                    }
                }
            }
        }

        // rune: Update hot id
        if (state->hot_id_found_this_frame == false && state->hot_id == 0 && state->active_id == 0 && mouse_is_over) {
            state->hot_id_found_this_frame = true;
            state->hot_id = node->id;
        } else if (state->hot_id == node->id && mouse_is_over == false) {
            state->hot_id = 0;
        }

        // rune: Update active id
        if (state->active_id == 0 && state->hot_id == node->id && e_mouse_press[YO_MOUSE_BUTTON_LEFT]) {
            state->active_id = node->id;
            state->mouse_drag_start = state->mouse_pos;
            sig.first_frame_active = 1;
            yo_event_eat(e_mouse_press[YO_MOUSE_BUTTON_LEFT]);
        } else if (state->active_id == node->id && e_mouse_release[YO_MOUSE_BUTTON_LEFT]) {
            state->active_id = 0;
            yo_event_eat(e_mouse_release[YO_MOUSE_BUTTON_LEFT]);

            sig.clicked = mouse_is_over;
        }

        // rune: Update focus id
        if (state->active_id == node->id) {
            state->focus_id = node->id;
        } else if (state->focus_id == node->id && e_mouse_press[YO_MOUSE_BUTTON_LEFT] && !mouse_is_over) {
            state->focus_id = 0;
        }

        // rune: Pressed
        if (state->active_id == node->id) {
            sig.pressed = true;
        }

        // rune: Dragging
        if (state->active_id == node->id) {
            sig.dragging = true;
        }

        // rune: Hovering
        if (state->hot_id == node->id && mouse_is_over) {
            sig.hovering = true;
        }

        // rune: Focused
        if (state->focus_id == node->id) {
            sig.focused = true;
        }
    }

    YO_PROFILE_END(yo_signal_from_node);
    return sig;
}

typedef struct yo_menu yo_menu;
struct yo_menu {
    rect avail_box;
    yo_node *node;
};

static void yo_menu_begin(yo_menu *menu, yo_id id, rect avail_box) {
    menu->node = yo_node_begin(id);
    menu->node->tag = "menu";

    yo_draw_rect(avail_box, YO_COLOR_SLATEBLUE);

    // rune: Padding
    vec2 padding = vec2(5, 5);
    yo_rect_cut_vec2(&avail_box, padding);

    yo_draw_rect(avail_box, YO_COLOR_GRAY);
    menu->avail_box = avail_box;
}

static void yo_menu_end(yo_menu *menu) {
    yo_node_end();
}

static void yo_text(str text) {
    yo_node(0) {
        vec2 dim = yo_dim_from_text(text);
        yo_draw_text(vec2(0, 0), text, YO_COLOR_WHITE);

        yo_node *node = yo_node_get();
        node->rel_rect = rect_make(vec2(0, 0), dim);
    }
}

static void yo_menu_item(yo_menu *menu, yo_id id, str text) {
    vec2 text_dim = yo_dim_from_text(text);
    vec2 padding = vec2(5, 2);
    rect box = yo_rect_cut_left(&menu->avail_box, text_dim.x + padding.x * 2);

    yo_node(id) {
        yo_node *node = yo_node_get();
        node->tag = (char *)text.v;
        node->flags = YO_NODE_FLAG_MOUSE_CLICKABLE;

        yo_signal sig = yo_signal_from_node(node);

        u32 background = YO_COLOR_DARKORANGE;
        if (sig.hovering) background = YO_COLOR_YELLOW;
        if (sig.pressed)  background = YO_COLOR_GOLD;

        yo_rect_cut_centered_y(&box, text_dim.y + padding.y * 2);
        yo_draw_rect(box, background);

        rect text_box = rect_make_dim(vec2_add(box.p0, padding), text_dim);
        yo_draw_text(text_box.p0, text, YO_COLOR_BLACK);

        f32 spacing = 5;
        yo_rect_cut_left(&menu->avail_box, spacing);

        node->rel_rect = box;

        yo_id ctx_menu_id = yo_id("ctx menu");
        if (sig.clicked) {
            yo_ctx_menu_open(ctx_menu_id);
        }

        yo_ctx_menu(ctx_menu_id) {
            yo_node *ctx_menu_node = yo_node_get();

            // rune: Build children
            yo_text(str("Item 1"));
            yo_text(str("Item 2"));
            yo_text(str("Item 3"));
            yo_text(str("Item 4"));

            // rune: Stack children vertically
            vec2 child_dim_sum = vec2(0, 0);
            vec2 child_dim_max = vec2(0, 0);
            for_list (yo_node, child, yo_children()) {
                child->rel_offset.y = child_dim_sum.y;

                vec2 dim = rect_dim(child->rel_rect);
                vec2_add_assign(&child_dim_sum, dim);
                vec2_max_assign(&child_dim_max, dim);
            }

            // rune: Position at bomttom left corner of parent.
            ctx_menu_node->rel_offset = vec2(box.x0, box.y1);

            // rune: Draw frame
            rect ctx_menu_rect = rect_make(vec2(0, 0), vec2(child_dim_max.x, child_dim_sum.y));
            yo_draw_rect(ctx_menu_rect, rgb(30, 30, 30));
        }
    }
}

static void yo_slider(yo_id id, rect area, f32 *value) {
    yo_node *node = yo_node_begin(id);
    node->tag = "slider";
    node->flags = YO_NODE_FLAG_MOUSE_CLICKABLE;
    node->rel_rect = area;

    yo_signal sig = yo_signal_from_node(node);

    u32 background = YO_COLOR_DARKGREEN;
    if (sig.hovering) background = YO_COLOR_GREEN;
    if (sig.dragging) background = YO_COLOR_BLUE;

    yo_draw_rect(area, background);

    vec2 delta    = vec2_sub(yo_mouse_pos(), area.p0);
    vec2 pct      = vec2_div(delta, rect_dim(area));
    vec2 pct_norm = vec2_clamp(pct, vec2(0, 0), vec2(1, 1));

    f32 filled_dim_x = rect_dim_x(area) * *value;
    rect filled_rect = yo_rect_get_left(&area, filled_dim_x);

    yo_draw_rect(filled_rect, rgba(255, 255, 255, 100));

    if (sig.dragging) {
        *value = pct_norm.x;
    }

    yo_node_end();
}

typedef struct yo_text_cursor yo_text_cursor;
struct yo_text_cursor {
    i64 caret;
    i64 mark;
    f32 anim_adv;
};

typedef enum yo_text_action_flags {
    YO_TEXT_ACTION_FLAG_SELECT  = 0x1,
    YO_TEXT_ACTION_FLAG_DELETE  = 0x2,
    YO_TEXT_ACTION_FLAG_BY_WORD = 0x4,
    YO_TEXT_ACTION_FLAG_BY_SIDE = 0x8,
} yo_text_action_flags;

typedef struct yo_text_action yo_text_action;
struct yo_text_action {
    i32 dir;
    u32 codepoint;
    yo_text_action_flags flags;
};

static i64 yo_buf_replace(u8 *buf, i64 cap, i64 *len, i64 replace_min, i64 replace_max, u8 *insert, i64 insert_len) {
    // rune: Don't insert outside the buffer
    clamp_assign(&replace_min, 0, *len);
    clamp_assign(&replace_max, 0, *len);
    i64 replace_len = replace_max - replace_min;

    if (*len - replace_len + insert_len <= cap) {
        // rune: Remove between replace_min/max and make space for the replace_with string
        u8 *shift_src =  buf + replace_max;
        u8 *shift_dst =  buf + replace_min + insert_len;
        i64 shift_len = *len - replace_max;
        memmove(shift_dst, shift_src, shift_len);

        // rune: Insert the new string
        memcpy(buf + replace_min, insert, insert_len);

        *len -= replace_len;
        *len += insert_len;
    } else {
        insert_len = 0;
    }

    return insert_len;
}

static yo_text_action yo_text_action_from_event(yo_event *e) {
    yo_text_action a = { 0 };

    switch (e->kind) {
        case YO_EVENT_KIND_KEY_PRESS: {
            switch (e->key) {
                case YO_KEY_LEFT: {
                    a.dir = -1;
                } break;

                case YO_KEY_RIGHT: {
                    a.dir = 1;
                } break;

                case YO_KEY_BACKSPACE: {
                    a.dir = -1;
                    a.flags |= YO_TEXT_ACTION_FLAG_DELETE;
                } break;

                case YO_KEY_DELETE: {
                    a.dir = 1;
                    a.flags |= YO_TEXT_ACTION_FLAG_DELETE;
                } break;

                case YO_KEY_HOME: {
                    a.dir = -1;
                    a.flags |= YO_TEXT_ACTION_FLAG_BY_SIDE;
                } break;

                case YO_KEY_END: {
                    a.dir = 1;
                    a.flags |= YO_TEXT_ACTION_FLAG_BY_SIDE;
                } break;
            }

            if (e->mods & YO_MODIFIER_SHIFT) {
                a.flags |= YO_TEXT_ACTION_FLAG_SELECT;
            }

            if (e->mods & YO_MODIFIER_CTRL) {
                a.flags |= YO_TEXT_ACTION_FLAG_BY_WORD;
            }
        } break;

        case YO_EVENT_KIND_CODEPOINT: {
            a.codepoint = e->codepoint;
        } break;
    }

    return a;
}

static bool yo_text_action_apply(yo_text_action *a, u8 *buf, i64 cap, i64 *len, yo_text_cursor *cursor) {
    bool applied = false;
    str s = str_make(buf, *len);
    clamp_assign(&cursor->caret, 0, *len);
    clamp_assign(&cursor->mark, 0, *len);

    // rune: Apply movement
    if (a->dir) {
        if (!(a->flags & YO_TEXT_ACTION_FLAG_DELETE) || cursor->caret == cursor->mark) {
            // rune: By home/end
            if (a->flags & YO_TEXT_ACTION_FLAG_BY_SIDE) {
                if (a->dir == 1) {
                    cursor->caret = *len;
                } else {
                    cursor->caret = 0;
                }
            }

            // rune: By word
            else if (a->flags & YO_TEXT_ACTION_FLAG_BY_WORD) {
                while (1) {
                    if (a->dir == 1) {
                        yo_utf8_iter_next(s, &cursor->caret, null);
                        if (cursor->caret == s.len) {
                            break;
                        }
                    } else {
                        yo_utf8_iter_prev(s, &cursor->caret, null);
                        if (cursor->caret == 0) {
                            break;
                        }
                    }

                    u32 curr_char = 0;
                    u32 prev_char = 0;
                    i64 pos = cursor->caret;
                    yo_utf8_iter_init(s, &pos, &curr_char);
                    yo_utf8_iter_prev(s, &pos, &prev_char);

                    bool prev_is_punct = u32_is_punct(prev_char);
                    bool curr_is_punct = u32_is_punct(curr_char);
                    bool prev_is_word = u32_is_word(prev_char) || u32_is_nonascii(prev_char);
                    bool curr_is_word = u32_is_word(curr_char) || u32_is_nonascii(curr_char);

                    if (!prev_is_punct && curr_is_punct) break;
                    if (!prev_is_word && curr_is_word) break;
                }
            }

            // rune: By character
            else {
                if (a->dir == 1) {
                    yo_utf8_iter_next(s, &cursor->caret, null);
                } else {
                    yo_utf8_iter_prev(s, &cursor->caret, null);
                }
            }

            // rune: Expand selection
            if (!(a->flags & YO_TEXT_ACTION_FLAG_SELECT) && !(a->flags & YO_TEXT_ACTION_FLAG_DELETE)) {
                cursor->mark = cursor->caret;
            }
        }

        applied = true;
    }

    // rune: Apply insertion and deletion
    if (a->codepoint || a->flags & YO_TEXT_ACTION_FLAG_DELETE) {
        i64 replace_min = min(cursor->caret, cursor->mark);
        i64 replace_max = max(cursor->caret, cursor->mark);

        u8  insert[4];
        i64 insert_len = a->codepoint ? encode_single_utf8_codepoint(a->codepoint, insert) : 0;
        insert_len = yo_buf_replace(buf, cap, len, replace_min, replace_max, insert, insert_len);

        cursor->caret = replace_min + insert_len;
        cursor->mark  = replace_min + insert_len;

        applied = true;
    }

    return applied;
}

static void yo_text_field(yo_id id, rect area, u8 *buf, i64 cap, i64 *len, yo_text_cursor *cursor) {
    yo_node(id) {
        yo_node *node = yo_node_get();
        node->flags = YO_NODE_FLAG_MOUSE_CLICKABLE;
        node->rel_rect = area;
        node->rel_offset = vec2(60, 60);

        yo_signal sig = yo_signal_from_node(node);

        // rune: Apply text edits -> mutate buffer and cursor
        if (sig.focused) {
            str s = str_make(buf, *len);
            for_list (yo_event, e, yo_events()) {
                yo_text_action a = yo_text_action_from_event(e);
                bool was_applied = yo_text_action_apply(&a, buf, cap, len, cursor);
                if (was_applied) {
                    yo_event_eat(e);
                }
            }
        }

        u32 background = rgb(30, 30, 30);
        if (sig.hovering) background = rgb(50, 50, 50);
        if (sig.focused)  background = rgb(10, 10, 10);

        str s = str_make(buf, *len);
        yo_draw_rect(area, background);
        yo_draw_text(area.p0, s, YO_COLOR_WHITE);

        if (sig.focused) {
            clamp_assign(&cursor->caret, 0, *len);
            clamp_assign(&cursor->mark, 0, *len);

            f32 caret_adv = yo_adv_from_text(str_left(s, cursor->caret));
            f32 mark_adv  = yo_adv_from_text(str_left(s, cursor->mark));

            // rune: Animate caret
            if (sig.first_frame_active) {
                cursor->anim_adv = caret_adv;
            } else {
                caret_adv = yo_anim_f32(&cursor->anim_adv, caret_adv, 25.0f);
            }

            if (cursor->caret == cursor->mark) {
                mark_adv = caret_adv;
            }

            // rune: Draw caret
            vec2 caret_p = vec2_add(area.p0, vec2(caret_adv, 0));
            vec2 mark_p  = vec2_add(area.p0, vec2(mark_adv, 0));
            yo_draw_caret(caret_p, 2, YO_COLOR_DARKCYAN);

            // rune: Draw selection
            if (caret_adv != mark_adv) {
                f32 sel_x_min = min(caret_adv, mark_adv);
                f32 sel_x_max = max(caret_adv, mark_adv);

                rect sel_rect = area;
                yo_rect_cut_left(&sel_rect, sel_x_min);
                yo_rect_max_left(&sel_rect, sel_x_max - sel_x_min);
                yo_draw_rect(sel_rect, rgba(255, 255, 255, 100));
            }
        }
    }
}
