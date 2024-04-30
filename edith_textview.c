////////////////////////////////////////////////////////////////
// rune: Helpers

#define EDITH_CURSOR_NO_WANT_X (I64_MAX - 1)
#define EDITH_CURSOR_NO_WANT_Y (I64_MAX - 1)
#define EDITH_CURSOR_NO_WANT   ((edith_textbuf_coord) { EDITH_CURSOR_NO_WANT_X, EDITH_CURSOR_NO_WANT_Y })

static edith_cursor edith_cursor_make(i64 caret, i64 mark, edith_textbuf_coord want) {
    edith_cursor ret = { 0 };
    ret.caret = caret;
    ret.mark = mark;
    ret.want = want;
    return ret;
}
static i64_range edith_cursor_range(edith_cursor cursor) {
    return make_range_i64(cursor.caret, cursor.mark);
}

////////////////////////////////////////////////////////////////
// rune: General

static void edith_textview_create(edith_textview *tv) {
    zero_struct(tv);

    tv->local = edith_arena_create_default(str("textview/local"));
    darray_create(&tv->cursors, 4096);

    edith_textview_set_face(tv, (ui_face) { 0 });

    edith_cursor inital_cursor = edith_cursor_make(0, 0, EDITH_CURSOR_NO_WANT);
    inital_cursor.is_primary = true;
    darray_put(&tv->cursors, inital_cursor);
}

static void edith_textview_destroy(edith_textview *tv) {
    edith_arena_destroy(tv->local);
    darray_destroy(&tv->cursors);
}

static void edith_textview_set_face(edith_textview *tv, ui_face face) {
    tv->cell_dim.x = (i64)ui_font_backend_get_advance(' ', face);
    tv->cell_dim.y = (i64)ui_font_backend_get_lineheight(face);
}

////////////////////////////////////////////////////////////////
// rune: Pixel <> coord <> pos translation

static i64 edith_textview_pos_from_pixel(edith_textview *tv, vec2 pixel) {
    vec2_add_assign(&pixel, tv->animated_scroll.pos);
    vec2_sub_assign(&pixel, vec2_from_ivec2(tv->glyph_viewport.p0));
    edith_textbuf_coord coord = {
        .col = i64(pixel.x) / tv->cell_dim.x,
        .row = i64(pixel.y) / tv->cell_dim.y,
    };

    i64 pos = edith_textbuf_pos_from_coord(&tv->tb, coord);
    return pos;
}

////////////////////////////////////////////////////////////////
// rune: Scroll

static edith_cursor *edith_textview_cursors_get_primary(edith_textview *tv) {
    assert(tv->cursors.count > 0);
    edith_cursor *ret = 0;
    for_array (edith_cursor, it, tv->cursors) {
        if (it->is_primary) {
            ret = it;
            break;
        }
    }
    assert(ret);
    return ret;
}

static void edith_textview_scroll_clamp(edith_textview *tv) {
    i64 line_count = edith_textbuf_row_count(&tv->tb);

    i64_range col_range = { 0, I64_MAX }; // TODO(rune): Max should be length of longest line
    i64_range row_range = { 0, line_count - 1 }; // NOTE(rune): Subtract 1 to keep last line on screen.

    clamp_to_range_assign(&tv->target_scroll.col, col_range);
    clamp_to_range_assign(&tv->target_scroll.row, row_range);
}

static void edith_textview_scroll_to_coord(edith_textview *tv, edith_textbuf_coord coord, edith_textview_scroll_target target) {
    i64_range visible_rows = {
          .min = tv->target_scroll.row,
          .max = tv->target_scroll.row + tv->cells_per_page,
    };

    switch (target) {
        case EDITH_TEXTVIEW_SCROLL_TARGET_DEFAULT: {
            i64 margin = 3;
            if (coord.row < visible_rows.min + margin) {
                tv->target_scroll.row = coord.row - margin;
            }

            if (coord.row > visible_rows.max - margin - 1) {
                tv->target_scroll.row = coord.row + margin + 1 - tv->cells_per_page;
            }
        } break;

        case EDITH_TEXTVIEW_SCROLL_TARGET_CENTER: {
            tv->target_scroll.row = coord.row - tv->cells_per_page / 2 + 1;
        } break;

    }

    edith_textview_scroll_clamp(tv);
}

