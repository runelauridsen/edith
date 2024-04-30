////////////////////////////////////////////////////////////////
// rune: Fragments

static lang_c_frag_kind_info *lang_c_frag_kind_info_get(lang_c_frag_kind a) {
    assert(a < countof(lang_c_frag_kind_infos));
    return &lang_c_frag_kind_infos[a];
}

static lang_c_frag *lang_c_frag_from_pos(lang_c_frag_tree *tree, i64 pos) {
    lang_c_frag *frag = null;
    for_list (lang_c_frag, it, tree->frags) {
        if (pos < it->pos) {
            break;
        }

        frag = it;
    }

    return frag;
}

static i64 lang_c_pos_from_frag(lang_c_frag_tree *tree, lang_c_frag *frag) {
    unused(tree);
    return frag->pos;
}

// NOTE(rune): Profiling instrumentation
static i64 frag_kind_rdtsc[LANG_C_FRAG_KIND_COUNT] = { 0 };
static i64 frag_kind_len[LANG_C_FRAG_KIND_COUNT] = { 0 };

static bool lang_c_next_frag(edith_gapbuffer *gb, i64 *pos, i64 *out_frag_pos, i64 *out_frag_len, lang_c_frag_kind *out_frag_kind) {

    // TODO(rune): Clean up this loop.
    // TODO(rune): Maybe just use lang_c_tokenizer even though it does more work than we need? We should measure the overhead.

    i64 len = edith_gapbuffer_len(gb);
    i64 at = *pos;
    while (at < len) {
        // rune: Eat whitespace.
        while (at < len && u8_is_whitespace(lang_c_gb_get_byte(gb, at))) {
            at += 1;
        }

        // rune: Begin fragment.
        lang_c_frag_kind frag_kind = 0;
        i64 frag_begin          = at;
        u8 peek0                = at + 0 < len ? lang_c_gb_get_byte(gb, at + 0) : '\0';
        u8 peek1                = at + 1 < len ? lang_c_gb_get_byte(gb, at + 1) : '\0';

        i64 rdtsc_begin = __rdtsc();

        // rune: Line comment fragment.
        if (peek0 == '/' && peek1 == '/') {
            frag_kind = LANG_C_FRAG_KIND_COMMENT;

            at += 2;
            while (at < len) {
                if (lang_c_gb_get_byte(gb, at) == '\r') break;
                if (lang_c_gb_get_byte(gb, at) == '\n') break;

                at += 1;
            }
        }

        // rune: Block comment fragment.
        else if (peek0 == '/' && peek1 == '*') {
            frag_kind = LANG_C_FRAG_KIND_COMMENT;

            at += 2;
            while (at < len) {
                peek0 = at + 0 < len ? lang_c_gb_get_byte(gb, at + 0) : '\0';
                peek1 = at + 1 < len ? lang_c_gb_get_byte(gb, at + 1) : '\0';

                if (peek0 == '*' && peek1 == '/') {
                    at += 2;
                    break;
                }

                at += 1;
            }
        }

        // rune: Preprocessor fragment.
        else if (peek0 == '#') {
            frag_kind = LANG_C_FRAG_KIND_PREPROC;

            at += 1;
            while (at < len) {
                // rune: Eat string literal.
                if (lang_c_gb_get_byte(gb, at) == '\"') {
                    at += 1;
                    bool escaped = false;
                    while (at < len) {
                        u8 c = lang_c_gb_get_byte(gb, at);
                        at += 1;
                        if (c == '"' && !escaped) {
                            break;
                        } else if (c == '\\' && !escaped) {
                            escaped = true;
                        } else {
                            escaped = false;
                        }
                    }
                }

                // rune: Eat char literal.
                if (lang_c_gb_get_byte(gb, at) == '\'') {
                    at += 1;
                    bool escaped = false;
                    while (at < len) {
                        u8 c = lang_c_gb_get_byte(gb, at);
                        at += 1;
                        if (c == '\'' && !escaped) {
                            break;
                        } else if (c == '\\' && !escaped) {
                            escaped = true;
                        } else {
                            escaped = false;
                        }
                    }
                }

                // rune: Eat line continuation.
                if (lang_c_gb_get_byte(gb, at) == '\\') {
                    at += 1;
                    while (at < len) {
                        peek0 = at + 0 < len ? lang_c_gb_get_byte(gb, at + 0) : '\0';
                        peek1 = at + 1 < len ? lang_c_gb_get_byte(gb, at + 1) : '\0';

                        if (peek0 == '\r' && peek1 == '\n') {
                            at += 2;
                            break;
                        }

                        if (peek0 == '\r' || peek0 == '\n') {
                            at += 1;
                            break;
                        }

                        if (!u8_is_whitespace(peek0)) {
                            break;
                        }
                    }
                }

                // rune: Continue until line break.
                if (lang_c_gb_get_byte(gb, at) == '\r') break;
                if (lang_c_gb_get_byte(gb, at) == '\n') break;

                at += 1;
            }
        }

        else if (peek0 == '{') {
            frag_kind = LANG_C_FRAG_KIND_OPEN;
            at += 1;
        }

        else if (peek0 == '}') {
            frag_kind = LANG_C_FRAG_KIND_CLOSE;
            at += 1;
        }

        else {
            frag_kind = LANG_C_FRAG_KIND_CODE;
            while (at < len) {
                // rune: Semicolon ends fragment.
                if (lang_c_gb_get_byte(gb, at) == ';') {
                    at += 1;
                    break;
                }

                // rune: Brace also ends fragment, but should be its own fragment.
                else if (lang_c_gb_get_byte(gb, at) == '}' || lang_c_gb_get_byte(gb, at) == '{') {
                    break;
                }

                // rune: Eat string literal.
                else if (lang_c_gb_get_byte(gb, at) == '\"') {
                    at += 1;
                    bool escaped = false;
                    while (at < len) {
                        u8 c = lang_c_gb_get_byte(gb, at);
                        at += 1;
                        if (c == '"' && !escaped) {
                            break;
                        } else if (c == '\\' && !escaped) {
                            escaped = true;
                        } else {
                            escaped = false;
                        }
                    }
                }

                // rune: Eat char literal.
                else if (lang_c_gb_get_byte(gb, at) == '\'') {
                    at += 1;
                    bool escaped = false;
                    while (at < len) {
                        u8 c = lang_c_gb_get_byte(gb, at);
                        at += 1;
                        if (c == '\'' && !escaped) {
                            break;
                        } else if (c == '\\' && !escaped) {
                            escaped = true;
                        } else {
                            escaped = false;
                        }
                    }
                }

                // rune: Eat line continuation.
                else if (lang_c_gb_get_byte(gb, at) == '\\') {
                    at += 1;
                    while (at < len) {
                        peek0 = at + 0 < len ? lang_c_gb_get_byte(gb, at + 0) : '\0';
                        peek1 = at + 1 < len ? lang_c_gb_get_byte(gb, at + 1) : '\0';

                        if (peek0 == '\r' && peek1 == '\n') {
                            at += 2;
                            break;
                        }

                        if (peek0 == '\r' || peek0 == '\n') {
                            at += 1;
                            break;
                        }

                        if (!u8_is_whitespace(peek0)) {
                            break;
                        }
                    }
                }

                // rune: Continue fragment.
                else {
                    at += 1;
                }
            }
        }

        // rune: Did we identify a fragment?
        i64 frag_end = at;
        i64 frag_len = frag_end - frag_begin;
        if (frag_len > 0) {
            assert(frag_kind);
            i64 rdtsc_end = __rdtsc();
            frag_kind_rdtsc[frag_kind] += rdtsc_end - rdtsc_begin;
            frag_kind_len[frag_kind] += frag_end  - frag_begin;

            *out_frag_kind = frag_kind;
            *out_frag_pos  = frag_begin;
            *out_frag_len  = frag_len;
            *pos           = frag_end;
            return true;
        } else {
            // rune: Move forward one character and try again.
            if (at < len) {
                at += 1;
            }
        }
    }

    *pos = len;
    return false;
}

