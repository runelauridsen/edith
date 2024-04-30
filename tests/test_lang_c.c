////////////////////////////////////////////////////////////////
// rune: Test C indexer

static void test_lang_c(void) {

    ////////////////////////////////////////////////////////////////
    // rune: C declaration parsing

    test_ctx ctx = { 0 };
    ctx.name = str("Declaration parsing");

    test_ctx(&ctx) {
        typedef struct test_case test_case;
        struct test_case {
            str name;
            str input;
            str expect;
        };

        static readonly test_case cases[] = {
            { STR("a"),         STR("app_input input =    "),   STR("input: app_input                    "), },
            { STR("printf2"),   STR("printf (a);          "),   STR("                                    "), },
            { STR("printf2"),   STR("printf (*a);         "),   STR("                                    "), },
            { STR("printf2"),   STR("printf (a[3]);       "),   STR("                                    "), },
            { STR("printf2"),   STR("printf (*a[3]);      "),   STR("                                    "), },
            { STR("printf2"),   STR("printf (*a)[];       "),   STR("a: *[]printf                        "), },
            { STR("printf2"),   STR("printf (*a)();       "),   STR("a: *func() -> printf                "), },

            { STR("base0"),     STR("int my_var;                                                        "), STR("my_var: int                                                                              "), },
            { STR("base1"),     STR("int * my_var;                                                      "), STR("my_var: *int                                                                             "), },
            { STR("base2"),     STR("my_type * my_var;                                                  "), STR("my_var: *my_type                                                                         "), },
            { STR("base3"),     STR("some_var = my_var * yo_var;                                        "), STR("                                                                                         "), },
            { STR("base4"),     STR("int const * volatile my_var;                                       "), STR("my_var: volatile*int const                                                               "), },
            { STR("base5"),     STR("char (*(*x[3])())[5];                                              "), STR("x: [3]*func() -> *[5]char                                                                "), },
            { STR("base6"),     STR("char * const (*(* const bar(int arg, float))[5])(int);             "), STR("bar: func(arg: int, <unnamed>: float) -> const*[5]*func(<unnamed>: int) -> const*char    "), },
            { STR("base7"),     STR("readonly some_macro my_type my_var;                                "), STR("my_var: readonly some_macro my_type                                                      "), },
            { STR("base8"),     STR("readonly my_type * my_var;                                         "), STR("my_var: *readonly my_type                                                                "), },
            { STR("base9"),     STR("readonly my_type (** my_var);                                      "), STR("my_var: **readonly my_type                                                               "), },
            { STR("base10"),    STR("int x[][sizeof(y[0])][33]                                          "), STR("x: [][sizeof(y[0])][33]int                                                               "), },
            { STR("base11"),    STR("for_list (T, it, list)                                             "), STR("                                                                                         "), },
            { STR("base12"),    STR("static ui_mouse_buttons win32_translate_wm_to_mousebutton(u32 wm)  "), STR("win32_translate_wm_to_mousebutton: func(wm: u32) -> static ui_mouse_buttons              "), }, // TODO(rune): Parse storage class properly

            { STR("printf0"),   STR("printf a;            "),   STR("a: printf                           "), },
            { STR("printf1"),   STR("printf *a;           "),   STR("a: *printf                          "), },
            { STR("printf2"),   STR("printf (*a);         "),   STR("                                    "), },
            { STR("printf3"),   STR("printf (*a)();       "),   STR("a: *func() -> printf                "), },
            { STR("printf4"),   STR("printf (a);          "),   STR("                                    "), },
            { STR("printf5"),   STR("printf (a)();        "),   STR("                                    "), },
            { STR("printf6"),   STR("c printf (a);        "),   STR("printf: func(<unnamed>: a) -> c     "), },
            { STR("printf7"),   STR("c printf (a *);      "),   STR("printf: func(<unnamed>: *a) -> c    "), },
            { STR("printf8"),   STR("c printf (a * b);    "),   STR("printf: func(b: *a) -> c            "), },
            { STR("printf9"),   STR("c printf (a b);      "),   STR("printf: func(b: a) -> c             "), },
            { STR("printf10"),  STR("c (*printf)(a);      "),   STR("printf: *func(<unnamed>: a) -> c    "), },
            { STR("printf11"),  STR("c (*printf)(a *);    "),   STR("printf: *func(<unnamed>: *a) -> c   "), },
            { STR("printf12"),  STR("c (*printf)(a * b);  "),   STR("printf: *func(b: *a) -> c           "), },
            { STR("printf13"),  STR("c (*printf)(a b);    "),   STR("printf: *func(b: a) -> c            "), },
        };

        arena *arena = test_arena();
        for_sarray (test_case, it, cases) {
            test_scope("%: % → %", it->name, it->input, it->expect) {
                lang_c_symbol_list symbols = lang_c_symbols_from_str(it->input, arena);

                str actual = lang_c_str_from_symbol_list(symbols, arena);
                str expect = str_trim(it->expect);

                test_assert_eq(loc(), actual, expect);
            }
        }
    }
}