static void edith_textview_scroll_to_pos(edith_textview *tv, i64 pos, edith_textview_scroll_target target) {
    edith_textbuf_coord coord = edith_textbuf_coord_from_pos(&tv->tb, pos);
    edith_textview_scroll_to_coord(tv, coord, target);
}

static void edith_textview_scroll_to_cursor(edith_textview *tv, edith_cursor cursor, edith_textview_scroll_target target) {
    edith_textview_scroll_to_pos(tv, cursor.caret, target);
}

static void edith_textview_scroll_to_primary_cursor(edith_textview *tv, edith_textview_scroll_target target) {
    edith_cursor primary = *edith_textview_cursors_get_primary(tv);
    edith_textview_scroll_to_cursor(tv, primary, target);
}

////////////////////////////////////////////////////////////////
// rune: Cursors

static void edith_debug_print_saved_cursors(edith_textview *tv);

static int edith_cmp_cursor_selection_min(const void *a_, const void *b_) {
    const edith_cursor *a = a_;
    const edith_cursor *b = b_;

    i64_range range_a = edith_cursor_range(*a);
    i64_range range_b = edith_cursor_range(*b);

    if (range_a.min > range_b.min)        return 1;
    else if (range_a.min < range_b.min)   return -1;
    else                                  return 0;
}

static void edith_assert_that_cursors_are_sorted(edith_textview *tv) {
    edith_cursor *v = tv->cursors.v;
    for_n (i64, i, tv->cursors.count - 1) {
        assert(edith_cursor_range(v[i]).min < edith_cursor_range(v[i + 1]).min);
    }
}

static void edith_assert_that_selections_to_not_overlap(edith_textview *tv) {
    edith_cursor *v = tv->cursors.v;
    for_n (i64, i, tv->cursors.count - 1) {
        assert(!ranges_overlap(edith_cursor_range(v[i]), edith_cursor_range(v[i + 1])));
    }
}

static void edith_textview_cursors_dedup(edith_textview *tv) {
    assert(tv->cursors.count > 0);
    if (tv->cursors.count > 1) {
        YO_PROFILE_BEGIN(edith_textview_cursors_dedup__qsort);
        qsort(tv->cursors.v, tv->cursors.count, sizeof(edith_cursor), edith_cmp_cursor_selection_min);
        YO_PROFILE_END(edith_textview_cursors_dedup__qsort);

        YO_PROFILE_BEGIN(edith_textview_cursors_dedup__collapse_loop);
        i64 tb_len          = edith_textbuf_len(&tv->tb);
        edith_cursor *begin = tv->cursors.v;
        edith_cursor *end   = tv->cursors.v + tv->cursors.count;
        edith_cursor *w     = begin;        // write pointer
        edith_cursor *r     = begin + 1;    // read pointer
        while (r < end) {
            min_assign(&r->caret, tb_len);
            min_assign(&r->mark, tb_len);

            i64_range r_range = edith_cursor_range(*r);
            i64_range w_range = edith_cursor_range(*w);

            bool touching = ((r_range.min < w_range.max) || (r->caret == w->caret));
            if (touching) {
                if (r->caret < w_range.min || r->caret > w_range.max) {
                    w->caret = r->caret;
                    w->animated_p = r->animated_p;
                }

                if (r->mark < w_range.min || r->mark > w_range.max) {
                    w->mark = r->mark;
                }

                w->is_primary |= r->is_primary;
            } else {
                *++w = *r;
            }

            r++;
        }
        YO_PROFILE_END(edith_textview_cursors_dedup__collapse_loop);

        tv->cursors.count = w - begin + 1;

        edith_assert_that_selections_to_not_overlap(tv);
    }
}

static void edith_textview_cursors_clear_secondary(edith_textview *tv) {
    edith_textview_submit_barrier(tv);
    edith_cursor primary = *edith_textview_cursors_get_primary(tv);
    darray_reset(&tv->cursors);
    darray_put(&tv->cursors, primary);
}

