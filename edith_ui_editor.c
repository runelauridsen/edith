static void editor_tab_handle_events(yo_id id, edith_editor_tab *tab, arena *temp) {
    unused(temp);

    YO_PROFILE_BEGIN(editor_tab_handle_events);

    edith_textview *editor = &tab->v;

    ////////////////////////////////////////////////
    // Process events.

    bool mouse_moved = false;
    for_list (yo_event, e, yo_events()) {
        if (yo_event_is_key_press(e, YO_KEY_ENTER, YO_MODIFIER_NONE)) {
            edith_textview_submit_u8(editor, '\n');
        }

        if (e->kind == YO_EVENT_KIND_CODEPOINT) {
            if (u32_is_printable(e->codepoint) || e->codepoint == '\n') {
                edith_textview_submit_u32(editor, (u8)e->codepoint);
            }
        }

        //- Click to move cursor.
        if (e->kind == YO_EVENT_KIND_MOUSE_PRESS) {
            bool keep_mark = e->mods & YO_MODIFIER_SHIFT;
            vec2 screen_pos = yo_mouse_pos();
            if (e->mods == (YO_MODIFIER_CTRL|YO_MODIFIER_ALT)) {
                edith_textview_add_cursor_at_pixel(editor, screen_pos, false);
            } else {
                edith_textview_move_to_pixel(editor, screen_pos, keep_mark);
            }
        }

        //- Scroll.
        if (e->kind == YO_EVENT_KIND_SCROLL) {
            const i64 lines_per_scroll = 4;
            editor->target_scroll.row += i64(e->scroll.y) * lines_per_scroll;
        }

        //- Drag select.
        if (e->kind == YO_EVENT_KIND_MOUSE_MOVE) {
            mouse_moved = true;
        }
    }

    ////////////////////////////////////////////////
    // Drag select.

    // TODO(rune): || !(scroll_animation.is_completed)
    if (mouse_moved) {
        // TODO(rune): && is mouse button down
        if (id == yo_active_id() && 0) {
            edith_textview_cursors_clear_secondary(editor);
            vec2 screen_pos = yo_mouse_pos();
            i64 pos = edith_textview_pos_from_pixel(editor, screen_pos);

            for_array (edith_cursor, it, editor->cursors) {
                if (it->is_primary) {
                    it->caret = pos;
                    break;
                }
            }
        }
    }

    YO_PROFILE_END(editor_tab_handle_events);
}

typedef struct caret_to_draw caret_to_draw;
struct caret_to_draw {
    vec2 pos;
    bool is_primary;
    caret_to_draw *next;
};

