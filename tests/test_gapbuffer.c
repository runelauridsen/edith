////////////////////////////////////////////////////////////////
// rune: Test gapbuffer

static void test_assert_gapbuffer(str loc, edith_gapbuffer *param, str expect) {
    edith_gapbuffer *gb = param;
    str actual = edith_doc_concat_fulltext(gb, test_arena());
    test_assert_eq(loc, actual, expect);
}

static void test_assert_buf(str loc, buf actual, str expect) {
    test_assert_eq(loc, buf_as_str(actual), expect);
}

static void test_assert_doc_iter(str loc, edith_gapbuffer *gb, i64 begin, i32 *dirs, u64 dir_count, str expect, u64 expect_pos) {
    u8 storage[1024];
    buf buffer = make_buf(storage, sizeof(storage), 0);

    edith_doc_iter iter = { 0 };
    edith_doc_iter_jump(&iter, gb, begin);

    for_narray (i32, it, dirs, dir_count) {
        if (iter.valid) {
            buf_append_utf8_codepoint(&buffer, iter.codepoint);
        }

        edith_doc_iter_next(&iter, (dir)*it);
    }

    test_assert_buf(loc, buffer, expect);
    test_assert_eq(loc, iter.pos, expect_pos);
}

static void test_gapbuffer_before(void *param) {
    edith_gapbuffer *gb = param;
    edith_gapbuffer_create(gb, 1024, str("test gapbuffer"));
}

static void test_gapbuffer_after(void *param) {
    edith_gapbuffer *gb = param;
    edith_gapbuffer_destroy(gb);
}