static void edith_editor_clear_selections(edith_textview *tv) {
    edith_textview_submit_barrier(tv);
    for_array (edith_cursor, it, tv->cursors) {
        it->mark = it->caret;
    }
}

static void edith_textview_add_cursor_at_pos(edith_textview *tv, i64 caret, i64 mark, bool make_primary, edith_cursor *parent) {
    edith_cursor new_cursor = edith_cursor_make(caret, mark, EDITH_CURSOR_NO_WANT);

    if (make_primary) {
        new_cursor.is_primary = true;
        for_array (edith_cursor, it, tv->cursors) {
            it->is_primary = false;
        }
    }

    if (parent) {
        new_cursor.animated_p = parent->animated_p;
        new_cursor.want = parent->want;
    }

    darray_put(&tv->cursors, new_cursor);
    edith_textview_cursors_dedup(tv);

    edith_textview_scroll_to_cursor(tv, new_cursor, EDITH_TEXTVIEW_SCROLL_TARGET_DEFAULT);
}

static void edith_textview_add_cursor_at_coord(edith_textview *tv, edith_textbuf_coord coord, bool make_primary, edith_cursor *parent) {
    i64 pos = edith_textbuf_pos_from_coord(&tv->tb, coord);
    edith_textview_add_cursor_at_pos(tv, pos, pos, make_primary, parent);
}

static void edith_textview_add_cursor_at_pixel(edith_textview *tv, vec2 pixel, bool make_primary) {
    i64 pos = edith_textview_pos_from_pixel(tv, pixel);
    edith_textview_add_cursor_at_pos(tv, pos, pos, make_primary, null);
}

static void edith_textview_add_cursor_at_row_offset(edith_textview *tv, i64 row_offset) {
    edith_cursor primary_cursor = *edith_textview_cursors_get_primary(tv);
    if (primary_cursor.want.col == EDITH_CURSOR_NO_WANT_X) {
        primary_cursor.want.col = edith_textbuf_col_from_pos(&tv->tb, primary_cursor.caret);
    }

    edith_textbuf_coord coord = edith_textbuf_coord_from_pos(&tv->tb, primary_cursor.caret);
    coord.row += row_offset;
    coord.col = primary_cursor.want.col;
    edith_textview_add_cursor_at_coord(tv, coord, true, &primary_cursor);
}

////////////////////////////////////////////////////////////////
// rune: Saved cursors

static void edith_textview_cursors_push(edith_textview *tv, side side) {
    i64 num = tv->cursors.count;

    edith_saved_cursor_array saved = edith_saved_cursor_array_reserve(num, tv->tb.history.arena);
    saved.tv = tv;

    for_n (i64, i, num) {
        edith_cursor       *src = &tv->cursors.v[i];
        edith_saved_cursor *dst = &saved.v[i];
        dst->caret = src->caret;
        dst->mark = src->mark;
        dst->want = src->want;
        dst->side = side;
        dst->is_primary = src->is_primary;
    }

    assert(tv->tb.history.at->saved_cursors[side].count == 0);
    tv->tb.history.at->saved_cursors[side] = saved;
    assert(tv->tb.history.at->saved_cursors[side].count > 0);

    edith_debug_print_saved_cursors(tv);
}

static void edith_textview_cursors_restore(edith_textview *tv, edith_saved_cursor_array saved_cursors) {
    assert(saved_cursors.count > 0);

    darray_reset(&tv->cursors);
    for_array (edith_saved_cursor, saved, saved_cursors) {
        edith_cursor restored = edith_cursor_make(saved->caret, saved->mark, saved->want);
        restored.is_primary = saved->is_primary;
        darray_put(&tv->cursors, restored);
    }

    edith_debug_print_saved_cursors(tv);
    edith_textview_scroll_to_primary_cursor(tv, EDITH_TEXTVIEW_SCROLL_TARGET_DEFAULT);
}