////////////////////////////////////////////////////////////////
// rune: Fuzz testing

static void lang_c_sanity_check(lang_c_indexer *actual_cix, edith_gapbuffer *actual_gb, arena *temp) {
    arena_scope(temp) {
        lang_c_indexer expect_cix = { 0 };
        edith_gapbuffer   expect_gb  = { 0 };

        // rune: Copy text from gb to truth_gb and parse with truth_cix.
        {
            str full_text = edith_str_from_gapbuffer_range(actual_gb, i64_range(0, edith_gapbuffer_len(actual_gb)), temp);

            edith_edit single_edit = { .pos = 0, .data = full_text, };
            edith_edit_array single_edits = { .v = &single_edit, .count = 1, .kind = EDITH_EDIT_KIND_INSERT };

            edith_gapbuffer_apply_edits(&expect_gb, single_edits);
            lang_c_apply_edits(&expect_cix, &expect_gb, single_edits);
        }

        // rune: Compare with truth.
        {
            str actual_src = edith_str_from_gapbuffer_range(actual_gb, i64_range(0, edith_gapbuffer_len(actual_gb)), temp);
            str expect_src = edith_str_from_gapbuffer_range(&expect_gb, i64_range(0, edith_gapbuffer_len(&expect_gb)), temp);
            assert(str_eq(actual_src, expect_src));

#if LANG_C_DEBUG_PRINT_FRAG_TREE
            print(ANSI_FG_BRIGHT_MAGENTA);
            println("EXPECT");
            println("EXPECT");
            println("EXPECT");
            print(ANSI_FG_DEFAULT);
            lang_c_debug_print_frag_tree(&expect_cix.frag_tree);

            print(ANSI_FG_BRIGHT_MAGENTA);
            println("ACTUAL");
            println("ACTUAL");
            println("ACTUAL");
            print(ANSI_FG_DEFAULT);
            lang_c_debug_print_frag_tree(&actual_cix->frag_tree);
#endif

            lang_c_frag *expect_frag = expect_cix.frag_tree.frags.first;
            lang_c_frag *actual_frag = actual_cix->frag_tree.frags.first;

            while (expect_frag && actual_frag) {
                assert(expect_frag->pos  == actual_frag->pos);
                assert(expect_frag->len  == actual_frag->len);
                assert(expect_frag->kind == actual_frag->kind);

                if (expect_frag->scope) {
                    assert(actual_frag->scope != null);
                    assert(expect_frag->scope->begin->pos == actual_frag->scope->begin->pos);

                    if (expect_frag->scope->end != null) {
                        assert(expect_frag->scope->end->pos == actual_frag->scope->end->pos);
                    } else {
                        //assert(actual_frag->scope->end == null);
                    }
                } else {
                    assert(actual_frag->scope == null);
                }

                expect_frag = expect_frag->next;
                actual_frag = actual_frag->next;
            }

            assert(expect_frag == null);
            assert(actual_frag == null);
        }

        // rune: Cleanup.
        {
            edith_gapbuffer_destroy(&expect_gb);
            lang_c_free(&expect_cix);
        }
    }
}

static void lang_c_fuzz_test_insert(str initial_src, str to_insert) {
    arena *arena = edith_arena_create_default(str("cix fuzz test"));
    for_n (i64, i, initial_src.len) {
        lang_c_global_debug_id_counter = 0;

#if LANG_C_DEBUG_PRINT_FRAG_TREE
        i = 66;
        to_insert = str("{");
#endif

        lang_c_indexer actual_cix = { 0 };
        edith_gapbuffer actual_gb = { 0 };

        // rune: Do 2 step insertion.
        {
            edith_edit  initial_edit  = { .pos = 0, .data = initial_src, };
            edith_edit_array initial_edits = { .v = &initial_edit, .count = 1, .kind = EDITH_EDIT_KIND_INSERT };

            edith_edit  fuzz_edit  = { .pos = i, .data = to_insert, };
            edith_edit_array fuzz_edits = { .v = &fuzz_edit, .count = 1, .kind = EDITH_EDIT_KIND_INSERT };

            edith_gapbuffer_apply_edits(&actual_gb, initial_edits);
            lang_c_apply_edits(&actual_cix, &actual_gb, initial_edits);

            edith_gapbuffer_apply_edits(&actual_gb, fuzz_edits);
            lang_c_apply_edits(&actual_cix, &actual_gb, fuzz_edits);
        }

        // rune: Check
        {
            lang_c_sanity_check(&actual_cix, &actual_gb, arena);
        }

        // rune: Cleanup
        {
            arena_reset(arena);
            edith_gapbuffer_destroy(&actual_gb);
            lang_c_free(&actual_cix);
        }
    }

    edith_arena_destroy(arena);
}

