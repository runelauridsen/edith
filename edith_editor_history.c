////////////////////////////////////////////////////////////////
// rune: Debug

static void edith_debug_print_edit_history(edith_edit_history *history) {
    unused(history);
#if !RUN_TESTS
#if 0
    println(ANSI_FG_GRAY "========================================================================" ANSI_RESET);
    println("");
    println("");

    for (edith_edit_history_record *record = &history->first_record;
         record;
         record = record->next) {

        print("--------------------------------------------\n");
        if (record == history->at) {
            print(ANSI_BG_GRAY);
        }

        print("debug id %\n", record->debug_id);

        print("cursors min ");
        for_array (edith_saved_cursor, cursor, record->saved_cursors[SIDE_MIN]) print("% ", i64_range(cursor->caret, cursor->mark));
        print("\n");

        print("cursors max ");
        for_array (edith_saved_cursor, cursor, record->saved_cursors[SIDE_MAX]) print("% ", i64_range(cursor->caret, cursor->mark));
        print("\n");

        for_list (edith_edit_batch_node, item, record->batch) {
            switch (item->edits.kind) {
                case EDITH_EDIT_KIND_INSERT: print(ANSI_FG_GREEN); break;
                case EDITH_EDIT_KIND_DELETE: print(ANSI_FG_RED); break;
            }

            println("    %" ANSI_RESET, edith_str_from_edit_kind(item->edits.kind));

            for_array (edith_edit, edit, item->edits) {
                println("    pos = % \t len = % \t data = %(lit)", i64_range(edit->pos, edit->pos + edit->data.len), edit->data.len, edit->data);
            }

            println("");
        }

        print(ANSI_RESET);
    }
#endif
#endif
}

static void edith_sanity_check_history(edith_edit_history *h) {
    for (edith_edit_history_record *b = &h->first_record; b; b = b->next) {
        assert(b->next != b);
        assert(b->prev != b);

        if (b->next) assert(b->next->prev == b);
        if (b->prev) assert(b->prev->next == b);
    }
}

////////////////////////////////////////////////////////////////
// rune: Undo history

static bool edith_is_nil_record(edith_edit_history_record *r) {
    bool ret = r->debug_id == 0;
    return ret;
}

static void edith_edit_history_init(edith_edit_history *h) {
    h->at = &h->first_record;
}

static void edith_edit_history_begin_record(edith_edit_history *h) {
    edith_sanity_check_history(h);

    if (h->at->next) {
        h->at->next = null;
        edith_sanity_check_history(h);
        arena_mark_set(h->arena, h->at->arena_mark);
        edith_sanity_check_history(h);
    }

    h->at->arena_mark = arena_mark_get(h->arena);

    edith_sanity_check_history(h);
    edith_edit_history_record *r = arena_push_struct(h->arena, edith_edit_history_record);
    r->debug_id = ++h->debug_id_counter;
    h->at->next = r;
    r->prev = h->at;
    h->at = r;
    edith_sanity_check_history(h);
}

static void edith_edit_history_add_to_record(edith_edit_history *h, edith_edit_array new_edits) {
    edith_sanity_check_history(h);
    assert(h->at);

    if (new_edits.count > 0) {

        ////////////////////////////////////////////////
        // Copy edits to history storage.

        edith_edit_array copied_edits = edith_edit_array_copy(h->arena, new_edits, true);

        ////////////////////////////////////////////////
        // Add to current batch.

        edith_edit_batch_push(&h->at->batch, copied_edits, h->arena);
    }

    edith_sanity_check_history(h);
    edith_debug_print_edit_history(h);
}

static edith_edit_history_record *edith_edit_history_step(edith_edit_history *h, edith_unredo unredo) {
    edith_sanity_check_history(h);

    edith_edit_history_record *ret = null;
    switch (unredo) {
        case EDITH_UNREDO_UNDO: {
            if (!edith_is_nil_record(h->at)) {
                ret = h->at;
                h->at = h->at->prev;
            } else {
                ret = null;
            }
        } break;

        case EDITH_UNREDO_REDO: {
            if (h->at->next) {
                // Move to next batch.
                ret = h->at = h->at->next;
            } else {
                // No next batch -> stay at last batch.
                ret = null;
            }
        } break;
    }

    edith_sanity_check_history(h);
    edith_debug_print_edit_history(h);
    return ret;
}

////////////////////////////////////////////////////////////////
// rune: Saved cursors

static edith_saved_cursor_array edith_saved_cursor_array_reserve(i64 count, arena *arena) {
    edith_saved_cursor_array array = { 0 };
    if (count > 0) {
        array.v     = arena_push_array(arena, edith_saved_cursor, count);
        array.count = count;
    }
    return array;
}