static void edith_debug_print_saved_cursors(edith_textview *tv) {
#if EDITH_TEXTVIEW_DEBUG_PRINT && !RUN_TESTS
    println("saved_cursor_idx = %", tv->saved_cursor_idx);

    i64 idx = 0;
    for_val (saved_cursor, r, tv->saved_cursors) {
        if (idx == tv->saved_cursor_idx) print(ANSICONSOLE_FG_YELLOW);
        println("idx = % \t side = % \t opid = % \t caret = % \t mark = %" ANSICONSOLE_RESET, idx, r.side ? "MAX" : "MIN", r.opid, r.caret, r.mark);
        idx++;
    }

    println("");
    println("");
#else
    unused(tv);
#endif
}

static void edith_textview_submit_begin(edith_textview *tv) {
    if (tv->tb.submit.submit_in_progress == false) {
        edith_edit_history_begin_record(&tv->tb.history);

        edith_textview_cursors_push(tv, SIDE_MIN);
        tv->tb.submit.submit_in_progress = true;
    }
}

static void edith_textview_submit_barrier(edith_textview *tv) {
    if (edith_textbuf_commit(&tv->tb)) {
        edith_textview_cursors_push(tv, SIDE_MAX);
    }
}

////////////////////////////////////////////////////////////////
// rune: Basic movement

static void edith_textview_move_to_pos(edith_textview *tv, i64 pos, bool expand_selection) {
    edith_textview_submit_barrier(tv);

    edith_textview_cursors_clear_secondary(tv);
    tv->cursors.v[0].caret = pos;

    if (!expand_selection) {
        tv->cursors.v[0].mark = tv->cursors.v[0].caret;
    }

    tv->cursors.v[0].want = EDITH_CURSOR_NO_WANT;

    edith_textview_scroll_to_pos(tv, tv->cursors.v[0].caret, EDITH_TEXTVIEW_SCROLL_TARGET_DEFAULT);
}

static void edith_textview_move_to_coord(edith_textview *tv, edith_textbuf_coord coord, bool expand_selection) {
    YO_PROFILE_BEGIN(edith_textview_move_to_coord);

    i64 pos = edith_textbuf_pos_from_coord(&tv->tb, coord);
    edith_textview_move_to_pos(tv, pos, expand_selection);

    YO_PROFILE_END(edith_textview_move_to_coord);
}

static void edith_textview_move_to_pixel(edith_textview *tv, vec2 pixel, bool keep_mark) {
    i64 pos = edith_textview_pos_from_pixel(tv, pixel);
    edith_textview_move_to_pos(tv, pos, keep_mark);
}

