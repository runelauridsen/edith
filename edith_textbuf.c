////////////////////////////////////////////////////////////////
// rune: Lifetime

static void edith_textbuf_create(edith_textbuf *tb) {
    zero_struct(tb);
    tb->submit.arena  = edith_arena_create_default(str("textbuf/submit"));
    tb->history.arena = edith_arena_create_default(str("textbuf/history"));
    edith_gapbuffer_create(&tb->gb, kilobytes(4), str("textbuf/gapbuffer"));
    edith_gb64_create(&tb->linemarks.pos_buf, str("textbuf/linemarks"));
    edith_edit_history_init(&tb->history);
}

static void edith_textbuf_destroy(edith_textbuf *tb) {
    edith_arena_destroy(tb->history.arena);
    edith_arena_destroy(tb->submit.arena);

    edith_gapbuffer_destroy(&tb->gb);
    edith_gb64_destroy(&tb->linemarks.pos_buf);
}

////////////////////////////////////////////////////////////////
// rune: Editing

static void edith_textbuf_insert(edith_textbuf *tb, i64 pos, str data, edith_textbuf_edit_flags flags) {
    edith_edit edit = { pos, data };
    edith_textbuf_apply_edit(tb, edit, EDITH_EDIT_KIND_INSERT, flags);
}

static void edith_textbuf_apply_edit(edith_textbuf *tb, edith_edit edit, edith_edit_kind kind, edith_textbuf_edit_flags flags) {
    edith_edit_array array = { 0 };
    array.v     = &edit;
    array.count = 1;
    array.kind  = kind;

    edith_textbuf_apply_edit_array(tb, array, flags);
}

static void edith_textbuf_apply_edit_array(edith_textbuf *tb, edith_edit_array edits, edith_textbuf_edit_flags flags) {
    YO_PROFILE_BEGIN(edith_textbuf_apply_edit_array);
    if (edits.count > 0) {
        edith_gapbuffer_apply_edits(&tb->gb, edits);
        edith_linemarks_apply_edits(&tb->linemarks, edits);

        lang_c_apply_edits(&tb->indexer, &tb->gb, edits); // TODO(rune): Abstraction-layer to support multiple languages

        if (flags & EDITH_TEXTBUF_EDIT_FLAG_SUBMIT) {
            assert(tb->submit.submit_in_progress == true);
            edith_edit_array copy = edith_edit_array_copy(tb->submit.arena, edits, true);
            edith_edit_batch_push(&tb->submit.waiting_for_commit, copy, tb->submit.arena);
            tb->submit.submit_in_progress = true;
        }
    }
    YO_PROFILE_END(edith_textbuf_apply_edit_array);
}

static bool edith_textbuf_commit(edith_textbuf *tb) {
    bool ret = false;

    if (tb->submit.submit_in_progress) {
        //edith_edit_history_begin_record(&tb->history);

        edith_edit_batch consolidated = edith_edit_batch_consolidate(tb->submit.waiting_for_commit, tb->submit.arena);

        //print(ANSI_FG_BRIGHT_MAGENTA "comitted\n" ANSI_RESET);
        //edith_print_edit_batch(consolidated);

        for_list (edith_edit_batch_node, node, consolidated) {
            edith_edit_history_add_to_record(&tb->history, node->edits);
        }

        ret = true;

        tb->submit.submit_in_progress       = false;
        tb->submit.waiting_for_commit.first = 0;
        tb->submit.waiting_for_commit.last  = 0;
        arena_reset(tb->submit.arena);
    }

    return ret;
}