static void ui_draw_editor(yo_id id, edith_textview *tv, rect viewport, yo_face face) {
    YO_PROFILE_BEGIN(ui_draw_editor);

    yo_node *node = yo_node_begin(id);

    edith_textview_scroll_clamp(tv);

#if EDITH_TEXTVIEW_DEBUG_PRINT && !defined(RUN_TESTS)
    println("===============================================================");
    println("is_editing = %", editor->is_editing);
    for_val (active_cursor, it, editor->active_cursors) {
        if (it.is_primary) print(ANSICONSOLE_FG_BRIGHT_CYAN);
        println("caret = % \t mark = % \t uncommit_start %", it.caret, it.mark, it.uncomitted_start);
        print(ANSICONSOLE_RESET);
    }

    editor_debug_print_saved_cursors(editor);
#endif

    arena *temp = tv->local;
    arena_scope_begin(temp);

    //ui_push_scissor();
    //ui_set_scissor(viewport, 0);
    edith_textview_set_face(tv, face);

    yo_signal sig = yo_signal_from_node(node);
    node->rel_rect = viewport;

    u32 background_color = rgb(15, 15, 15);
    u32 foreground_color = rgb(180, 180, 180);

    f32 lineheight    = f32(tv->cell_dim.y);
    vec2 cell_dim     = vec2_from_ivec2(tv->cell_dim);

    yo_draw_rect(viewport, background_color);

    yo_face_set(face);

    ////////////////////////////////////////////////////////////////
    // rune: Animate

    vec2 target_scroll_f = vec2(0, f32(tv->target_scroll.row * tv->cell_dim.y));
#if 0
    f32 scroll_anim_rate = 15.0f;
    ui_anim_damped_vec2(&tv->animated_scroll, target_scroll_f, scroll_anim_rate, 0.1f);
#else
    f32 scroll_anim_rate = 18.0f;
    yo_anim_vec2(&tv->animated_scroll, target_scroll_f, scroll_anim_rate);
#endif

    // NOTE(rune): Sine glyphs are always snapped to the pixel grid,
    // we use rounded scroll to avoid flickering, when a glyph snaps
    // from one pixel to the next.
    vec2 rounded_scroll = vec2_round(tv->animated_scroll);
    i64 lines_per_page = i64(rect_dim_y(viewport) / lineheight) + 1;

    ////////////////////////////////////////////////////////////////
    // rune: Calculate visible part of document

    i64 min_visible_line    = i64(rounded_scroll.y / lineheight);
    i64 max_visible_line    = min_visible_line + lines_per_page;
    i64 min_visible_line_y  = min_visible_line * i64(lineheight);
    i64 max_visible_line_y  = max_visible_line * i64(lineheight);
    i64 min_visible_pos     = edith_textbuf_pos_from_row(&tv->tb, min_visible_line);
    i64 max_visible_pos     = edith_textbuf_pos_from_row(&tv->tb, max_visible_line + 1);

    unused(max_visible_line_y);

    vec2 scroll_fraction = { 0 };
    scroll_fraction.y += min_visible_line_y;
    scroll_fraction.x -= rounded_scroll.x;
    scroll_fraction.y -= rounded_scroll.y;

    ////////////////////////////////////////////////////////////////
    // rune: Gather selected ranges inside viewport and determine which cursors to draw.

    YO_PROFILE_BEGIN(editor_draw_calc_selections);

    // NOTE(rune): We will at most have active_cursors.count number of selections.
    i64_range *selected_ranges = arena_push_array_nozero(tv->local, i64_range, tv->cursors.count);;
    i64 selected_ranges_count = 0;
    caret_to_draw *carets = null;
    edith_cursor primary_cursor = { 0 };
    {
        // NOTE(rune): Since the cursors animated posistion lags begin its pos,
        // we add 10 lines of extra margin.
        i64 caret_clamp_pos_margin = 10;
        i64 caret_clamp_pos_min = edith_textbuf_pos_from_row(&tv->tb, min_visible_line - caret_clamp_pos_margin);
        i64 caret_clamp_pos_max = edith_textbuf_pos_from_row(&tv->tb, max_visible_line + caret_clamp_pos_margin);

        for_array (edith_cursor, it, tv->cursors) {

            ////////////////////////////////////////////////////////////////
            // rune: Draw selection?

            i64_range it_range = edith_cursor_range(*it);
            clamp_assign(&it_range.min, min_visible_pos, max_visible_pos);
            if (it_range.min < it_range.max) {
                selected_ranges[selected_ranges_count++] = it_range;
            }

            ////////////////////////////////////////////////////////////////
            // rune: Draw caret?

            if (it->caret >= caret_clamp_pos_min &&
                it->caret <= caret_clamp_pos_max) { // NOTE(rune): Must be <= since caret can be at pos == document length

                edith_textbuf_coord caret_coord = edith_textbuf_coord_from_pos(&tv->tb, it->caret);
                vec2 caret_p = vec2(
                    (f32)caret_coord.col * tv->cell_dim.x,
                    (f32)caret_coord.row * tv->cell_dim.y
                );

                // Animate?
                bool is_drag_selecting = id == yo_active_id() && 0; // TODO(rune): & ui_mouse_button_is_down(YO_MOUSE_BUTTON_LEFT);
                if (is_drag_selecting) it->animated_p = caret_p;

#if 0
                it->animated_p.pos = caret_p;
#elif 0
                f32 caret_anim_rate = 20.0f;
                ui_anim_damped_vec2(&it->animated_p, caret_p, caret_anim_rate, 1);
#else
                f32 caret_anim_rate = 20.0f;
                yo_anim_vec2(&it->animated_p, caret_p, caret_anim_rate);
#endif

                // Push to list of carets that need to be drawn.
                caret_to_draw *a = arena_push_struct(yo_frame_arena(), caret_to_draw);
                a->is_primary = it->is_primary;
                a->pos = it->animated_p;
                slstack_push(&carets, a);

            } else {
                // TODO(rune): ui_reset_damped_vec2(&it->animated_p);
            }

            ////////////////////////////////////////////////////////////////
            // rune: Store primary cursor for later.

            if (it->is_primary) {
                primary_cursor = *it;
            }
        }
    }
    YO_PROFILE_END(editor_draw_calc_selections);

    ////////////////////////////////////////////////////////////////
    // rune: Draw line numbers

    YO_PROFILE_BEGIN(editor_draw_linenumbers);
    {
        i64 highlight_linenumber;
        if (1) {
            highlight_linenumber = i64((primary_cursor.animated_p.y + lineheight / 2) / lineheight);
        } else {
            highlight_linenumber = edith_textbuf_row_from_pos(&tv->tb, primary_cursor.caret);
        }

        i64 max_linenumber = edith_textbuf_row_count(&tv->tb);
        str longest_decimal = yo_fmt("%", max_linenumber);

        f32 linenumber_padding = 5.0f;
        f32 linenumber_width = yo_adv_from_text(longest_decimal);
        u32 linenumber_color = rgb(60, 60, 60);
        u32 linenumber_highlight_color = rgb(90, 90, 90);
        f32 linenumber_anim_rate = 20.0f;
        linenumber_width = f32_round(
            yo_anim_damped_f32(
                &tv->anim_linenumber_width,
                &tv->anim_linenumber_width_vel,
                linenumber_width,
                linenumber_anim_rate,
                0.1f,
                &tv->anim_linenumber_width_started,
                &tv->anim_linenumber_width_finished
            )
        );

        yo_rect_cut_left(&viewport, linenumber_padding);
        rect linenumber_area = yo_rect_cut_left(&viewport, linenumber_width);
        yo_rect_cut_left(&viewport, linenumber_padding);

        i64 at = min_visible_line;
        vec2 p = vec2_add(linenumber_area.p0, scroll_fraction);
        while (p.y < viewport.y1 && at < max_linenumber) {
            str decimal = yo_fmt("%", at + 1);
            f32 adv = yo_adv_from_text(decimal);

            vec2 glyph_p = p;
            glyph_p.x += linenumber_width - adv;

            u32 at_color;
            if (at == highlight_linenumber) {
                at_color = linenumber_highlight_color;
            } else {
                at_color = linenumber_color;
            }
            yo_draw_text(glyph_p, decimal, at_color);

            at++;
            p.y += lineheight;
        }

        u32 divider_color = rgb(50, 50, 50);
        f32 divider_width = 2.0f;
        rect divider_area = yo_rect_cut_left(&viewport, divider_width);
        yo_draw_rect(divider_area, divider_color);
        f32 divider_margin = 5.0f;
        yo_rect_cut_left(&viewport, divider_margin);
    }
    YO_PROFILE_END(editor_draw_linenumbers);

    ////////////////////////////////////////////////////////////////
    // rune: Gather syntax highlight ranges

    edith_syntax_range_list syntax_ranges = lang_c_get_syntax_ranges(&tv->tb.indexer, &tv->tb.gb, i64_range(min_visible_pos, max_visible_pos), temp);

    ////////////////////////////////////////////////////////////////
    // rune: Finder results

    edith_search_result_array search_results = edith_search_results_from_key(tv->search_result_key_visible, 0, 1);
    i64 search_result_idx = edith_search_result_idx_from_pos(search_results, min_visible_pos).max;
    if (search_result_idx == -1) {
        search_result_idx = I64_MAX;
    }

    ////////////////////////////////////////////////////////////////
    // rune: Draw glyphs.

    YO_PROFILE_BEGIN(editor_draw_glyphs);
    edith_syntax_range *curr_syntax_range = syntax_ranges.first;
    rect glyph_viewport = viewport;
    tv->glyph_viewport = irect_from_rect(glyph_viewport);
    tv->cells_per_page = irect_dim_y(tv->glyph_viewport) / tv->cell_dim.y;
    f32 max_y = rect_dim_y(viewport);
    vec2 p = scroll_fraction;
    edith_doc_iter iter = { 0 };
    edith_doc_iter_jump(&iter, &tv->tb.gb, min_visible_pos);
    i64 line_counter = 0;
    while (iter.valid && p.y < max_y) {
        rect cell_rect = { 0 };

        // rune: Move syntax range forward
        while (curr_syntax_range) {
            if (curr_syntax_range->range.max <= iter.pos) {
                curr_syntax_range = curr_syntax_range->next;
            } else {
                break;
            }
        }

        // rune: Move search result forward
        while (search_result_idx < search_results.count) {
            if (search_results.v[search_result_idx].range.max <= iter.pos) {
                search_result_idx++;
            } else if (search_results.v[search_result_idx].range.max >= max_visible_pos) {
                search_result_idx = search_results.count;
            } else {
                break;
            }
        }

        switch (iter.codepoint) {
            case '\n': {
                vec2 p0 = vec2_add(p, glyph_viewport.p0);
                cell_rect = rect_make_dim(p0, cell_dim);

                p.x = 0.0f;
                p.y += lineheight;

                line_counter++;
            } break;

            default: {
                u32 color = foreground_color;
                if (curr_syntax_range &&
                    curr_syntax_range->range.min <= iter.pos &&
                    curr_syntax_range->range.max >  iter.pos) {
                    switch (curr_syntax_range->kind) {
                        case EDITH_SYNTAX_KIND_KEYWORD:     color = rune_theme_fg_keyword;  break;
                        case EDITH_SYNTAX_KIND_LITERAL:     color = rune_theme_fg_literal;  break;
                        case EDITH_SYNTAX_KIND_COMMENT:     color = rune_theme_fg_comment;  break;
                        case EDITH_SYNTAX_KIND_OPERATOR:    color = rune_theme_fg_operator; break;
                        case EDITH_SYNTAX_KIND_PUNCT:       color = rune_theme_fg_punct;    break;
                        case EDITH_SYNTAX_KIND_PREPROC:     color = rune_theme_fg_preproc;  break;
                    }
                }

                vec2 p0 = vec2_add(p, glyph_viewport.p0);
                cell_rect = rect_make_dim(p0, cell_dim);

                yo_draw_glyph(cell_rect.p0, iter.codepoint, color);

                if (search_result_idx < search_results.count) {
                    if (range_contains(search_results.v[search_result_idx].range, iter.pos)) {
                        yo_draw_rect(cell_rect, rgba(255, 180, 0, 100));
                    }
                }

                p.x += tv->cell_dim.x;
            } break;
        }

        for (i64 i = 0; i < selected_ranges_count; i++) {
            if (range_contains(selected_ranges[i], iter.pos)) {
                u32 selection_color = rgba(200, 255, 255, 50);
                rect r = cell_rect;
                yo_draw_rect(r, selection_color);
                break;
            }
        }

        edith_doc_iter_next(&iter, DIR_FORWARD);
    }
    YO_PROFILE_END(editor_draw_glyphs);

    ////////////////////////////////////////////////////////////////
    // rune: Draw cursors.

    YO_PROFILE_BEGIN(editor_draw_cursors);
    {
        u32 primary_caret_color   = rgb(0, 255, 0);
        u32 secondary_caret_color = rgb(0, 200, 200);

        for (caret_to_draw *it = carets; it; it = it->next) {
            u32 caret_color = it->is_primary ? primary_caret_color : secondary_caret_color;
            vec2 caret_screen_p = it->pos;
            vec2_sub_assign(&caret_screen_p, rounded_scroll);
            vec2_add_assign(&caret_screen_p, glyph_viewport.p0);
            yo_draw_caret(caret_screen_p, 2, caret_color);
        }
    }
    YO_PROFILE_END(editor_draw_cursors);

    //ui_pop_scissor();
    arena_scope_end(tv->local);

    yo_node_end();

    YO_PROFILE_END(ui_draw_editor);

}
