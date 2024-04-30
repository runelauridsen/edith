////////////////////////////////////////////////////////////////
// rune: Test find and replace

static void test_find_and_replace_before(edith_gapbuffer *gb) {
    zero_struct(gb);
    edith_gapbuffer_create(gb, 1024, str("test gapbuffer"));
}

static void test_find_and_replace_after(edith_gapbuffer *gb) {
    edith_gapbuffer_destroy(gb);
}

static void test_find_and_replace(void) {
    edith_gapbuffer _gb = { 0 };
    edith_gapbuffer *gb = &_gb;

    test_ctx ctx = {
        .name   = str("Find and replace"),
        .before = test_find_and_replace_before,
        .after  = test_find_and_replace_after,
        .param  = gb,
    };

    test_ctx(&ctx) {
        test_scope("forward_basic") {
            edith_gapbuffer_reserve_gap(gb, 16);
            edith_gapbuffer_insert(gb, 0, str("abcdefg"));
            edith_gapbuffer_insert(gb, 7, str("hijklmn"));
            edith_gapbuffer_shift_gap(gb, 7);

            i64 a = edith_next_occurence_in_gapbuffer(gb, i64_range(0, I64_MAX), str("cde"), DIR_FORWARD, false, test_arena());
            i64 b = edith_next_occurence_in_gapbuffer(gb, i64_range(0, I64_MAX), str("efghi"), DIR_FORWARD, false, test_arena());
            i64 c = edith_next_occurence_in_gapbuffer(gb, i64_range(0, I64_MAX), str("jklm"), DIR_FORWARD, false, test_arena());

            i64 d = edith_next_occurence_in_gapbuffer(gb, i64_range(0, I64_MAX), str("abc"), DIR_FORWARD, false, test_arena());
            i64 e = edith_next_occurence_in_gapbuffer(gb, i64_range(0, I64_MAX), str("efg"), DIR_FORWARD, false, test_arena());
            i64 f = edith_next_occurence_in_gapbuffer(gb, i64_range(0, I64_MAX), str("hij"), DIR_FORWARD, false, test_arena());

            test_assert_eq(loc(), a, 2);
            test_assert_eq(loc(), b, 4);
            test_assert_eq(loc(), c, 9);
            test_assert_eq(loc(), d, 0);
            test_assert_eq(loc(), e, 4);
            test_assert_eq(loc(), f, 7);
        }

        test_scope("forward_skip") {
            edith_gapbuffer_reserve_gap(gb, 16);
            edith_gapbuffer_insert(gb, 0, str("abcdefg"));
            edith_gapbuffer_insert(gb, 7, str("hijklmn"));
            edith_gapbuffer_shift_gap(gb, 7);

            i64 a = edith_next_occurence_in_gapbuffer(gb, i64_range(5, I64_MAX), str("cde"), DIR_FORWARD, false, test_arena());
            i64 b = edith_next_occurence_in_gapbuffer(gb, i64_range(5, I64_MAX), str("efghi"), DIR_FORWARD, false, test_arena());
            i64 c = edith_next_occurence_in_gapbuffer(gb, i64_range(5, I64_MAX), str("jklm"), DIR_FORWARD, false, test_arena());

            i64 d = edith_next_occurence_in_gapbuffer(gb, i64_range(5, I64_MAX), str("abc"), DIR_FORWARD, false, test_arena());
            i64 e = edith_next_occurence_in_gapbuffer(gb, i64_range(5, I64_MAX), str("efg"), DIR_FORWARD, false, test_arena());
            i64 f = edith_next_occurence_in_gapbuffer(gb, i64_range(5, I64_MAX), str("hij"), DIR_FORWARD, false, test_arena());

            test_assert_eq(loc(), a, -1); // 2
            test_assert_eq(loc(), b, -1); // 4
            test_assert_eq(loc(), c,  9);
            test_assert_eq(loc(), d, -1); // 0
            test_assert_eq(loc(), e, -1); // 4
            test_assert_eq(loc(), f,  7);
        }

        test_scope("forward_skip2") {
            edith_gapbuffer_reserve_gap(gb, 16);
            edith_gapbuffer_insert(gb, 0, str("abcdefg"));
            edith_gapbuffer_insert(gb, 7, str("hijklmn"));
            edith_gapbuffer_shift_gap(gb, 7);

            i64 a = edith_next_occurence_in_gapbuffer(gb, i64_range(8, I64_MAX), str("cde"), DIR_FORWARD, false, test_arena());
            i64 b = edith_next_occurence_in_gapbuffer(gb, i64_range(8, I64_MAX), str("efghi"), DIR_FORWARD, false, test_arena());
            i64 c = edith_next_occurence_in_gapbuffer(gb, i64_range(8, I64_MAX), str("jklm"), DIR_FORWARD, false, test_arena());

            i64 d = edith_next_occurence_in_gapbuffer(gb, i64_range(8, I64_MAX), str("abc"), DIR_FORWARD, false, test_arena());
            i64 e = edith_next_occurence_in_gapbuffer(gb, i64_range(8, I64_MAX), str("efg"), DIR_FORWARD, false, test_arena());
            i64 f = edith_next_occurence_in_gapbuffer(gb, i64_range(8, I64_MAX), str("hij"), DIR_FORWARD, false, test_arena());

            test_assert_eq(loc(), a, -1); // 2
            test_assert_eq(loc(), b, -1); // 4
            test_assert_eq(loc(), c,  9);
            test_assert_eq(loc(), d, -1); // 0
            test_assert_eq(loc(), e, -1); // 4
            test_assert_eq(loc(), f, -1); // 7
        }

        test_scope("forward_skip3") {
            edith_gapbuffer_reserve_gap(gb, 16);
            edith_gapbuffer_insert(gb, 0, str("abcdefg"));
            edith_gapbuffer_insert(gb, 7, str("hijklmn"));
            edith_gapbuffer_shift_gap(gb, 7);

            i64 a = edith_next_occurence_in_gapbuffer(gb, i64_range(8, I64_MAX), str("jklm"), DIR_FORWARD, false, test_arena());
            i64 b = edith_next_occurence_in_gapbuffer(gb, i64_range(9, I64_MAX), str("jklm"), DIR_FORWARD, false, test_arena());
            i64 c = edith_next_occurence_in_gapbuffer(gb, i64_range(10, I64_MAX), str("jklm"), DIR_FORWARD, false, test_arena());

            test_assert_eq(loc(), a,  9);
            test_assert_eq(loc(), b,  9);
            test_assert_eq(loc(), c, -1);
        }
    }
}