static void edith_textview_submit_move_cursor(edith_textview *tv, move_by by, dir dir, bool expand_selection, edith_cursor *it) {
    if (!dir) return;
    if (!by) return;

    bool keep_want_col = false;
    bool keep_want_row = false;

    bool backward = dir == DIR_BACKWARD;

    edith_gapbuffer *gb = &tv->tb.gb;

    switch (by) {
        case MOVE_BY_CHAR: {
            i64_range selection = i64_range(it->caret, it->mark);
            if (selection.min != selection.max && expand_selection == false) {
                i64 to = 0;
                if (dir == DIR_FORWARD)  to = selection.max;
                if (dir == DIR_BACKWARD) to = selection.min;
                it->caret = to;
                it->mark  = to;
            } else {
                edith_doc_iter iter = { 0 };
                edith_doc_iter_jump(&iter, gb, it->caret);
                edith_doc_iter_next(&iter, dir);
                it->caret = iter.pos;
            }
        } break;

        case MOVE_BY_WORD: {
            edith_doc_iter iter = { 0 };
            edith_doc_iter_jump(&iter, gb, it->caret);
            edith_doc_iter_next(&iter, dir);

            while (iter.valid) {
                edith_doc_iter prev = iter;
                edith_doc_iter_next(&prev, DIR_BACKWARD);

                char_flags curr_flags = u32_get_char_flags(iter.codepoint);
                char_flags prev_flags = u32_get_char_flags(prev.codepoint);

                if (iter.codepoint == '\n') break;

                if (!(prev_flags & CHAR_FLAG_PUNCT) && (curr_flags & CHAR_FLAG_PUNCT)) break;
                if (!(prev_flags & CHAR_FLAG_WORD)  && (curr_flags & CHAR_FLAG_WORD)) break;

                edith_doc_iter_next(&iter, dir);
            }

            it->caret = iter.pos;

        } break;

        case MOVE_BY_SUBWORD: {
            // TODO(rune): @Implement
        } break;

        case MOVE_BY_HOME_END: {
            edith_doc_iter iter = { 0 };
            edith_doc_iter_jump(&iter, gb, it->caret);
            edith_doc_iter_to_line_side(&iter, backward ? SIDE_MIN : SIDE_MAX);
            i64 line_side = iter.pos;

            while (iter.codepoint != '\n' && u32_is_whitespace(iter.codepoint)) {
                edith_doc_iter_next(&iter, DIR_FORWARD);
            }

            i64 first_non_whitespace = iter.pos;

            if (it->caret != first_non_whitespace && backward) {
                it->caret = first_non_whitespace;
            } else {
                it->caret = line_side;
            }

            if (dir == DIR_FORWARD) {
                it->want.col = I64_MAX;
                keep_want_col = true;
            }
        } break;

        case MOVE_BY_PARAGRAPH: {
            i32 prev_was_paragraph_sep = -1;
            i64 found = -1;
            edith_doc_iter iter = { 0 };
            edith_doc_iter_jump(&iter, gb, it->caret);
            edith_doc_iter_next(&iter, dir);

            edith_doc_iter next = iter;
            edith_doc_iter_next(&next, dir);

            while (next.valid) {
                i32 this_is_paragrap_sep = (iter.codepoint == '\n' && next.codepoint == '\n');
                if (this_is_paragrap_sep) {
                    if (dir == DIR_FORWARD) {
                        found = next.pos;
                    } else {
                        found = iter.pos;
                    }
                }

                if (this_is_paragrap_sep != prev_was_paragraph_sep && prev_was_paragraph_sep != -1) {
                    break;
                }
                prev_was_paragraph_sep = this_is_paragrap_sep;
                iter = next;
                edith_doc_iter_next(&next, dir);
            }

            if (found != -1) {
                it->caret = found;
            } else if (dir == DIR_FORWARD) {
                it->caret = edith_gapbuffer_len(gb);
            } else if (dir == DIR_BACKWARD) {
                it->caret = 0;
            }
        } break;

        case MOVE_BY_LINE: {
            keep_want_col = true;

            edith_textbuf_coord caret_coord = edith_textbuf_coord_from_pos(&tv->tb, it->caret);
            if (it->want.col == EDITH_CURSOR_NO_WANT_X) {
                it->want.col = caret_coord.col;
            }

            edith_textbuf_coord goto_pos = {
                .col = it->want.col,
                .row = caret_coord.row + dir,
            };

            it->caret = edith_textbuf_pos_from_coord(&tv->tb, goto_pos);
        } break;

        case MOVE_BY_PAGE: {
            keep_want_col = true;
            keep_want_row = true;

            edith_textbuf_coord caret_pos = edith_textbuf_coord_from_pos(&tv->tb, it->caret);

            if (it->want.col == EDITH_CURSOR_NO_WANT_X) {
                it->want.col = caret_pos.col;
            }

            if (it->want.row == EDITH_CURSOR_NO_WANT_Y) {
                it->want.row = caret_pos.row - (i64)tv->animated_scroll.pos.y;
            }

            i64 move_y = tv->cells_per_page * dir;

            edith_textbuf_coord goto_pos = {
                .col = it->want.col,
                .row = it->want.row + tv->target_scroll.row + move_y,
            };

            tv->target_scroll.row += move_y;
            it->caret = edith_textbuf_pos_from_coord(&tv->tb, goto_pos);
        } break;

        case MOVE_BY_DOCUMENT: {
            if (dir == DIR_BACKWARD) it->caret = 0;
            if (dir == DIR_FORWARD)  it->caret = edith_textbuf_len(&tv->tb);
        } break;
    }

    if (!expand_selection) it->mark = it->caret;
    if (!keep_want_col) it->want.col = EDITH_CURSOR_NO_WANT_X;
    if (!keep_want_row) it->want.row = EDITH_CURSOR_NO_WANT_Y;
}