////////////////////////////////////////////////////////////////
// rune: Scope tree debug

static void lang_c_debug_check_scope_tree_(lang_c_scope *parent) {
    for_list (lang_c_scope, child, parent->children) {
        assert(child->parent == parent);
    }

    for_list (lang_c_scope, child, parent->children) {
        lang_c_debug_check_scope_tree_(child);
    }
}

static void lang_c_debug_check_scope_tree(lang_c_scope_tree *tree) {
    for_list (lang_c_scope, root, tree->roots) {
        lang_c_debug_check_scope_tree_(root);
    }
}

static void lang_c_debug_check_frag_tree(lang_c_frag_tree *tree) {
    for_list (lang_c_frag, frag, tree->frags) {
        if (frag->prev) {
            assert(frag->prev->pos <= frag->pos);
        }
    }
}

static void lang_c_debug_print_frag_tree(lang_c_frag_tree *tree) {
#if LANG_C_DEBUG_PRINT_FRAG_TREE
    println(ANSICONSOLE_FG_GRAY "== Fragment Tree ===============================================================" ANSICONSOLE_FG_DEFAULT);
    for_list (lang_c_frag, frag, tree->frags) {
        i64 pos = lang_c_pos_from_frag(tree, frag);

        print(ANSICONSOLE_FG_GRAY);
        i64 level = 0;
        lang_c_scope *scope = frag->scope;
        while (scope) {
            level += 1;
            scope = scope->parent;
        }

        print("id:\t%\toff:\t%\tpos:\t%\tlen:\t%\tlevel:\t%\t", frag->debug_id, frag->off, pos, frag->len, level);
        if (frag->kind == LANG_C_FRAG_KIND_OPEN || frag->kind == LANG_C_FRAG_KIND_CLOSE) {
            if (level > 0) level -= 1;
        }

        for_n (i64, i, level) {
            print("->  ");
        }

        if (frag->kind == LANG_C_FRAG_KIND_OPEN || frag->kind == LANG_C_FRAG_KIND_CLOSE) {
            print("->");
        }

        print(lang_c_get_frag_kind_info(frag->kind)->ansi_color);
        for_n (i64, i, frag->len) {
            u8 c = lang_c_gb_get_byte(lang_c_global_src_buffer, pos + i);
            switch (c) {
                case '\r': print("\\r"); break;
                case '\n': print("\\n"); break;
                case '\t': print("\\t"); break;
                default:   print("%(char)", c); break;
            }
        }
        print(ANSICONSOLE_FG_DEFAULT);
        print("\n");
    }
#else
    unused(tree);
#endif
}