static void test_gapbuffer(void) {
    edith_gapbuffer _gb = { 0 };
    edith_gapbuffer *gb = &_gb;

    test_ctx ctx = { 0 };
    ctx.name   = str("Gapbuffer");
    ctx.param  = gb;
    ctx.before = test_gapbuffer_before;
    ctx.after  = test_gapbuffer_after;

    test_ctx(&ctx) {

        ////////////////////////////////////////////////////////////////
        // rune: Gapbuffer insert/delete

        test_scope("insert") {
            edith_gapbuffer_insert(gb, 0, str("aaa"));

            test_assert_gapbuffer(loc(), gb, str("aaa"));
        }

        test_scope("insert_after") {
            edith_gapbuffer_insert(gb, 0, str("aaa"));
            edith_gapbuffer_insert(gb, 3, str("bbbbb"));

            test_assert_gapbuffer(loc(), gb, str("aaabbbbb"));
        }

        test_scope("insert_split") {
            edith_gapbuffer_insert(gb, 0, str("aaa"));
            edith_gapbuffer_insert(gb, 2, str("bbbbb"));

            test_assert_gapbuffer(loc(), gb, str("aabbbbba"));
        }

        test_scope("insert_mid") {
            edith_gapbuffer_insert(gb, 0, str("aaa"));
            edith_gapbuffer_insert(gb, 3, str("bbbbb"));
            edith_gapbuffer_insert(gb, 3, str("ccccccc"));

            test_assert_gapbuffer(loc(), gb, str("aaacccccccbbbbb"));
        }

        test_scope("insert_start") {
            edith_gapbuffer_insert(gb, 0, str("aaa"));
            edith_gapbuffer_insert(gb, 3, str("bbbbb"));
            edith_gapbuffer_insert(gb, 0, str("ccccccc"));

            test_assert_gapbuffer(loc(), gb, str("cccccccaaabbbbb"));
        }

        test_scope("insert_end") {
            edith_gapbuffer_insert(gb, 0, str("aaa"));
            edith_gapbuffer_insert(gb, 3, str("bbbbb"));
            edith_gapbuffer_insert(gb, 8, str("ccccccc"));

            test_assert_gapbuffer(loc(), gb, str("aaabbbbbccccccc"));
        }

        test_scope("insert_split_start") {
            edith_gapbuffer_insert(gb, 0, str("aaa"));
            edith_gapbuffer_insert(gb, 2, str("bbbbb"));
            edith_gapbuffer_insert(gb, 0, str("ccccccc"));

            test_assert_gapbuffer(loc(), gb, str("cccccccaabbbbba"));
        }

        test_scope("insert_split_end") {
            edith_gapbuffer_insert(gb, 0, str("aaa"));
            edith_gapbuffer_insert(gb, 2, str("bbbbb"));
            edith_gapbuffer_insert(gb, 8, str("ccccccc"));

            test_assert_gapbuffer(loc(), gb, str("aabbbbbaccccccc"));
        }

        test_scope("insert_split_twice") {
            edith_gapbuffer_insert(gb, 0, str("aaa"));
            edith_gapbuffer_insert(gb, 2, str("bbbbb"));
            edith_gapbuffer_insert(gb, 5, str("ccccccc"));

            test_assert_gapbuffer(loc(), gb, str("aabbbcccccccbba"));
        }

        test_scope("delete") {
            edith_gapbuffer_insert(gb, 0, str("abcdefg"));
            edith_gapbuffer_delete(gb, 2, 4);

            test_assert_gapbuffer(loc(), gb, str("abg"));
        }

        test_scope("delete_across") {
            edith_gapbuffer_insert(gb, 0, str("aaa"));
            edith_gapbuffer_insert(gb, 3, str("bbbbb"));
            edith_gapbuffer_delete(gb, 2, 3);

            test_assert_gapbuffer(loc(), gb, str("aabbb"));
        }

        test_scope("basic_1") {
            edith_gapbuffer_insert(gb, 0, str("A_large_span_of_text"));
            edith_gapbuffer_insert(gb, 20, str("I am the Walruls"));
            edith_gapbuffer_delete(gb, 2, 6);
            edith_gapbuffer_delete(gb, 14, 2);
            edith_gapbuffer_delete(gb, 0, 2);
            edith_gapbuffer_insert(gb, 0, str("Donatello"));

            test_assert_gapbuffer(loc(), gb, str("Donatellospan_of_textam the Walruls"));
        }

        ////////////////////////////////////////////////////////////////
        // rune: Iterator forward

        test_scope("forward_basic") {
            edith_gapbuffer_insert(gb, 0, str("Initial text"));

            i32 dirs[] = { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 };
            test_assert_doc_iter(loc(), gb, 0, dirs, countof(dirs), str("Initial text"), 12);
        }

        test_scope("forward_basic_two_piece") {
            edith_gapbuffer_insert(gb, 0, str("abcdef"));
            edith_gapbuffer_insert(gb, 2, str("ABC"));

            i32 dirs[] = { 1, 1, 1, 1, 1, 1 };
            test_assert_doc_iter(loc(), gb, 0, dirs, countof(dirs), str("abABCc"), 6);
        }

        test_scope("forward_begin_mid_two_piece") {
            edith_gapbuffer_insert(gb, 0, str("abcdef"));
            edith_gapbuffer_insert(gb, 4, str("ABC"));

            i32 dirs[] = { 1, 1, 1, 1, 1 };
            test_assert_doc_iter(loc(), gb, 3, dirs, countof(dirs), str("dABCe"), 8);
        }

        test_scope("forward_begin_boundary_two_piece") {
            edith_gapbuffer_insert(gb, 0, str("abcdef"));
            edith_gapbuffer_insert(gb, 4, str("ABC"));

            i32 dirs[] = { 1, 1, 1, 1 };
            test_assert_doc_iter(loc(), gb, 4, dirs, countof(dirs), str("ABCe"), 8);
        }

        test_scope("forward_past_end") {
            edith_gapbuffer_insert(gb, 0, str("abc"));
            edith_gapbuffer_insert(gb, 2, str("ABC"));

            i32 dirs[] = { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 };
            test_assert_doc_iter(loc(), gb, 4, dirs, countof(dirs), str("Cc"), 6);
        }

        ////////////////////////////////////////////////////////////////
        // rune: Iterator backward

        test_scope("backward_basic") {
            edith_gapbuffer_insert(gb, 0, str("abc"));

            i32 dirs[] = { -1, -1, -1, -1 };
            test_assert_doc_iter(loc(), gb, 3, dirs, countof(dirs), str("cba"), 0);
        }

        test_scope("backward_basic_two_piece") {
            edith_gapbuffer_insert(gb, 0, str("abc"));
            edith_gapbuffer_insert(gb, 1, str("ABC"));

            i32 dirs[] = { -1, -1, -1, -1, -1 };
            test_assert_doc_iter(loc(), gb, 4, dirs, countof(dirs), str("bCBAa"), 0);
        }

        test_scope("backward_begin_mid_two_piece") {
            edith_gapbuffer_insert(gb, 0, str("abcdef"));
            edith_gapbuffer_insert(gb, 4, str("ABC"));

            i32 dirs[] = { -1, -1, -1 };
            test_assert_doc_iter(loc(), gb, 5, dirs, countof(dirs), str("BAd"), 2);
        }

        ////////////////////////////////////////////////////////////////
        // rune: Iterator line endings forward

        test_scope("crlf_forward") {
            edith_gapbuffer_insert(gb, 0, str("abc\r\ndef"));

            i32 dirs[] = { 1, 1, 1, 1, 1, 1, 1 };
            test_assert_doc_iter(loc(), gb, 0, dirs, countof(dirs), str("abc\ndef"), 8);
        }

        test_scope("crlf_backward") {
            edith_gapbuffer_insert(gb, 0, str("abc\r\ndef"));

            i32 dirs[] = { -1, -1, -1, -1, -1, -1, -1 };
            test_assert_doc_iter(loc(), gb, 7, dirs, countof(dirs), str("fed\ncba"), 0);
        }
    }
}