static void edith_textview_submit_move(edith_textview *tv, move_by by, dir dir, bool expand_selection) {
    if (!dir) return;
    if (!by) return;

    for_array (edith_cursor, it, tv->cursors) {
        edith_textview_submit_move_cursor(tv, by, dir, expand_selection, it);
    }

    edith_textview_cursors_dedup(tv);

    // NOTE(rune): MOVE_BY_PAGE scroll is handled in editor_move_cursor().
    if (by != MOVE_BY_PAGE) {
        edith_textview_scroll_to_primary_cursor(tv, EDITH_TEXTVIEW_SCROLL_TARGET_DEFAULT);
    }
}

static void edith_textview_move(edith_textview *tv, move_by by, dir dir, bool expand_selection) {
    edith_textview_submit_barrier(tv);
    edith_textview_submit_move(tv, by, dir, expand_selection);
}

////////////////////////////////////////////////////////////////
// rune: Submission

static void edith_textview_submit_u8(edith_textview *tv, u8 c) {
    str s = str_make(&c, 1);
    edith_textview_submit_str(tv, s);

    if (c == '\n') {
        edith_textview_submit_barrier(tv);
    }
}

static void edith_textview_submit_u32(edith_textview *tv, u32 c) {
    u8 utf8[4];
    i32 num_bytes = encode_single_utf8_codepoint(c, utf8);
    str s = str_make(utf8, num_bytes);
    edith_textview_submit_str(tv, s);

    if (c == '\n') {
        edith_textview_submit_barrier(tv);
    }
}

static void edith_textview_submit_str(edith_textview *tv, str s) {
    arena *temp = edith_thread_local_arena;
    arena_scope_begin(temp);

    edith_textview_submit_begin(tv);
    edith_textview_submit_delete_selected(tv);

    // rune: Move cursors and gather edits.
    edith_edit_list edit_list = { 0 };
    i64 offset = 0;
    for_array (edith_cursor, it, tv->cursors) {
        edith_edit *edit = edith_edit_list_push(&edit_list, temp);
        edit->pos = it->caret;
        edit->data = s;

        it->want              = EDITH_CURSOR_NO_WANT;
        it->caret            += offset + s.len;
        it->mark              = it->caret;

        offset += s.len;
    }

    // rune: Apply edit and scroll
    edith_edit_array edit_array = edith_edit_array_from_list(edit_list, EDITH_EDIT_KIND_INSERT, temp);
    edith_textbuf_apply_edit_array(&tv->tb, edit_array, EDITH_TEXTBUF_EDIT_FLAG_NO_HISTORY|EDITH_TEXTBUF_EDIT_FLAG_SUBMIT);
    edith_textview_cursors_dedup(tv);
    edith_textview_scroll_to_primary_cursor(tv, EDITH_TEXTVIEW_SCROLL_TARGET_DEFAULT);

    arena_scope_end(temp);
}

static void edith_textview_submit_str_list(edith_textview *tv, str_list strings) {
    arena *temp = edith_thread_local_arena;
    arena_scope_begin(temp);

    edith_textview_submit_begin(tv);
    edith_textview_submit_delete_selected(tv);

    // rune: Move cursor and gather edits.
    edith_edit_list edit_list = { 0 };
    prof_scope("cursors") {
        i64 offset = 0;
        str_node *node = strings.first;
        for_n (i64, i, tv->cursors.count) {
            edith_cursor *it = &tv->cursors.v[i];
            i64 total_len = 0;
            while (node) {
                str s = node->v;
                total_len += s.len;

                edith_edit *edit = edith_edit_list_push(&edit_list, temp);
                edit->pos = it->caret;
                edit->data = s;

                node = node->next;

                // NOTE(rune): Last cursor consumes all remaining strings.
                if (i + 1 != tv->cursors.count) {
                    break;
                }
            }

            it->want              = EDITH_CURSOR_NO_WANT;
            it->caret            += offset + total_len;
            it->mark              = it->caret;

            offset += total_len;
        }
    }

    // rune: Apply edit and scroll
    edith_edit_array edit_array = edith_edit_array_from_list(edit_list, EDITH_EDIT_KIND_INSERT, temp);
    edith_textbuf_apply_edit_array(&tv->tb, edit_array, EDITH_TEXTBUF_EDIT_FLAG_NO_HISTORY|EDITH_TEXTBUF_EDIT_FLAG_SUBMIT);
    edith_textview_cursors_dedup(tv);
    edith_textview_scroll_to_primary_cursor(tv, EDITH_TEXTVIEW_SCROLL_TARGET_DEFAULT);

    arena_scope_end(temp);
}

