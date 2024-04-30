////////////////////////////////////////////////////////////////
// rune: Parser

static void lang_c_frag_parser_init(lang_c_frag_parser *p, i64_range range, edith_gapbuffer *src_buffer) {
    lang_c_token_list tokens = lang_c_token_list_from_gapbuffer_range(src_buffer, range, &lang_c_symbol_arena);
    p->token_list = tokens;
    p->peek       = coalesce(tokens.first, &lang_c_null_token);
    p->gb         = src_buffer;
}

static lang_c_token *lang_c_eat_token(lang_c_frag_parser *p) {
    lang_c_token *eaten = p->peek;

    p->peek  = coalesce(p->peek->next, &lang_c_null_token);

#if 0
    print(ANSICONSOLE_FG_GRAY);
    print("=======================================================\n");
    print(ANSICONSOLE_FG_DEFAULT);

    for_list (lang_c_token, t, p->token_list) {
        if (t == p->peek) {
            print(ANSICONSOLE_FG_BRIGHT_GREEN);
            print("->");
            print(ANSICONSOLE_FG_DEFAULT);
        } else {
            print(ANSICONSOLE_FAINT);
            print("  ");
        }

        lang_c_debug_print_token(t, p->gb);
        print(ANSICONSOLE_RESET);
    }

#endif
    return eaten;
}

////////////////////////////////////////////////////////////////
// rune: Token range

static void lang_c_token_range_push(lang_c_token_range *range, lang_c_token *token, arena *arena) {
    lang_c_token_range_node *node = arena_push_struct(arena, lang_c_token_range_node);
    node->token = token;
    dlist_push(range, node);
    range->count++;
}

static lang_c_token *lang_c_token_range_pop(lang_c_token_range *range) {
    assert(range->count > 0);

    lang_c_token_range_node *popped = range->last;
    range->last = popped->prev;
    popped->prev->next = null;
    popped->prev = null;

    range->count--;

    return popped->token;
}

static str lang_c_str_from_token(lang_c_token *token, edith_gapbuffer *gb, arena *arena) {
    i64_range token_range = { token->pos, token->pos + token->len };
    str text = edith_str_from_gapbuffer_range(gb, token_range, arena);
    return text;
}

static str lang_c_str_from_token_list(lang_c_token_list tokens, edith_gapbuffer *gb, arena *arena) {
    str_list texts = { 0 };
    for_list (lang_c_token, token, tokens) {
        str text = lang_c_str_from_token(token, gb, arena);
        str_list_push(&texts, arena, text);

        if (token == tokens.last) {
            break;
        }
    }
    str joined = str_list_concat(&texts, arena);
    return joined;
}

static str lang_c_str_from_token_range(lang_c_token_range range, edith_gapbuffer *gb, arena *arena) {
    str_list texts = { 0 };
    for_list (lang_c_token_range_node, node, range) {
        str text = lang_c_str_from_token(node->token, gb, arena);
        str_list_push(&texts, arena, text);
    }

    str joined = str_list_concat(&texts, arena);
    return joined;
}

////////////////////////////////////////////////////////////////
// rune: Declaraion parsing

static bool lang_c_token_kind_is_specifier(lang_c_token_kind kind) {
    static readonly bool lang_c_specifier_tokens[LANG_C_TOKEN_KIND_COUNT] = {
        [LANG_C_TOKEN_KIND_TYPEDEF] = true,
        [LANG_C_TOKEN_KIND_AUTO]     = true,
        [LANG_C_TOKEN_KIND_STATIC]   = true,
        [LANG_C_TOKEN_KIND_EXTERN]   = true,

        [LANG_C_TOKEN_KIND_VOID]     = true,
        [LANG_C_TOKEN_KIND_BOOL]     = true,
        [LANG_C_TOKEN_KIND_CHAR]     = true,
        [LANG_C_TOKEN_KIND_INT]      = true,
        [LANG_C_TOKEN_KIND_SHORT]    = true,
        [LANG_C_TOKEN_KIND_LONG]     = true,
        [LANG_C_TOKEN_KIND_FLOAT]    = true,
        [LANG_C_TOKEN_KIND_DOUBLE]   = true,
        [LANG_C_TOKEN_KIND_SIGNED]   = true,
        [LANG_C_TOKEN_KIND_UNSIGNED] = true,
        [LANG_C_TOKEN_KIND_CONST]    = true,
        [LANG_C_TOKEN_KIND_VOLATILE] = true,

        [LANG_C_TOKEN_KIND_STRUCT]   = true,
        [LANG_C_TOKEN_KIND_UNION]    = true,
        [LANG_C_TOKEN_KIND_ENUM]     = true,

        [LANG_C_TOKEN_KIND_IDENT]    = true,
    };

    bool is_specifier = lang_c_specifier_tokens[kind];
    return is_specifier;
}

