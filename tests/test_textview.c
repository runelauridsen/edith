////////////////////////////////////////////////////////////////
// rune: Test textview

static void test_textview_init_text(edith_textview *tv, str initial_text) {
    edith_edit edit = { 0, initial_text };
    edith_textbuf_apply_edit(&tv->tb, edit, EDITH_EDIT_KIND_INSERT, EDITH_TEXTBUF_EDIT_FLAG_NO_HISTORY);
}

static void test_assert_caret(str loc, edith_textview *tv, i64 caret) {
    test_assert_eq(loc, tv->cursors.v[0].caret, caret);
}

static void test_textview_before(void *param) {
    edith_textview *tv = param;
    edith_textview_create(tv);
    edith_textbuf_create(&tv->tb);
}

static void test_textview_after(void *param) {
    edith_textview *tv = param;
    edith_textview_destroy(tv);
    edith_textbuf_destroy(&tv->tb);
}

static void test_textview(void) {
    edith_textview _tv = { 0 };
    edith_textview *tv = &_tv;

    test_ctx ctx = { 0 };
    ctx.name = str("tv");
    ctx.before = test_textview_before;
    ctx.after  = test_textview_after;
    ctx.param  = tv;

    test_ctx(&ctx) {

        test_scope("undo_cursor_restore") {
            test_textview_init_text(tv, str("initial text"));

            tv->cursors.v[0].caret = 3;
            tv->cursors.v[0].mark  = 6;
            edith_textview_submit_delete(tv, MOVE_BY_CHAR, DIR_FORWARD);

            tv->cursors.v[0].caret = 1;
            tv->cursors.v[0].mark  = 1;
            edith_textview_unredo(tv, EDITH_UNREDO_UNDO);

            test_assert_eq(loc(), tv->cursors.v[0].caret, 3);
            test_assert_eq(loc(), tv->cursors.v[0].mark, 6);
        }

        test_scope("undo_redo_cursor_restore") {
            test_textview_init_text(tv, str("initial text"));

            tv->cursors.v[0].caret = 3;
            tv->cursors.v[0].mark  = 6;
            edith_textview_submit_delete(tv, MOVE_BY_CHAR, DIR_FORWARD);

            edith_textview_move_to_pos(tv, 2, false);
            tv->cursors.v[0].caret = 2;
            tv->cursors.v[0].mark  = 4;
            edith_textview_unredo(tv, EDITH_UNREDO_UNDO);

            test_assert_eq(loc(), tv->cursors.v[0].caret, 3);
            test_assert_eq(loc(), tv->cursors.v[0].mark, 6);

            tv->cursors.v[0].caret = 7;
            tv->cursors.v[0].mark  = 7;
            edith_textview_unredo(tv, EDITH_UNREDO_REDO);

            test_assert_eq(loc(), tv->cursors.v[0].caret, 3);
            test_assert_eq(loc(), tv->cursors.v[0].mark , 3);
        }

        test_scope("undo_redo_side_restore") {
            test_textview_init_text(tv, str("////////////////////////////////////////////////////////////////"));

            edith_textview_move(tv, MOVE_BY_CHAR, DIR_FORWARD, false);
            edith_textview_move(tv, MOVE_BY_CHAR, DIR_FORWARD, false);
            edith_textview_move(tv, MOVE_BY_CHAR, DIR_FORWARD, false); // caret == 3

            edith_textview_submit_u8(tv, 'a');
            edith_textview_submit_u8(tv, 'a');
            edith_textview_submit_u8(tv, 'a');
            edith_textview_submit_u8(tv, 'a');
            edith_textview_submit_u8(tv, 'a');
            edith_textview_submit_u8(tv, 'a'); // caret == 9

            edith_textview_move(tv, MOVE_BY_CHAR, DIR_FORWARD, false);
            edith_textview_move(tv, MOVE_BY_CHAR, DIR_FORWARD, false);
            edith_textview_move(tv, MOVE_BY_CHAR, DIR_FORWARD, false); // caret == 12

            edith_textview_submit_u8(tv, 'a');
            edith_textview_submit_u8(tv, 'a');
            edith_textview_move(tv, MOVE_BY_CHAR, DIR_FORWARD, false); // caret == 15

            test_assert_caret(loc(), tv, 15);
            edith_textview_unredo(tv, EDITH_UNREDO_UNDO);
            test_assert_caret(loc(), tv, 12);
            edith_textview_unredo(tv, EDITH_UNREDO_UNDO);
            test_assert_caret(loc(), tv, 3);
            edith_textview_unredo(tv, EDITH_UNREDO_REDO);
            test_assert_caret(loc(), tv, 9);
        }

        test_scope("paragraph_move_past_start") {
            test_textview_init_text(tv, str("aaaaaa\n\nbbbbbb\n\ncccccc\n"));
            tv->cursors.v[0].caret = 17;

            edith_textview_move(tv, MOVE_BY_PARAGRAPH, DIR_BACKWARD, false);
            test_assert_caret(loc(), tv, 15);

            edith_textview_move(tv, MOVE_BY_PARAGRAPH, DIR_BACKWARD, false);
            test_assert_caret(loc(), tv, 7);

            edith_textview_move(tv, MOVE_BY_PARAGRAPH, DIR_BACKWARD, false);
            test_assert_caret(loc(), tv, 0);

            edith_textview_move(tv, MOVE_BY_PARAGRAPH, DIR_BACKWARD, false);
            test_assert_caret(loc(), tv, 0);
        }

        test_scope("paragraph_move_past_end") {
            test_textview_init_text(tv, str("aaaaaa\n\nbbbbbb\n\ncccccc\n"));
            tv->cursors.v[0].caret = 4;

            edith_textview_move(tv, MOVE_BY_PARAGRAPH, DIR_FORWARD, false);
            test_assert_caret(loc(), tv, 7);

            edith_textview_move(tv, MOVE_BY_PARAGRAPH, DIR_FORWARD, false);
            test_assert_caret(loc(), tv, 15);

            edith_textview_move(tv, MOVE_BY_PARAGRAPH, DIR_FORWARD, false);
            test_assert_caret(loc(), tv, 23);

            edith_textview_move(tv, MOVE_BY_PARAGRAPH, DIR_FORWARD, false);
            test_assert_caret(loc(), tv, 23);
        }

        test_scope("delete_near_overlapping_selections") {
            test_textview_init_text(tv, str("aaabbbcccdddeeefff"));
            tv->cursors.v[0].caret = 3;
            tv->cursors.v[0].mark  = 3;

            edith_textview_add_cursor_at_pos(tv, 6, 6, true, null);
            edith_textview_add_cursor_at_pos(tv, 9, 9, true, null);

            loop(3) edith_textview_move(tv, MOVE_BY_CHAR, DIR_FORWARD, true);

            edith_textview_submit_delete(tv, MOVE_BY_CHAR, DIR_BACKWARD);

            test_assert_gapbuffer(loc(), &tv->tb.gb, str("aaaeeefff"));
        }

        test_scope("delete_overlapping_selections") {
            test_textview_init_text(tv, str("aaabbbcccdddeeefff"));
            tv->cursors.v[0].caret = 3;
            tv->cursors.v[0].mark  = 3;

            edith_textview_add_cursor_at_pos(tv, 6, 6, true, null);
            edith_textview_add_cursor_at_pos(tv, 9, 9, true, null);

            loop(5) edith_textview_move(tv, MOVE_BY_CHAR, DIR_FORWARD, true);

            edith_textview_submit_delete(tv, MOVE_BY_CHAR, DIR_BACKWARD);

            test_assert_gapbuffer(loc(), &tv->tb.gb, str("aaaefff"));
        }

        test_scope("delete_overlapping_selections_crazy") {
            test_textview_init_text(tv, str("aaabbbcccdddeeefffggg"));
            tv->cursors.v[0].caret = 3;
            tv->cursors.v[0].mark  = 3;

            edith_textview_add_cursor_at_pos(tv, 6, 6, true, null);
            edith_textview_add_cursor_at_pos(tv, 9, 9, true, null);
            edith_textview_add_cursor_at_pos(tv, 12, 12, true, null);
            edith_textview_add_cursor_at_pos(tv, 15, 15, true, null);

            //     ________________     _____
            //     ||  | | |  |   |     |   |
            // [0] 3|__6 | |  |   |     |   |
            // [1]  4____|_|__12  |     |   |
            // [2]       8_10 |   |     |   |
            // [3]            12__14    |   |
            // [4]                      16__18

            tv->cursors.v[0].caret = 3;
            tv->cursors.v[0].mark  = 6;

            tv->cursors.v[1].caret = 4;
            tv->cursors.v[1].mark  = 12;

            tv->cursors.v[2].caret = 8;
            tv->cursors.v[2].mark  = 10;

            tv->cursors.v[3].caret = 12;
            tv->cursors.v[3].mark  = 14;

            tv->cursors.v[4].caret = 16;
            tv->cursors.v[4].mark  = 18;

            edith_textview_submit_delete(tv, MOVE_BY_CHAR, DIR_BACKWARD);

            test_assert_gapbuffer(loc(), &tv->tb.gb, str("aaaefggg"));
        }

        test_scope("undo_redo_cutoff_future") {
            test_textview_init_text(tv, str(""));

            edith_textview_submit_u8(tv, 'a'); edith_textview_submit_u8(tv, '\n');
            edith_textview_submit_u8(tv, 'b'); edith_textview_submit_u8(tv, '\n');
            edith_textview_submit_u8(tv, 'c'); edith_textview_submit_u8(tv, '\n');
            edith_textview_submit_u8(tv, 'd'); edith_textview_submit_u8(tv, '\n');
            edith_textview_submit_u8(tv, 'e'); edith_textview_submit_u8(tv, '\n');
            test_assert_gapbuffer(loc(), &tv->tb.gb, str("a\nb\nc\nd\ne\n"));

            edith_textview_unredo(tv, EDITH_UNREDO_UNDO);
            edith_textview_unredo(tv, EDITH_UNREDO_UNDO);
            edith_textview_unredo(tv, EDITH_UNREDO_UNDO);
            test_assert_gapbuffer(loc(), &tv->tb.gb, str("a\nb\n"));

            edith_textview_unredo(tv, EDITH_UNREDO_REDO);
            edith_textview_unredo(tv, EDITH_UNREDO_REDO);
            test_assert_gapbuffer(loc(), &tv->tb.gb, str("a\nb\nc\nd\n"));

            edith_textview_unredo(tv, EDITH_UNREDO_UNDO);
            edith_textview_unredo(tv, EDITH_UNREDO_UNDO);
            test_assert_gapbuffer(loc(), &tv->tb.gb, str("a\nb\n"));

            edith_textview_submit_u8(tv, 'x'); edith_textview_submit_u8(tv, '\n');
            edith_textview_submit_u8(tv, 'y'); edith_textview_submit_u8(tv, '\n');
            test_assert_gapbuffer(loc(), &tv->tb.gb, str("a\nb\nx\ny\n"));

            edith_textview_unredo(tv, EDITH_UNREDO_REDO);
            test_assert_gapbuffer(loc(), &tv->tb.gb, str("a\nb\nx\ny\n"));

            edith_textview_unredo(tv, EDITH_UNREDO_UNDO);
            test_assert_gapbuffer(loc(), &tv->tb.gb, str("a\nb\nx\n"));
            edith_textview_unredo(tv, EDITH_UNREDO_UNDO);
            test_assert_gapbuffer(loc(), &tv->tb.gb, str("a\nb\n"));

            edith_textview_unredo(tv, EDITH_UNREDO_UNDO);
            test_assert_gapbuffer(loc(), &tv->tb.gb, str("a\n"));
        }

        test_scope("undo_redo_past_all") {
            test_textview_init_text(tv, str(""));

            edith_textview_submit_u8(tv, 'a');
            edith_textview_submit_u8(tv, '\n');
            edith_textview_submit_u8(tv, 'b');
            edith_textview_submit_u8(tv, '\n');
            edith_textview_submit_u8(tv, 'c');
            edith_textview_submit_u8(tv, '\n'); test_assert_gapbuffer(loc(), &tv->tb.gb, str("a\nb\nc\n"));

            edith_textview_unredo(tv, EDITH_UNREDO_UNDO); test_assert_gapbuffer(loc(), &tv->tb.gb, str("a\nb\n"));
            edith_textview_unredo(tv, EDITH_UNREDO_UNDO); test_assert_gapbuffer(loc(), &tv->tb.gb, str("a\n"));
            edith_textview_unredo(tv, EDITH_UNREDO_UNDO); test_assert_gapbuffer(loc(), &tv->tb.gb, str(""));

            edith_textview_unredo(tv, EDITH_UNREDO_UNDO);
            edith_textview_unredo(tv, EDITH_UNREDO_UNDO);
            edith_textview_unredo(tv, EDITH_UNREDO_UNDO);
            edith_textview_unredo(tv, EDITH_UNREDO_UNDO); test_assert_gapbuffer(loc(), &tv->tb.gb, str(""));

            edith_textview_unredo(tv, EDITH_UNREDO_REDO); test_assert_gapbuffer(loc(), &tv->tb.gb, str("a\n"));
            edith_textview_unredo(tv, EDITH_UNREDO_REDO); test_assert_gapbuffer(loc(), &tv->tb.gb, str("a\nb\n"));

            edith_textview_unredo(tv, EDITH_UNREDO_REDO); test_assert_gapbuffer(loc(), &tv->tb.gb, str("a\nb\nc\n"));

            edith_textview_unredo(tv, EDITH_UNREDO_REDO);
            edith_textview_unredo(tv, EDITH_UNREDO_REDO);
            edith_textview_unredo(tv, EDITH_UNREDO_REDO);
            edith_textview_unredo(tv, EDITH_UNREDO_REDO);
            edith_textview_unredo(tv, EDITH_UNREDO_REDO);
            edith_textview_unredo(tv, EDITH_UNREDO_REDO); test_assert_gapbuffer(loc(), &tv->tb.gb, str("a\nb\nc\n"));

            edith_textview_unredo(tv, EDITH_UNREDO_UNDO); test_assert_gapbuffer(loc(), &tv->tb.gb, str("a\nb\n"));
            edith_textview_unredo(tv, EDITH_UNREDO_UNDO); test_assert_gapbuffer(loc(), &tv->tb.gb, str("a\n"));
            edith_textview_unredo(tv, EDITH_UNREDO_UNDO); test_assert_gapbuffer(loc(), &tv->tb.gb, str(""));
        }

        test_scope("overlapping_selections") {
            static readonly str init_text = STR(
                "typedef struct fuzzy_match fuzzy_match;\r\n"
                "struct fuzzy_match {\r\n"
                "    range_ix range;\r\n"
                "};\r\n"
                "\r\n"
                "static fuzzy_match get_fuzzy_match(str s, str pattern);\r\n"
            );

            static readonly str after_text = STR(
                "typedef struct fuzzy_match fuzzy_match;\r\n"
                "struct aaaa};aaaas, str pattern);\r\n"
            );

            test_textview_init_text(tv, init_text);

            tv->cursors.v[0].caret = 48;
            tv->cursors.v[0].mark  = 48;

            loop(4) edith_textview_add_cursor_at_row_offset(tv, 1);
            loop(4) edith_textview_move(tv, MOVE_BY_WORD, DIR_FORWARD, true);

            loop(1) edith_textview_submit_delete(tv, MOVE_BY_CHAR, DIR_BACKWARD);
            loop(1) edith_textview_unredo(tv, EDITH_UNREDO_UNDO);

            test_assert_linemarks(loc(), tv);
            test_assert_gapbuffer(loc(), &tv->tb.gb, init_text);

            // NOTE(rune): Since the cursors overlap, they sould've been
            // collapsed to 2 cursors and we should only get 2 "aaaa"s.
            loop(4) edith_textview_submit_u8(tv, 'a');
            test_assert_linemarks(loc(), tv);
            test_assert_gapbuffer(loc(), &tv->tb.gb, after_text);

            loop(1) edith_textview_unredo(tv, EDITH_UNREDO_UNDO);
            test_assert_linemarks(loc(), tv);
            test_assert_gapbuffer(loc(), &tv->tb.gb, init_text);

            loop(1) edith_textview_unredo(tv, EDITH_UNREDO_REDO);
            test_assert_linemarks(loc(), tv);
            test_assert_gapbuffer(loc(), &tv->tb.gb, after_text);
        }
    }
}