static void edith_editor_delete_selected(edith_textview *tv) {
    edith_textview_submit_barrier(tv);
    edith_textview_submit_delete_selected(tv);
    edith_textview_cursors_dedup(tv);
    edith_textview_submit_barrier(tv);
}

static void edith_textview_submit_delete_selected(edith_textview *tv) {
    arena *temp = edith_thread_local_arena;
    arena_scope_begin(temp);

    edith_textview_cursors_dedup(tv);

    edith_textview_submit_begin(tv);

    // rune: Move cursors and gather edits.
    i64 count = tv->cursors.count;
    edith_edit_list edit_list = { 0 };
    i64 offset = 0;
    for_n (i64, i, count) {
        edith_cursor *it = &tv->cursors.v[i];
        i64_range it_range = edith_cursor_range(*it);
        if (it_range.min != it_range.max) {
            edith_edit *edit = edith_edit_list_push(&edit_list, temp);
            edit->pos = it_range.min;
            edit->data = edith_str_from_gapbuffer_range(&tv->tb.gb, it_range, temp);
        }

        it->mark  = it_range.min - offset;
        it->caret = it_range.min - offset;

        offset += range_len(it_range);
    }

    // rune: Apply edits.
    edith_edit_array edit_array = edith_edit_array_from_list(edit_list, EDITH_EDIT_KIND_DELETE, temp);
    edith_textbuf_apply_edit_array(&tv->tb, edit_array, EDITH_TEXTBUF_EDIT_FLAG_NO_HISTORY|EDITH_TEXTBUF_EDIT_FLAG_SUBMIT);

    arena_scope_end(temp);

    // NOTE(rune): It is the callers responsibility to call editor_deduplicate_cursors()
    // if needed. This is to prevent all cursors being collapsed when beginning a submission,
    // when multiple cursors have touching selections.
}

static dir edith_invert_dir(dir dir) {
    switch (dir) {
        case DIR_FORWARD:  return DIR_BACKWARD;
        case DIR_BACKWARD: return DIR_FORWARD;
        default:           return 0;
    }
}

static void edith_textview_submit_delete(edith_textview *tv, move_by by, dir dir) {
    arena *temp = edith_thread_local_arena;
    arena_scope_begin(temp);

    edith_textview_submit_begin(tv);

    // rune: Move cursors with nothing selected.
    for_array (edith_cursor, cursor, tv->cursors) {
        if (cursor->caret == cursor->mark) {
            edith_textview_submit_move_cursor(tv, by, edith_invert_dir(dir), true, cursor);
        }
    }

    // rune: Apply edits.
    edith_textview_submit_delete_selected(tv);
    edith_textview_cursors_dedup(tv);
    edith_textview_scroll_to_primary_cursor(tv, EDITH_TEXTVIEW_SCROLL_TARGET_DEFAULT);

    arena_scope_end(temp);
}

static void edith_textview_unredo(edith_textview *tv, edith_unredo unredo) {
    edith_textview_submit_barrier(tv);
    edith_saved_cursor_array saved_cursors = edith_textbuf_unredo(&tv->tb, unredo);
    if (saved_cursors.tv == tv) {
        edith_textview_cursors_restore(tv, saved_cursors);
    }
}