////////////////////////////////////////////////////////////////
// rune: Scope tree

static lang_c_scope *lang_c_alloc_scope(lang_c_indexer *cix) {
    // TODO(rune): Freelist.
    lang_c_scope *scope = arena_push_struct(cix->arena, lang_c_scope);
    return scope;
}

static lang_c_frag *lang_c_add_frag(lang_c_indexer *cix, i64 pos, i64 len, lang_c_frag_kind kind) {
    lang_c_frag_tree *tree = &cix->frag_tree;

    lang_c_frag *f = lang_c_frag_from_pos(tree, pos);

    // rune: Insert into frag tree.
    lang_c_frag *frag = arena_push_struct(cix->arena, lang_c_frag);
    frag->debug_id = ++lang_c_global_debug_id_counter;
    frag->pos      = pos;
    frag->len      = len;
    frag->kind     = kind;

    if (f) {
        dlist_insert_after(&tree->frags, frag, f);
    } else {
        dlist_push_front(&tree->frags, frag);
    }

    // rune: Find which scope to place the new frag in.
    lang_c_scope *scope = null;
    if (frag->prev) {
        if (frag->prev->kind == LANG_C_FRAG_KIND_CLOSE) {
            scope = frag->prev->scope ? frag->prev->scope->parent : null;
        } else {
            scope = frag->prev->scope;
        }
    }

    // rune: Open new scope if needed.
    if (frag->kind == LANG_C_FRAG_KIND_OPEN) {
        lang_c_scope *new_scope = lang_c_alloc_scope(cix);
        new_scope->parent = scope;
        new_scope->begin  = frag;
        frag->scope = new_scope;
    } else {
        frag->scope = scope;
    }

    return frag;

}