static edith_gapbuffer *lang_c_global_src_buffer; // TODO(rune): Remove global.

static void lang_c_frag_parse_declarator_recurse(lang_c_frag_parser *p, lang_c_declarator *decl, arena *arena) {
    // rune: Grouping parenthesis
    if (p->peek->kind == LANG_C_TOKEN_KIND_PAREN_OPEN) {
        lang_c_eat_token(p);

        if (p->peek->kind == LANG_C_TOKEN_KIND_IDENT &&
            p->peek->next &&
            p->peek->next->kind == LANG_C_TOKEN_KIND_PAREN_CLOSE) {
            p->error = true;
        }

        lang_c_frag_parse_declarator_recurse(p, decl, arena);
        if (p->peek->kind == LANG_C_TOKEN_KIND_PAREN_CLOSE) {
            lang_c_eat_token(p);
        } else {
            p->error = true;
        }
    }

    // rune: Pointer indirection
    else if (p->peek->kind == LANG_C_TOKEN_KIND_MUL) {
        lang_c_eat_token(p);

        // TODO(rune): Allow LANG_C_TOKEN_TYPE_KIND_IDENT as a qualifier, and backtrack if it results in an unsuccesful parse.
        lang_c_token_range qualifier_tokens = { 0 };
        while ((p->peek->kind == LANG_C_TOKEN_KIND_CONST) ||
               (p->peek->kind == LANG_C_TOKEN_KIND_VOLATILE)) {
            lang_c_token_range_push(&qualifier_tokens, p->peek, arena);
            lang_c_eat_token(p);
        }

        lang_c_frag_parse_declarator_recurse(p, decl, arena);

        lang_c_type *type = arena_push_struct(&lang_c_symbol_arena, lang_c_type);
        type->kind = LANG_C_TYPE_KIND_POINTER;
        type->text = lang_c_str_from_token_range(qualifier_tokens, p->gb, arena);
        slist_push(&decl->type_list, type);
    }

    // rune: Identifier
    else if (p->peek->kind == LANG_C_TOKEN_KIND_IDENT) {
        lang_c_token *name_token = lang_c_eat_token(p);
        if (decl->name_token == null) {
            decl->name_token = name_token;
        } else {
            p->error = true;
        }
    }

    if (p->error) {
        return;
    }

    // rune: Postfix operators
    bool decl_stop = false;
    while (decl_stop == false && p->peek->kind != 0) {
        // rune: Function call parenthesis
        if (p->peek->kind == LANG_C_TOKEN_KIND_PAREN_OPEN) {
            lang_c_eat_token(p);

            lang_c_symbol_list args = { 0 };
            bool arg_stop = false;
            while (arg_stop == false && p->peek->kind != 0) {
                lang_c_symbol_list list = lang_c_frag_parse_declaration(p, arena, LANG_C_FRAG_PARSE_FLAG_ARG);

                // TODO(rune): Cleanup
                if (list.count > 0) {
                    assert(list.count == 1);
                    lang_c_symbol *arg = list.first;
                    arg->next = null;
                    slist_push(&args, arg);
                    args.count++;
                }

                switch (p->peek->kind) {
                    case LANG_C_TOKEN_KIND_COMMA: {
                        lang_c_eat_token(p);
                    } break;

                    default: {
                        arg_stop = true;
                        decl_stop = true;
                    } break;
                }
            }

            lang_c_type *type = arena_push_struct(&lang_c_symbol_arena, lang_c_type);
            type->kind = LANG_C_TYPE_KIND_FUNC;
            type->args = args;
            slist_push(&decl->type_list, type);

            if (p->peek->kind == LANG_C_TOKEN_KIND_PAREN_CLOSE) {
                lang_c_eat_token(p);
            } else {
                // TODO(rune): Fix the bug that causes the tests to fail, when this line is uncommented.
                //p->error = true;
            }
        }

        // rune: Subscript
        else if (p->peek->kind == LANG_C_TOKEN_KIND_BRACKET_OPEN) {
            i64_range expr_range = { I64_MAX, 0 };

            // TODO(rune): Cleanup
            // TODO(rune): Cleanup
            // TODO(rune): Cleanup
            // TODO(rune): Cleanup
            // TODO(rune): Cleanup
            i64 level = 0;
            while (p->peek->kind != 0) {
                if (p->peek->kind == LANG_C_TOKEN_KIND_BRACKET_OPEN) {
                    if (level > 0) {
                        min_assign(&expr_range.min, p->peek->pos);
                        max_assign(&expr_range.max, p->peek->pos + p->peek->len);
                    }

                    level++;
                }

                else if (p->peek->kind == LANG_C_TOKEN_KIND_BRACKET_CLOSE) {
                    level--;

                    if (level > 0) {
                        min_assign(&expr_range.min, p->peek->pos);
                        max_assign(&expr_range.max, p->peek->pos + p->peek->len);
                    }
                }

                else {
                    min_assign(&expr_range.min, p->peek->pos);
                    max_assign(&expr_range.max, p->peek->pos + p->peek->len);
                }

                lang_c_eat_token(p);

                if (level <= 0) {
                    break;
                }
            }

            str expr = { 0 };
            if (expr_range.min < expr_range.max) {
                expr = edith_str_from_gapbuffer_range(p->gb, expr_range, &lang_c_symbol_arena);
            }

            lang_c_type *type = arena_push_struct(&lang_c_symbol_arena, lang_c_type);
            type->kind = LANG_C_TYPE_KIND_ARRAY;
            type->text = expr;
            slist_push(&decl->type_list, type);
        }

        else {
            decl_stop = true;
        }
    }
}

