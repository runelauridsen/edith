////////////////////////////////////////////////////////////////
// rune: Test edits

static void test_assert_consolidated_edits(str initial, edith_edit_batch orig) {
    // rune: Consolidate
    edith_edit_batch consolidated = edith_edit_batch_consolidate(orig, test_arena());

    // rune: Expect
    edith_gapbuffer gb_expect = { 0 };
    edith_gapbuffer_create(&gb_expect, 1024, str("test gapbuffer"));
    edith_gapbuffer_insert(&gb_expect, 0, initial);
    for_list (edith_edit_batch_node, node, orig) {
        edith_gapbuffer_apply_edits(&gb_expect, node->edits);
    }

    // rune: Actual
    edith_gapbuffer gb_actual = { 0 };
    edith_gapbuffer_create(&gb_actual, 1024, str("test gapbuffer"));
    edith_gapbuffer_insert(&gb_actual, 0, initial);
    for_list (edith_edit_batch_node, node, consolidated) {
        edith_gapbuffer_apply_edits(&gb_actual, node->edits);
    }

    // rune: Check
    str expect = edith_str_from_gapbuffer(&gb_expect, test_arena());
    str actual = edith_str_from_gapbuffer(&gb_actual, test_arena());
    test_assert_eq(loc(), actual, expect);

    i64 consolidated_edit_count = 0;
    for_list (edith_edit_batch_node, node, consolidated) {
        for_array (edith_edit, edit, node->edits) {
            consolidated_edit_count++;
        }
    }

    i64 orig_edit_count = 0;
    for_list (edith_edit_batch_node, node, orig) {
        for_array (edith_edit, edit, node->edits) {
            orig_edit_count++;
        }
    }

    test_assert(loc(), consolidated_edit_count < orig_edit_count);

    // rune: Debug print
#if EDITH_DEBUG_PRINT_EDIT_BATCH_CONSOLIDATE
    print(ANSI_FG_BRIGHT_BLUE);
    print("%(literal)\n", expect);
    print("%(literal)\n", actual);
    print("\n");
    print(ANSI_RESET);
#endif
}