static void lang_c_scope_tree_fixus(lang_c_indexer *cix, lang_c_frag *from) {
    if (from == null) return;

    lang_c_frag *curr_frag = from;
    if (curr_frag->kind == LANG_C_FRAG_KIND_OPEN) {
        curr_frag = curr_frag->next;
    }

    lang_c_scope *curr_scope = from->scope;

    while (curr_frag) {
        switch (curr_frag->kind) {
            case LANG_C_FRAG_KIND_PREPROC:
            case LANG_C_FRAG_KIND_COMMENT:
            case LANG_C_FRAG_KIND_CODE: {
                curr_frag->scope = curr_scope;
                curr_frag = curr_frag->next;
            } break;

            case LANG_C_FRAG_KIND_OPEN: {
                if (curr_frag->scope) {
                    curr_frag->scope->parent = curr_scope;
                    curr_frag->scope->begin = curr_frag;

                    if (curr_frag->scope->end) {
                        curr_frag = curr_frag->scope->end->next;
                    } else {
                        curr_scope = curr_frag->scope;
                        curr_frag = curr_frag->next;
                    }
                } else {
                    assert(false);
                }
            } break;

            case LANG_C_FRAG_KIND_CLOSE: {
                curr_frag->scope = curr_scope;

                if (curr_scope) curr_scope->end  = curr_frag;

                curr_scope       = curr_frag->scope ? curr_frag->scope->parent : null;
                curr_frag        = curr_frag->next;
            } break;

            default: {
                assert(false && "Invalid frag_kind.");
            } break;
        }

        lang_c_debug_print_frag_tree(&cix->frag_tree);
    }
}

static void lang_c_remove_frag(lang_c_frag_tree *tree, lang_c_frag *frag) {
    lang_c_debug_print_frag_tree(tree);
    dlist_remove(&tree->frags, frag);
    lang_c_debug_print_frag_tree(tree);
}

static void lang_c_scope_tree_rebuild(lang_c_indexer *cix) {
    // TODO(rune): Free all current scopes.

    lang_c_frag_tree *frag_tree   = &cix->frag_tree;
    lang_c_scope_tree *scope_tree = &cix->scope_tree;
    lang_c_scope *curr_scope      = null;

    for_list (lang_c_frag, frag, frag_tree->frags) {
        switch (frag->kind) {
            case LANG_C_FRAG_KIND_PREPROC:
            case LANG_C_FRAG_KIND_COMMENT:
            case LANG_C_FRAG_KIND_CODE: {
                frag->scope = curr_scope;
            } break;

            case LANG_C_FRAG_KIND_OPEN: {
                lang_c_scope *new_scope = lang_c_alloc_scope(cix);

                new_scope->begin = frag;
                new_scope->parent = curr_scope;

                if (curr_scope) {
                    dlist_push(&curr_scope->children, new_scope);
                }

                curr_scope = new_scope;

                frag->scope = curr_scope;
            } break;

            case LANG_C_FRAG_KIND_CLOSE: {
                frag->scope = curr_scope;

                if (curr_scope) {
                    curr_scope->end = frag;
                    curr_scope = curr_scope->parent;
                }
            } break;

            default: {
                assert(false && "Invalid frag_kind.");
            } break;
        }

        //lang_c_debug_print_scope_tree(tree);
        lang_c_debug_print_frag_tree(frag_tree);
        lang_c_debug_check_scope_tree(scope_tree);
    }
}