static edith_saved_cursor_array edith_textbuf_unredo(edith_textbuf *tb, edith_unredo unredo) {
    arena *temp = edith_thread_local_arena;
    arena_scope_begin(temp);

    edith_saved_cursor_array restore_cursors = { 0 };
    edith_edit_history_record *record = edith_edit_history_step(&tb->history, unredo);
    if (record) {
        switch (unredo) {
            case EDITH_UNREDO_UNDO: {
                for_list_rev (edith_edit_batch_node, item, record->batch) {
                    edith_edit_array inverted = edith_edit_array_invert(item->edits, temp);
                    edith_textbuf_apply_edit_array(tb, inverted, EDITH_TEXTBUF_EDIT_FLAG_NO_HISTORY);
                }

                restore_cursors = record->saved_cursors[SIDE_MIN];
            } break;

            case EDITH_UNREDO_REDO: {
                for_list (edith_edit_batch_node, item, record->batch) {
                    edith_textbuf_apply_edit_array(tb, item->edits, EDITH_TEXTBUF_EDIT_FLAG_NO_HISTORY);
                }

                restore_cursors = record->saved_cursors[SIDE_MAX];
            } break;
        }
    }

    arena_scope_end(temp);

    edith_saved_cursor_array *_restore_cursors = &restore_cursors;
    return *_restore_cursors;
}

static i64 edith_textbuf_len(edith_textbuf *t) {
    i64 len = edith_gapbuffer_len(&t->gb);
    return len;
}

////////////////////////////////////////////////////////////////
// rune: Coordinates

static edith_textbuf_coord edith_textbuf_coord_make(i64 x, i64 y) {
    edith_textbuf_coord ret = { x, y };
    return ret;
}

static i64 edith_textbuf_row_count(edith_textbuf *tb) {
    i64 count = edith_gb64_count(&tb->linemarks.pos_buf);
    return count;
}

static i64 edith_textbuf_pos_from_row(edith_textbuf *tb, i64 row) {
    i64 pos = 0;
    if (row < 0) {
        pos = 0;
    } else if (row >= edith_textbuf_row_count(tb)) {
        pos = edith_gapbuffer_len(&tb->gb);
    } else {
        pos = edith_pos_from_linenumber(&tb->linemarks, row);
    }
    return pos;
}

static i64_range edith_textbuf_pos_range_from_row(edith_textbuf *tb, i64 row) {
    i64 min  = edith_textbuf_pos_from_row(tb, row);
    i64 max  = edith_textbuf_pos_from_row(tb, row + 1);
    i64_range ret = { min, max };
    return ret;
}

static i64 edith_textbuf_col_from_pos(edith_textbuf *tb, i64 pos) {
    YO_PROFILE_BEGIN(edith_textbuf_col_from_pos);
    pos = min(pos, edith_gapbuffer_len(&tb->gb));

    i64 col = 0;
    edith_doc_iter iter = { 0 };
    edith_doc_iter_jump(&iter, &tb->gb, pos);
    while (1) {
        edith_doc_iter_next(&iter, DIR_BACKWARD);
        if (iter.valid == false || iter.codepoint == '\n') {
            break;
        }

        col += 1;
    }
    YO_PROFILE_END(edith_textbuf_col_from_pos);
    return col;
}

static i64 edith_textbuf_row_from_pos(edith_textbuf *tb, i64 pos) {
    i64 linenumber = edith_linenumber_from_pos(&tb->linemarks, pos);
    return linenumber;
}

static edith_textbuf_coord edith_textbuf_coord_from_pos(edith_textbuf *tb, i64 pos) {
    YO_PROFILE_BEGIN(edith_textbuf_coord_from_pos);
    i64 col = edith_textbuf_col_from_pos(tb, pos);
    i64 row = edith_textbuf_row_from_pos(tb, pos);
    YO_PROFILE_END(edith_textbuf_coord_from_pos);
    return edith_textbuf_coord_make(row, col);
}

static i64 edith_textbuf_pos_from_coord(edith_textbuf *tb, edith_textbuf_coord pos) {
    YO_PROFILE_BEGIN(edith_textbuf_pos_from_coord);

    i64 line_number = pos.row;
    i64 line_start_pos = edith_textbuf_pos_from_row(tb, line_number);

    edith_doc_iter iter = { 0 };
    edith_doc_iter_jump(&iter, &tb->gb, line_start_pos);
    edith_doc_iter_by_cols(&iter, pos.col);

    YO_PROFILE_END(edith_textbuf_pos_from_coord);
    return iter.pos;
}