static void test_edits(void) {
    test_ctx ctx = { 0 };
    ctx.name = str("Edit consolidation");
    test_ctx(&ctx) {
#if 1
        test_scope("consolidate 1 (simple)") {
            str initial = str("aaaaaaaaa bbbbbbbbb ccccccccc ddddddddd");

            edith_edit e0[] = {
                {  0, str("X0-") },
                { 10, str("Y0-") },
                { 20, str("Z0-") },
                { 30, str("W0-") },
            };

            edith_edit e1[] = {
                {  0 +  3 * 1, str("X1--") },
                { 10 +  6 * 1, str("Y1--") },
                { 20 +  9 * 1, str("Z1--") },
                { 30 + 12 * 1, str("W1--") },
            };

            edith_edit e2[] = {
                {  0 +  3 * 1 +  4 * 1, str("X2---") },
                { 10 +  6 * 1 +  8 * 1, str("Y2---") },
                { 20 +  9 * 1 + 12 * 1, str("Z2---") },
                { 30 + 12 * 1 + 16 * 1, str("W2---") },
            };

            edith_edit_batch batch = { 0 };
            edith_edit_batch_push(&batch, (edith_edit_array) { e0, countof(e0), EDITH_EDIT_KIND_INSERT }, test_arena());
            edith_edit_batch_push(&batch, (edith_edit_array) { e1, countof(e1), EDITH_EDIT_KIND_INSERT }, test_arena());
            edith_edit_batch_push(&batch, (edith_edit_array) { e2, countof(e2), EDITH_EDIT_KIND_INSERT }, test_arena());

            test_assert_consolidated_edits(initial, batch);
        }

        test_scope("consolidate insert 2 (mixed array len)") {
            str initial = str("aaaaaaaaa bbbbbbbbb ccccccccc ddddddddd");

            edith_edit e0[] = {
                {  0, str("X0-") },
                { 10, str("Y0-") },
                { 20, str("Z0-") },
                { 30, str("W0-") },
            };

            edith_edit e1[] = {
                {  0 +  3 * 1, str("X1--") },
                { 10 +  6 * 1, str("Y1--") },
                //{ 20 +  9 * 1, str("Z1--") },
                { 30 + 12 * 1, str("w1--") },
            };

            edith_edit e2[] = {
                {  0 +  3 * 1 +  4 * 1, str("X2---") },
                { 10 +  6 * 1 +  8 * 1, str("Y2---") },
                { 20 +  9 * 1 +  8 * 1, str("Z2---") }, // NOTE(rune): Should be appended to "Z0-", even though e1[] did not have a chain part for "Z0-"
                { 30 + 12 * 1 + 12 * 1, str("W2---") },
            };

            edith_edit_batch batch = { 0 };
            edith_edit_batch_push(&batch, (edith_edit_array) { e0, countof(e0), EDITH_EDIT_KIND_INSERT }, test_arena());
            edith_edit_batch_push(&batch, (edith_edit_array) { e1, countof(e1), EDITH_EDIT_KIND_INSERT }, test_arena());
            edith_edit_batch_push(&batch, (edith_edit_array) { e2, countof(e2), EDITH_EDIT_KIND_INSERT }, test_arena());

            test_assert_consolidated_edits(initial, batch);
        }

        test_scope("consolidate insert 3 (mixed array len)") {
            str initial = str("aaaaaaaaa bbbbbbbbb ccccccccc ddddddddd");

            edith_edit e0[] = {
                {  0, str("X0-") },
                { 10, str("Y0-") },
                //{ 20, str("Z0-") },
                { 30, str("W0-") },
            };

            edith_edit e1[] = {
                {  0 +  3 * 1, str("X1--") },
                { 10 +  6 * 1, str("Y1--") },
                { 20 +  6 * 1, str("Z1--") }, // NOTE(rune): Causes a new chain to be created, because "Z0-" is commented out.
                { 30 +  9 * 1, str("w1--") },
            };

            edith_edit e2[] = {
                {  0 +  3 * 1 +  4 * 1, str("X2---") },
                { 10 +  6 * 1 +  8 * 1, str("Y2---") },
                { 20 +  9 * 1 +  9 * 1, str("Z2---") },
                { 30 + 12 * 1 + 13 * 1, str("W2---") },
            };

            edith_edit_batch batch = { 0 };
            edith_edit_batch_push(&batch, (edith_edit_array) { e0, countof(e0), EDITH_EDIT_KIND_INSERT }, test_arena());
            edith_edit_batch_push(&batch, (edith_edit_array) { e1, countof(e1), EDITH_EDIT_KIND_INSERT }, test_arena());
            edith_edit_batch_push(&batch, (edith_edit_array) { e2, countof(e2), EDITH_EDIT_KIND_INSERT }, test_arena());

            test_assert_consolidated_edits(initial, batch);
        }

        test_scope("consolidate insert 4 (mixed array len + 5 chains)") {
            str initial = str("aaaaaaaaa bbbbbbbbb ccccccccc ddddddddd");

            edith_edit e0[] = {
                {  0, str("X0-") },
                { 10, str("Y0-") },
                //{ 20, str("Z0-") },
                { 30, str("W0-") },
            };

            edith_edit e1[] = {
                {  0 +  3 * 1, str("X1--") },
                { 10 +  6 * 1, str("Y1--") },
                { 20 +  6 * 1, str("Z1--") },
                { 30 +  9 * 1, str("w1--") },
            };

            edith_edit e2[] = {
                {  0 +  3 * 1 +  4 * 1, str("X2---") },
                { 10 +  6 * 1 +  8 * 1, str("Y2---") },
                { 21 +  9 * 1 +  9 * 1, str("Z2---") },
                { 30 + 12 * 1 + 13 * 1, str("W2---") },
            };

            edith_edit_batch batch = { 0 };
            edith_edit_batch_push(&batch, (edith_edit_array) { e0, countof(e0), EDITH_EDIT_KIND_INSERT }, test_arena());
            edith_edit_batch_push(&batch, (edith_edit_array) { e1, countof(e1), EDITH_EDIT_KIND_INSERT }, test_arena());
            edith_edit_batch_push(&batch, (edith_edit_array) { e2, countof(e2), EDITH_EDIT_KIND_INSERT }, test_arena());

            test_assert_consolidated_edits(initial, batch);
        }

        test_scope("consolidate insert 5 (before/after)") {
            str initial = str("aaaaaaaaa bbbbbbbbb ccccccccc ddddddddd");

            edith_edit e0[] = {
                {  0, str("X0") },
                { 10, str("Y0") },
                { 20, str("Z0") },
            };

            edith_edit e1[] = {
                {  0, str("X1") }, // NOTE(rune): Deletion comes before "a5"
                { 14, str("Y1") }, // NOTE(rune): Deletion comes after  "b5"
                { 24, str("Z1") }, // NOTE(rune): Deletion comes before "b5"
            };

            edith_edit_batch batch = { 0 };
            edith_edit_batch_push(&batch, (edith_edit_array) { e0, countof(e0), EDITH_EDIT_KIND_INSERT }, test_arena());
            edith_edit_batch_push(&batch, (edith_edit_array) { e1, countof(e1), EDITH_EDIT_KIND_INSERT }, test_arena());

            test_assert_consolidated_edits(initial, batch);
        }

        test_scope("consolidate delete 1 (simple)") {
            str initial = str("a1a2a3a4a b1b2b3b4b c1c2c3c4c d1d2d3d4d ");
            //str initial = str("3456789 3456789 3456789 3456789 ");

            edith_edit e0[] = {
                {  0, str("a1") },
                { 10, str("b1") },
                { 20, str("c1") },
                { 30, str("d1") },
            };

            edith_edit e1[] = {
                {  0, str("a2") },
                {  8, str("b2") },
                { 16, str("c2") },
                { 24, str("d2") },
            };

            edith_edit e2[] = {
                {  0, str("a3") },
                {  6, str("b3") },
                { 12, str("c3") },
                { 18, str("d3") },
            };

            edith_edit_batch batch = { 0 };
            edith_edit_batch_push(&batch, (edith_edit_array) { e0, countof(e0), EDITH_EDIT_KIND_DELETE }, test_arena());
            edith_edit_batch_push(&batch, (edith_edit_array) { e1, countof(e1), EDITH_EDIT_KIND_DELETE }, test_arena());
            edith_edit_batch_push(&batch, (edith_edit_array) { e2, countof(e2), EDITH_EDIT_KIND_DELETE }, test_arena());

            test_assert_consolidated_edits(initial, batch);
        }

        test_scope("consolidate delete 2 (mixed array len)") {
            str initial = str("a1a2a3a4a b1b2b3b4b c1c2c3c4c d1d2d3d4d ");

            edith_edit e0[] = {
                {  0, str("a1") },
                { 30, str("d1") },
            };

            edith_edit e1[] = {
                {  0, str("a2") },
            };

            edith_edit e2[] = {
                {  0, str("a3") },
                {  6, str("b3") },
                { 14, str("c3") },
            };

            edith_edit_batch batch = { 0 };
            edith_edit_batch_push(&batch, (edith_edit_array) { e0, countof(e0), EDITH_EDIT_KIND_DELETE }, test_arena());
            edith_edit_batch_push(&batch, (edith_edit_array) { e1, countof(e1), EDITH_EDIT_KIND_DELETE }, test_arena());
            edith_edit_batch_push(&batch, (edith_edit_array) { e2, countof(e2), EDITH_EDIT_KIND_DELETE }, test_arena());

            test_assert_consolidated_edits(initial, batch);
        }

        test_scope("consolidate delete 3 (mixed array len)") {
            str initial = str("a1a2a3a4a5a6a7a8a9a b1b2b3b4b5b6b7b8b9b c1c2c3c4c5c6c7c8c9c d1d2d3d4d5d6d7d8d9d ");

            edith_edit e0[] = {
                {  0, str("a1a2a3a4") },
                { 20, str("b1b2b3") },
                { 40, str("c1c2") },
            };

            edith_edit e1[] = {
                {  0, str("a5") },
            };

            edith_edit_batch batch = { 0 };
            edith_edit_batch_push(&batch, (edith_edit_array) { e0, countof(e0), EDITH_EDIT_KIND_DELETE }, test_arena());
            edith_edit_batch_push(&batch, (edith_edit_array) { e1, countof(e1), EDITH_EDIT_KIND_DELETE }, test_arena());

            test_assert_consolidated_edits(initial, batch);
        }

        test_scope("consolidate delete 4 (before/after)") {
            str initial = str("a1a2a3a4a5a6a7a8a9a b1b2b3b4b5b6b7b8b9b c1c2c3c4c5c6c7c8c9c d1d2d3d4d5d6d7d8d9d ");

            edith_edit e0[] = {
                {  8, str("a5") },
                { 28, str("b5") },
                { 48, str("c5") },
            };

            edith_edit e1[] = {
                {  6, str("a4") }, // NOTE(rune): Deletion comes before "a5"
                { 26, str("b6") }, // NOTE(rune): Deletion comes after  "b5"
                { 42, str("c4") }, // NOTE(rune): Deletion comes before "b5"
            };

            edith_edit_batch batch = { 0 };
            edith_edit_batch_push(&batch, (edith_edit_array) { e0, countof(e0), EDITH_EDIT_KIND_DELETE }, test_arena());
            edith_edit_batch_push(&batch, (edith_edit_array) { e1, countof(e1), EDITH_EDIT_KIND_DELETE }, test_arena());

            test_assert_consolidated_edits(initial, batch);
        }
#endif
#if 1
        test_scope("temp") {
            str initial = str("aaaaaaaaa bbbbbbbbb ccccccccc ddddddddd");

            edith_edit e0[] = {
                {  0, str("X0") },
                { 10, str("Y0") },
                { 20, str("Z0") },
            };

            edith_edit e1[] = {
                {  0, str("X1") }, // NOTE(rune): Deletion comes before "a5"
                { 14, str("Y1") }, // NOTE(rune): Deletion comes after  "b5"
                { 24, str("Z1") }, // NOTE(rune): Deletion comes before "b5"
            };

            edith_edit_batch batch = { 0 };
            edith_edit_batch_push(&batch, (edith_edit_array) { e0, countof(e0), EDITH_EDIT_KIND_INSERT }, test_arena());
            edith_edit_batch_push(&batch, (edith_edit_array) { e1, countof(e1), EDITH_EDIT_KIND_INSERT }, test_arena());

            test_assert_consolidated_edits(initial, batch);
        }
#endif
    }
}