static void lang_c_rebuild(lang_c_indexer *cix, edith_gapbuffer *src_buffer) {
    lang_c_global_src_buffer = src_buffer;
    i64 pos = 0;
    i64 frag_pos = 0;
    i64 frag_len = 0;
    lang_c_frag_kind frag_kind = 0;
    while (lang_c_next_frag(src_buffer, &pos, &frag_pos, &frag_len, &frag_kind)) {
        lang_c_add_frag(cix, frag_pos, frag_len, frag_kind);
    }

    lang_c_scope_tree_rebuild(cix);
}

static edith_syntax_range_list lang_c_get_syntax_ranges(lang_c_indexer *cix, edith_gapbuffer *src_buffer, i64_range src_range, arena *arena) {
    YO_PROFILE_BEGIN(lang_c_get_syntax_ranges);

    edith_syntax_range_list result = { 0 };

    lang_c_frag *frag = null;
    {
        frag = lang_c_frag_from_pos(&cix->frag_tree, src_range.min);
        if (frag == null) {
            frag = cix->frag_tree.frags.first;
        }
    }

    {
        for (; frag; frag = frag->next) {
            i64_range frag_range = { frag->pos, frag->pos + frag->len };
            if (frag_range.min >= src_range.max) {
                break;
            }

            lang_c_lexer lexer = { 0 };
            lang_c_lexer_init(&lexer, frag_range.min, src_buffer);

            lang_c_token token = { 0 };
            while (lang_c_lexer_next_token(&lexer, &token)) {
                i64_range token_range = { token.pos, token.pos + token.len };
                if (token_range.min > src_range.max) {
                    break;
                }

                edith_syntax_range *node = arena_push_struct(arena, edith_syntax_range);
                node->kind = lang_c_token_kind_to_syntax_kind_table[token.kind];
                node->range = token_range;
                slist_push(&result, node);
            }
        }
    }

    YO_PROFILE_END(lang_c_get_syntax_ranges);
    return result;
}

