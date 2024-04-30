////////////////////////////////////////////////////////////////
// rune: Test linemarks

static readonly str test_linemarks_text = STR(
    "typedef struct fuzzy_match fuzzy_match;\r\n"
    "struct fuzzy_match {\r\n"
    "    range_ix range;\r\n"
    "};\r\n"
    "\r\n"
    "static fuzzy_match get_fuzzy_match(str s, str pattern);\r\n"
    ""
);

static void test_assert_linemarks(str loc, edith_textview *editor) {
    test_assert(loc, edith_linemarks_sanity_check(&editor->tb.linemarks, &editor->tb.gb));
}

static void test_linemarks_before(edith_textview *editor) {
    edith_textview_create(editor);
    edith_textbuf_create(&editor->tb);
    test_textview_init_text(editor, test_linemarks_text);
}

static void test_linemarks_after(edith_textview *editor) {
    edith_textbuf_destroy(&editor->tb);
    edith_textview_destroy(editor);
}

static void test_linemarks(void) {
    edith_textview _editor = { 0 };
    edith_textview *editor = &_editor;

    test_ctx ctx = {
        .name   = str("Linemarks"),
        .before = test_linemarks_before,
        .after  = test_linemarks_after,
        .param  = editor,
    };

    test_ctx(&ctx) {
        ////////////////////////////////////////////////////////////////
        // rune: Test initial file

        test_scope("inital_file") {
            test_assert_linemarks(loc(), editor);
        }

        ////////////////////////////////////////////////////////////////
        // rune: Test insert

        test_scope("insert_single_char") {
            // Test insert single char.
            loop(50) edith_textview_move(editor, MOVE_BY_CHAR, DIR_FORWARD, false);
            loop(1)  edith_textview_submit_u8(editor, 'a');

            // Post condition
            test_assert_linemarks(loc(), editor);
        }

        test_scope("insert_multiple_chars") {

            // Test insert single char.
            loop(50) edith_textview_move(editor, MOVE_BY_CHAR, DIR_FORWARD, false);
            loop(1)  edith_textview_submit_u8(editor, 'a');

            // Test insert multiple chars.
            loop(1)  edith_textview_add_cursor_at_row_offset(editor, 1);
            loop(1)  edith_textview_submit_u8(editor, 'a');

            // Post condition
            test_assert_linemarks(loc(), editor);
        }

        test_scope("insert_multiple_chars_at_line_start") {
            // Test insert single char.
            loop(50) edith_textview_move(editor, MOVE_BY_CHAR, DIR_FORWARD, false);
            loop(1)  edith_textview_submit_u8(editor, 'a');

            // Test insert multiple chars.
            loop(1)  edith_textview_add_cursor_at_row_offset(editor, 1);
            loop(1)  edith_textview_submit_u8(editor, 'a');

            // Test insert multiple chars at line start.
            loop(12) edith_textview_move(editor, MOVE_BY_CHAR, DIR_BACKWARD, false);
            loop(1)  edith_textview_submit_u8(editor, 'a');

            // Post condition
            test_assert_linemarks(loc(), editor);
        }

        test_scope("insert_single_newline") {

            // Test insert single char.
            loop(50) edith_textview_move(editor, MOVE_BY_CHAR, DIR_FORWARD, false);
            loop(1)  edith_textview_submit_u8(editor, 'a');

            // Test insert multiple chars.
            loop(1)  edith_textview_add_cursor_at_row_offset(editor, 1);
            loop(1)  edith_textview_submit_u8(editor, 'a');

            // Test insert multiple chars at line start.
            loop(12) edith_textview_move(editor, MOVE_BY_CHAR, DIR_BACKWARD, false);
            loop(1)  edith_textview_submit_u8(editor, 'a');

            // Test insert single newline.
            loop(1)  edith_textview_cursors_clear_secondary(editor);
            loop(1)  edith_textview_move(editor, MOVE_BY_LINE, DIR_BACKWARD, false);
            loop(10) edith_textview_move(editor, MOVE_BY_CHAR, DIR_FORWARD, false);
            loop(1)  edith_textview_submit_u8(editor, '\n');

            // Post condition
            test_assert_linemarks(loc(), editor);
        }

        test_scope("insert_multiple_newlines") {

            // Test insert single char.
            loop(50) edith_textview_move(editor, MOVE_BY_CHAR, DIR_FORWARD, false);
            loop(1)  edith_textview_submit_u8(editor, 'a');

            // Test insert multiple chars.
            loop(1)  edith_textview_add_cursor_at_row_offset(editor, 1);
            loop(1)  edith_textview_submit_u8(editor, 'a');

            // Test insert multiple chars at line start.
            loop(12) edith_textview_move(editor, MOVE_BY_CHAR, DIR_BACKWARD, false);
            loop(1)  edith_textview_submit_u8(editor, 'a');

            // Test insert single newline.
            loop(1)  edith_textview_cursors_clear_secondary(editor);
            loop(1)  edith_textview_move(editor, MOVE_BY_LINE, DIR_BACKWARD, false);
            loop(10) edith_textview_move(editor, MOVE_BY_CHAR, DIR_FORWARD, false);
            loop(1)  edith_textview_submit_u8(editor, '\n');

            // Test insert multiple newlines.
            loop(10) edith_textview_move(editor, MOVE_BY_CHAR, DIR_FORWARD, false);
            loop(1)  edith_textview_add_cursor_at_row_offset(editor, -1);
            loop(1)  edith_textview_submit_u8(editor, '\n');

            // Post condition
            test_assert_linemarks(loc(), editor);
        }

        test_scope("insert_multiple_newlines_at_line_start") {

            // Test insert single char.
            loop(50) edith_textview_move(editor, MOVE_BY_CHAR, DIR_FORWARD, false);
            loop(1)  edith_textview_submit_u8(editor, 'a');

            // Test insert multiple chars.
            loop(1)  edith_textview_add_cursor_at_row_offset(editor, 1);
            loop(1)  edith_textview_submit_u8(editor, 'a');

            // Test insert multiple chars at line start.
            loop(12) edith_textview_move(editor, MOVE_BY_CHAR, DIR_BACKWARD, false);
            loop(1)  edith_textview_submit_u8(editor, 'a');

            // Test insert single newline.
            loop(1)  edith_textview_cursors_clear_secondary(editor);
            loop(1)  edith_textview_move(editor, MOVE_BY_LINE, DIR_BACKWARD, false);
            loop(10) edith_textview_move(editor, MOVE_BY_CHAR, DIR_FORWARD, false);
            loop(1)  edith_textview_submit_u8(editor, '\n');

            // Test insert multiple newlines.
            loop(10) edith_textview_move(editor, MOVE_BY_CHAR, DIR_FORWARD, false);
            loop(1)  edith_textview_add_cursor_at_row_offset(editor, -1);
            loop(1)  edith_textview_submit_u8(editor, '\n');

            // Test insert multiple newlines at line start.
            loop(2)  edith_textview_move(editor, MOVE_BY_HOME_END, DIR_BACKWARD, false);
            loop(1)  edith_textview_submit_u8(editor, '\n');

            // Post condition
            test_assert_linemarks(loc(), editor);
        }

        test_scope("insert_multiple_chars_on_same_line") {
            loop(10) edith_textview_move(editor, MOVE_BY_CHAR, DIR_FORWARD, false);
            loop(1)  edith_textview_add_cursor_at_row_offset(editor, 1);
            loop(1)  edith_textview_move(editor, MOVE_BY_WORD, DIR_BACKWARD, false);
            loop(1)  edith_textview_move(editor, MOVE_BY_LINE, DIR_BACKWARD, false);
            loop(1)  edith_textview_submit_u8(editor, 'a');

            test_assert_linemarks(loc(), editor);
        }

        test_scope("insert_single_char_at_end_of_document") {
            loop(2) edith_textview_move(editor, MOVE_BY_PARAGRAPH, DIR_FORWARD, false);
            loop(1) edith_textview_submit_u8(editor, 'a');

            test_assert_linemarks(loc(), editor);
        }

        test_scope("insert_single_newline_at_end_of_document") {
            loop(2) edith_textview_move(editor, MOVE_BY_PARAGRAPH, DIR_FORWARD, false);
            loop(1) edith_textview_submit_u8(editor, '\n');

            test_assert_linemarks(loc(), editor);
        }

        test_scope("insert_four_chars") {
            loop(50) edith_textview_move(editor, MOVE_BY_CHAR, DIR_FORWARD, false);
            loop(1)  edith_textview_add_cursor_at_row_offset(editor, 1);
            loop(2)  edith_textview_move(editor, MOVE_BY_HOME_END, DIR_BACKWARD, false);
            loop(2)  edith_textview_add_cursor_at_row_offset(editor, 1);
            loop(1)  edith_textview_submit_u8(editor, 'a');

            test_assert_linemarks(loc(), editor);
        }

        ////////////////////////////////////////////////////////////////
        // rune: Test delete

        test_scope("delete_single_char") {
            loop(50) edith_textview_move(editor, MOVE_BY_CHAR, DIR_FORWARD, false);
            loop(1)  edith_textview_submit_delete(editor, MOVE_BY_CHAR, DIR_BACKWARD);

            test_assert_linemarks(loc(), editor);
        }

        test_scope("delete_multiple_chars") {
            loop(50) edith_textview_move(editor, MOVE_BY_CHAR, DIR_FORWARD, false);
            loop(1)  edith_textview_add_cursor_at_row_offset(editor, 1);
            loop(1)  edith_textview_submit_delete(editor, MOVE_BY_CHAR, DIR_BACKWARD);

            test_assert_linemarks(loc(), editor);
        }

        test_scope("delete_multiple_chars_at_line_start") {
            loop(50) edith_textview_move(editor, MOVE_BY_CHAR, DIR_FORWARD, false);
            loop(1)  edith_textview_add_cursor_at_row_offset(editor, 1);
            loop(2)  edith_textview_move(editor, MOVE_BY_HOME_END, DIR_BACKWARD, false);
            loop(1)  edith_textview_submit_delete(editor, MOVE_BY_CHAR, DIR_BACKWARD);

            test_assert_linemarks(loc(), editor);
        }

        test_scope("delete_selection") {
            loop(52) edith_textview_move(editor, MOVE_BY_CHAR, DIR_FORWARD, false);
            loop(30) edith_textview_move(editor, MOVE_BY_CHAR, DIR_FORWARD, true);
            loop(1)  edith_textview_submit_delete(editor, MOVE_BY_CHAR, DIR_BACKWARD);

            test_assert_linemarks(loc(), editor);
        }

        test_scope("delete_single_newline_at_end_of_document") {
            loop(2) edith_textview_move(editor, MOVE_BY_PARAGRAPH, DIR_FORWARD, false);
            loop(1) edith_textview_submit_delete(editor, MOVE_BY_CHAR, DIR_BACKWARD);

            test_assert_linemarks(loc(), editor);
        }
    }
}