static lang_c_symbol *lang_c_frag_parse_declarator(lang_c_frag_parser *p, lang_c_token_range specifier_tokens, lang_c_frag_parse_flags flags, arena *arena) {
    lang_c_declarator declarator = { 0 };
    lang_c_frag_parse_declarator_recurse(p, &declarator, arena);

    lang_c_type *basic_type = arena_push_struct(arena, lang_c_type);
    basic_type->kind = LANG_C_TYPE_KIND_BASIC;
    basic_type->text = lang_c_str_from_token_range(specifier_tokens, p->gb, arena);
    slist_push(&declarator.type_list, basic_type);

    if (p->error) {
        return null;
    }

    bool allow_unnamed = flags & LANG_C_FRAG_PARSE_FLAG_ARG;
    if (declarator.name_token == null && allow_unnamed == false) {
        return null;
    }

    lang_c_symbol *symbol = arena_push_struct(arena, lang_c_symbol);
    symbol->type = declarator.type_list;

    if (declarator.name_token) {
        symbol->name = lang_c_str_from_token(declarator.name_token, p->gb, arena);
    } else {
        symbol->name = str("<unnamed>");
    }

    return symbol;
}

static lang_c_symbol_list lang_c_frag_parse_declaration(lang_c_frag_parser *p, arena *arena, lang_c_frag_parse_flags flags) {
    lang_c_global_src_buffer = p->gb; // TODO(rune): Remove global.

    lang_c_token_range specifier_tokens = { 0 };
    while (lang_c_token_kind_is_specifier(p->peek->kind)) {
        lang_c_token_range_push(&specifier_tokens, p->peek, arena);
        lang_c_eat_token(p);
    }

    lang_c_symbol_list a = { 0 };
    lang_c_symbol_list b = { 0 };

    // rune: First attempt
    if (specifier_tokens.count >= 1) {
        lang_c_symbol *symbol = lang_c_frag_parse_declarator(p, specifier_tokens, flags, arena);
        if (symbol && ((flags & LANG_C_FRAG_PARSE_FLAG_ARG) || (p->peek->kind == 0) || (p->peek->kind == LANG_C_TOKEN_KIND_SEMICOLON) || (p->peek->kind == LANG_C_TOKEN_KIND_ASSIGN))) {
            slist_push(&a, symbol);
            a.count++;
        }
    }

    // rune: Second attempt
    if (specifier_tokens.count >= 2) {
        lang_c_token *possible_ident = lang_c_token_range_pop(&specifier_tokens);
        p->peek = possible_ident;
        p->error = false;

        lang_c_symbol *symbol = lang_c_frag_parse_declarator(p, specifier_tokens, flags, arena);
        if (symbol && ((flags & LANG_C_FRAG_PARSE_FLAG_ARG) || (p->peek->kind == 0) || (p->peek->kind == LANG_C_TOKEN_KIND_SEMICOLON) || (p->peek->kind == LANG_C_TOKEN_KIND_ASSIGN))) {
            slist_push(&b, symbol);
            b.count++;
        }
    }

    // rune: Choose attempt
    lang_c_symbol_list result = { 0 };
    if (a.count > 0 && b.count == 0) {
        result = a;
    } else if (a.count == 0 && b.count > 0) {
        result = b;
    } else if (a.count > 0 && b.count > 0) {
        if (b.first->type.first->kind == LANG_C_TYPE_KIND_FUNC) {
            result = a;
        } else {
            result = b;
        }
    }

    return result;
}