static void lang_c_apply_edits(lang_c_indexer *cix, edith_gapbuffer *src_buffer, edith_edit_array edits) {
    // TODO(rune): Move to create() function
    if (cix->arena == 0) cix->arena = edith_arena_create_default(str("cix"));

    lang_c_global_src_buffer = src_buffer;
    cix->src_buffer = src_buffer;

    if (cix->init == false) {
        cix->init = true;
        lang_c_rebuild(cix, src_buffer);
    } else {
        // rune: Get edit range.
        i64_range edit_range = { 0 };
        i64 edit_len = 0;
        {
            i64 edit_min = I64_MAX;
            i64 edit_max = 0;
            for_array (edith_edit, e, edits) {
                min_assign(&edit_min, e->pos);
                max_assign(&edit_max, e->pos + e->data.len);
            }

            edit_range.min = edit_min;
            edit_range.max = edit_max;
            edit_len = edit_max - edit_min;
        }

        // rune: Shift
        // TODO(rune): Acceleration structure.
        {
            for_list (lang_c_frag, it, cix->frag_tree.frags) {
                if (it->pos >= edit_range.min) {
                    it->pos += edit_len;
                }
            }

            lang_c_debug_print_frag_tree(&cix->frag_tree);
        }

        // rune: Delete frags in range.
        // TODO(rune): Acceleration structure.
        i64_range deleted_range = { 0 };
        lang_c_debug_print_frag_tree(&cix->frag_tree);
        {
            lang_c_frag *delete_first = null;
            lang_c_frag *delete_last = null;

            i64 deleted_min = 0;
            i64 deleted_max = 0;
            for_list (lang_c_frag, frag, cix->frag_tree.frags) {
                i64_range frag_range = {
                    frag->pos,
                    frag->pos + frag->len,
                };

                if (frag_range.min <= edit_range.min) {
                    delete_first = frag;
                    deleted_min = frag_range.min;
                }

                if (frag_range.max <= edit_range.max) {
                    delete_last  = frag;
                    deleted_max = frag_range.max;
                }
            }

            for (lang_c_frag *frag = delete_first; frag; frag = frag->next) {
                lang_c_remove_frag(&cix->frag_tree, frag);

                if (frag == delete_last) {
                    break;
                }
            }

            lang_c_debug_check_frag_tree(&cix->frag_tree);
            deleted_range.min = deleted_min;
            deleted_range.max = deleted_max;
        }

        i64_range dirty_range = {
            min(edit_range.min, deleted_range.min),
            max(edit_range.max, deleted_range.max),
        };

        // rune: Insert new fragments.
#if 1
        lang_c_frag *first_added = null;
        {
            i64 pos = dirty_range.min;
            i64 frag_pos = 0;
            i64 frag_len = 0;
            lang_c_frag_kind frag_kind = 0;
            while (lang_c_next_frag(src_buffer, &pos, &frag_pos, &frag_len, &frag_kind)) {
                // TODO(rune): Cleanup. This is inefficient.
                lang_c_frag *existing = lang_c_frag_from_pos(&cix->frag_tree, frag_pos);
                if (existing && existing->pos == frag_pos) {
                    break;
                }

                lang_c_frag *added = lang_c_add_frag(cix, frag_pos, frag_len, frag_kind);
                if (first_added == null) first_added = added;

                // TODO(rune): Cleanup.
                while (1) {
                    lang_c_frag *next = added->next;
                    if (next) {
                        i64_range added_range = {
                            frag_pos,
                            frag_pos + added->len
                        };

                        i64_range next_range  = {
                            next->pos ,
                            next->pos + next->len
                        };

                        if (ranges_overlap(added_range, next_range)) {
                            lang_c_remove_frag(&cix->frag_tree, next);
                        } else {
                            break;
                        }
                    } else {
                        break;
                    }
                }

#if 0
                if (pos >= dirty_range.max) {
                    break;
                }
#endif
            }
        }
#endif

        lang_c_debug_print_frag_tree(&cix->frag_tree);

        {
            lang_c_scope_tree_fixus(cix, first_added);
        }

        nop();
    }
}

static edith_gapbuffer lang_c_immutable_gapbuffer(str s) {
    edith_gapbuffer ret = {
        .buf     = s.v,
        .buf_cap = s.len,
        .gap.min = 0,
        .gap.max = 0,
    };

    return ret;
}

static lang_c_symbol_list lang_c_symbols_from_str(str input, arena *arena) {
    unused(arena);

    lang_c_symbol_list result = { 0 };

    edith_gapbuffer gb = lang_c_immutable_gapbuffer(input);

    lang_c_frag_parser parser = { 0 };
    lang_c_frag_parser_init(&parser, i64_range(0, I64_MAX), &gb);
    lang_c_symbol_list symbols = lang_c_frag_parse_declaration(&parser, &lang_c_symbol_arena, 0);

    if (parser.error == false) {
        result = symbols;
    }

    return result;
}

static lang_c_symbol_list lang_c_symbols_from_range(i64_range range, edith_gapbuffer *src_buffer) {
    lang_c_symbol_list result = { 0 };

    lang_c_frag_parser parser = { 0 };
    lang_c_frag_parser_init(&parser, range, src_buffer);
    lang_c_symbol_list symbols = lang_c_frag_parse_declaration(&parser, &lang_c_symbol_arena, 0);

    if (parser.error == false) {
        result = symbols;
    }

    return result;
}

static void lang_c_free(lang_c_indexer *cix) {
    edith_arena_destroy(cix->arena);
}