static void lang_c_fuzz_test(void) {
#if 1
    str src = str(
        " #if 0 // ay                                                                                       \n"
        " #endif                                                                                            \n"
        "                                                                                                   \n"
        "         static lang_c_frag *lang_c_frag_from_pos(lang_c_frag_tree *tree, i64 pos, i64 *out_pos) {          \n"
        "         lang_c_frag *frag = null;                                                                    \n"
        "         i64 total_off = 0;                                                                        \n"
        "         for_list (lang_c_frag, it, tree->frags) { // yoyoyo                                          \n"
        "             if (pos < total_off + it->off) {                                                      \n"
        "                 break;                                                                            \n"
        "             }                                                                                     \n"
        "                                                                                                   \n"
        "             total_off += it->off;                                                                 \n"
        "             frag = it;                                                                            \n"
        "         }                                                                                         \n"
        "                                                                                                   \n"
        "         *out_pos = total_off + frag->off;                                                         \n"
        "         return frag;                                                                              \n"
        "     }                                                                                             \n"
        "                                                                                                   \n"
        "     int main(void) {                                                                              \n"
        "         printf(\"Hello, World!\");                                                                \n"
        "     }                                                                                             \n"
    );
#else
    str src = str(
        " #if 0 // ay                 \n"
        " int main(void) {            \n"
        "     if (1) { // comment     \n"
        "         printf(\"hello\");  \n"
        "     }                       \n"
        "     1 + 2;                  \n"
        " }                           \n"
    );
#endif


#if 1
    lang_c_fuzz_test_insert(src, str("}"));
    lang_c_fuzz_test_insert(src, str("{"));
    lang_c_fuzz_test_insert(src, str(";"));
    lang_c_fuzz_test_insert(src, str("a"));
    lang_c_fuzz_test_insert(src, str(" "));
    lang_c_fuzz_test_insert(src, str("\""));
    lang_c_fuzz_test_insert(src, str("#"));
    lang_c_fuzz_test_insert(src, str("("));

    u8 buffer[3] = { 0 };
    u8 opts[] = { '{', '}', ';', 'a', ' ', '\"', '#', '(' };

    i64 num_permutations = i64_pow(countof(opts), countof(buffer));
    for_n (i64, i, num_permutations) {
        i64 k = i;
        for_n (i64, j, countof(buffer)) {
            buffer[j] = opts[k % countof(opts)];
            k /= countof(opts);
        }
        str s = str_make(buffer, sizeof(buffer));

        lang_c_fuzz_test_insert(src, s);

        print(ANSI_FG_BRIGHT_GREEN);
        println("%/% done %(literal)", i + 1, num_permutations, s);
        print(ANSI_FG_DEFAULT);
    }
#else
    lang_c_fuzz_test_insert(src, str("a"));
#endif
}

static void lang_c_fuzz_test_decl_parse(void) {
    str parts[] = {
       str("("),
       str(")"),
       str("a"),
       str("*"),
       //str(","),
    };

    arena *arena = edith_arena_create_default(str("cix fuzz test decl"));
    for_range (i64, i, 3, 6) {
        i64 num_permutations = i64_pow(countof(parts), i);
        for_range (i64, j, 0, num_permutations) {
            arena_scope(arena) {
                // rune: Generate permutation.
                str s = str(""); {
                    str_list list = { 0 };
                    i64 k = j;
                    while (list.count < i) {
                        i64 idx = k % countof(parts);
                        k /= countof(parts);

                        str_list_push(&list, arena, parts[idx]);
                    }
                    str_list_push(&list, arena, str(";"));
                    s = str_list_concat(&list, arena);
                }

                // rune: Tokenize.
                lang_c_token_list token_list = lang_c_token_list_from_str(s, arena);

                // rune: Check balanced parenthesis.
                bool balanced_paren = true; {
                    i64 stack_idx = 0;
                    for_list (lang_c_token, token, token_list) {
                        if (token->kind == LANG_C_TOKEN_KIND_PAREN_OPEN) {
                            stack_idx++;
                        }

                        if (token->kind == LANG_C_TOKEN_KIND_PAREN_CLOSE) {
                            if (stack_idx > 0) {
                                stack_idx--;
                            } else {
                                balanced_paren = false;
                                break;
                            }
                        }
                    }

                    if (stack_idx != 0) {
                        balanced_paren = false;
                    }
                }

                // rune: Parse.
                if (balanced_paren) {
                    lang_c_symbol_list symbols = lang_c_symbols_from_str(s, arena);
                    str symbols_str = lang_c_str_from_symbol_list(symbols, arena);
                    println("% \t %", s, symbols_str);
                }
            }
        }
    }
}
