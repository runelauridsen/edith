////////////////////////////////////////////////////////////////
// rune: Helpers

static u8 lang_c_gb_get_byte(edith_gapbuffer *gb, i64 pos) {
    i64 gap_len = gb->gap.max - gb->gap.min;
    if (pos < gb->gap.min) {
        return gb->buf[pos];
    } else if (pos + gap_len < gb->buf_cap) {
        return gb->buf[pos + gap_len];
    } else {
        return 0;
    }
}

static bool lang_c_gb_range_eq(edith_gapbuffer *gb, i64 pos, i64 len, char *check) {
    bool full_match = true;
    for_n (i64, i, len) {
        u8 a = lang_c_gb_get_byte(gb, pos + i);
        u8 b = check[i];
        if (a != b) {
            full_match = false;
            break;
        }
    }

    return full_match;
}

static lang_c_token_kind lang_c_keyword_from_text(i64 pos, i64 len, edith_gapbuffer *gb) {
    // TODO(rune): Update codegen
    // NOTE(rune): Switch-case generated with:
#if 0
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#define countof(a)          (sizeof(a) / sizeof(*(a)))

    char *kw[] = {
        "alignas", "alignof", "auto", "bool", "break", "case",
        "char", "const", "constexpr", "continue", "default",
        "do", "double", "else", "enum", "extern", "false",
        "float", "for", "goto", "if", "inline", "int",
        "long", "nullptr", "register", "restrict", "return",
        "short", "signed", "sizeof", "static", "static_assert",
        "struct", "switch", "thread_local", "true", "typedef",
        "typeof", "typeof_unqual", "union", "unsigned", "void",
        "volatile", "while", "_Alignas", "_Alignof", "_Atomic",
        "_BitInt", "_Bool", "_Complex", "_Decimal128", "_Decimal32",
        "_Decimal64", "_Generic", "_Imaginary", "_Noreturn", "_Static_assert",
        "_Thread_local"
    };

    static int strcmplen(const char **a, const char **b) {
        const char *str_a = *a;
        const char *str_b = *b;

        int a_len = strlen(str_a);
        int b_len = strlen(str_b);
        if (a_len > b_len) return  1;
        if (a_len < b_len) return -1;
        else               return  0;
    }

    int main(void) {
        qsort(kw, countof(kw), sizeof(kw[0]), strcmplen);

        int current_len = 0;
        for (int i = 0; i < countof(kw); i++) {
            int len = strlen(kw[i]);
            if (len != current_len) {

                if (current_len != 0) {
                    printf("    return false;\n");
                }

                printf("case %i:\n", len);
                current_len = len;
            }

            // if (lang_c_gb_range_eq(gb, pos, 4, "long")) return true;
            printf("    if (lang_c_gb_range_eq(gb, pos, %i, \"%s\")) return true;\n", len, kw[i]);
        }

        printf("default:\n");
        printf("    return false;\n");
    }
#endif

    switch (len) {
        case 2:
            if (lang_c_gb_range_eq(gb, pos, 2, "do")) return LANG_C_TOKEN_KIND_DO;
            if (lang_c_gb_range_eq(gb, pos, 2, "if")) return LANG_C_TOKEN_KIND_IF;
            return false;
        case 3:
            if (lang_c_gb_range_eq(gb, pos, 3, "int")) return LANG_C_TOKEN_KIND_INT;
            if (lang_c_gb_range_eq(gb, pos, 3, "for")) return LANG_C_TOKEN_KIND_FOR;
            return false;
        case 4:
            if (lang_c_gb_range_eq(gb, pos, 4, "long")) return LANG_C_TOKEN_KIND_LONG;
            if (lang_c_gb_range_eq(gb, pos, 4, "bool")) return LANG_C_TOKEN_KIND_BOOL;
            if (lang_c_gb_range_eq(gb, pos, 4, "auto")) return LANG_C_TOKEN_KIND_AUTO;
            if (lang_c_gb_range_eq(gb, pos, 4, "void")) return LANG_C_TOKEN_KIND_VOID;
            if (lang_c_gb_range_eq(gb, pos, 4, "goto")) return LANG_C_TOKEN_KIND_GOTO;
            if (lang_c_gb_range_eq(gb, pos, 4, "true")) return LANG_C_TOKEN_KIND_TRUE;
            if (lang_c_gb_range_eq(gb, pos, 4, "char")) return LANG_C_TOKEN_KIND_CHAR;
            if (lang_c_gb_range_eq(gb, pos, 4, "case")) return LANG_C_TOKEN_KIND_CASE;
            if (lang_c_gb_range_eq(gb, pos, 4, "else")) return LANG_C_TOKEN_KIND_ELSE;
            if (lang_c_gb_range_eq(gb, pos, 4, "enum")) return LANG_C_TOKEN_KIND_ENUM;
            return false;
        case 5:
            if (lang_c_gb_range_eq(gb, pos, 5, "const")) return LANG_C_TOKEN_KIND_CONST;
            if (lang_c_gb_range_eq(gb, pos, 5, "false")) return LANG_C_TOKEN_KIND_FALSE;
            if (lang_c_gb_range_eq(gb, pos, 5, "float")) return LANG_C_TOKEN_KIND_FLOAT;
            if (lang_c_gb_range_eq(gb, pos, 5, "_Bool")) return LANG_C_TOKEN_KIND_BOOL;
            if (lang_c_gb_range_eq(gb, pos, 5, "union")) return LANG_C_TOKEN_KIND_UNION;
            if (lang_c_gb_range_eq(gb, pos, 5, "while")) return LANG_C_TOKEN_KIND_WHILE;
            if (lang_c_gb_range_eq(gb, pos, 5, "break")) return LANG_C_TOKEN_KIND_BREAK;
            if (lang_c_gb_range_eq(gb, pos, 5, "short")) return LANG_C_TOKEN_KIND_SHORT;
            return false;
        case 6:
            if (lang_c_gb_range_eq(gb, pos, 6, "typeof")) return LANG_C_TOKEN_KIND_TYPEOF;
            if (lang_c_gb_range_eq(gb, pos, 6, "double")) return LANG_C_TOKEN_KIND_DOUBLE;
            if (lang_c_gb_range_eq(gb, pos, 6, "switch")) return LANG_C_TOKEN_KIND_SWITCH;
            if (lang_c_gb_range_eq(gb, pos, 6, "return")) return LANG_C_TOKEN_KIND_RETURN;
            if (lang_c_gb_range_eq(gb, pos, 6, "extern")) return LANG_C_TOKEN_KIND_EXTERN;
            if (lang_c_gb_range_eq(gb, pos, 6, "inline")) return LANG_C_TOKEN_KIND_INLINE;
            if (lang_c_gb_range_eq(gb, pos, 6, "signed")) return LANG_C_TOKEN_KIND_SIGNED;
            if (lang_c_gb_range_eq(gb, pos, 6, "sizeof")) return LANG_C_TOKEN_KIND_SIZEOF;
            if (lang_c_gb_range_eq(gb, pos, 6, "static")) return LANG_C_TOKEN_KIND_STATIC;
            if (lang_c_gb_range_eq(gb, pos, 6, "struct")) return LANG_C_TOKEN_KIND_STRUCT;
            return false;
        case 7:
            if (lang_c_gb_range_eq(gb, pos, 7, "alignas")) return LANG_C_TOKEN_KIND_ALIGNAS;
            if (lang_c_gb_range_eq(gb, pos, 7, "_Atomic")) return LANG_C_TOKEN_KIND_ATOMIC;
            if (lang_c_gb_range_eq(gb, pos, 7, "alignof")) return LANG_C_TOKEN_KIND_ALIGNOF;
            if (lang_c_gb_range_eq(gb, pos, 7, "_BitInt")) return LANG_C_TOKEN_KIND_BITINT;
            if (lang_c_gb_range_eq(gb, pos, 7, "typedef")) return LANG_C_TOKEN_KIND_TYPEDEF;
            if (lang_c_gb_range_eq(gb, pos, 7, "nullptr")) return LANG_C_TOKEN_KIND_NULLPTR;
            if (lang_c_gb_range_eq(gb, pos, 7, "default")) return LANG_C_TOKEN_KIND_DEFAULT;
            return false;
        case 8:
            if (lang_c_gb_range_eq(gb, pos, 8, "_Complex")) return LANG_C_TOKEN_KIND_COMPLEX;
            if (lang_c_gb_range_eq(gb, pos, 8, "register")) return LANG_C_TOKEN_KIND_REGISTER;
            if (lang_c_gb_range_eq(gb, pos, 8, "continue")) return LANG_C_TOKEN_KIND_CONTINUE;
            if (lang_c_gb_range_eq(gb, pos, 8, "volatile")) return LANG_C_TOKEN_KIND_VOLATILE;
            if (lang_c_gb_range_eq(gb, pos, 8, "_Generic")) return LANG_C_TOKEN_KIND_GENERIC;
            if (lang_c_gb_range_eq(gb, pos, 8, "_Alignas")) return LANG_C_TOKEN_KIND_ALIGNAS;
            if (lang_c_gb_range_eq(gb, pos, 8, "_Alignof")) return LANG_C_TOKEN_KIND_ALIGNOF;
            if (lang_c_gb_range_eq(gb, pos, 8, "restrict")) return LANG_C_TOKEN_KIND_RESTRICT;
            if (lang_c_gb_range_eq(gb, pos, 8, "unsigned")) return LANG_C_TOKEN_KIND_UNSIGNED;
            return false;
        case 9:
            if (lang_c_gb_range_eq(gb, pos, 9, "_Noreturn")) return LANG_C_TOKEN_KIND_NORETURN;
            if (lang_c_gb_range_eq(gb, pos, 9, "constexpr")) return LANG_C_TOKEN_KIND_CONSTEXPR;
            return false;
        case 10:
            if (lang_c_gb_range_eq(gb, pos, 10, "_Decimal32")) return LANG_C_TOKEN_KIND_DECIMAL32;
            if (lang_c_gb_range_eq(gb, pos, 10, "_Decimal64")) return LANG_C_TOKEN_KIND_DECIMAL64;
            if (lang_c_gb_range_eq(gb, pos, 10, "_Imaginary")) return LANG_C_TOKEN_KIND_IMAGINARY;
            return false;
        case 11:
            if (lang_c_gb_range_eq(gb, pos, 11, "_Decimal128")) return LANG_C_TOKEN_KIND_DECIMAL128;
            return false;
        case 12:
            if (lang_c_gb_range_eq(gb, pos, 12, "thread_local")) return LANG_C_TOKEN_KIND_THREAD_LOCAL;
            return false;
        case 13:
            if (lang_c_gb_range_eq(gb, pos, 13, "static_assert")) return LANG_C_TOKEN_KIND_STATIC_ASSERT;
            if (lang_c_gb_range_eq(gb, pos, 13, "_Thread_local")) return LANG_C_TOKEN_KIND_THREAD_LOCAL;
            if (lang_c_gb_range_eq(gb, pos, 13, "typeof_unqual")) return LANG_C_TOKEN_KIND_TYPEOF_UNQUAL;
            return false;
        case 14:
            if (lang_c_gb_range_eq(gb, pos, 14, "_Static_assert")) return LANG_C_TOKEN_KIND_STATIC_ASSERT;
        default:
            return false;
    }
}

////////////////////////////////////////////////////////////////
// rune: Lexer

static u8 lang_c_lexer_eat(lang_c_lexer *l) {
    u8 ret = l->peek0;
    if (l->at < edith_gapbuffer_len(l->src)) {
        l->at++;
    }

    l->peek0 = l->peek1;
    l->peek1 = l->peek2;
    l->peek2 = lang_c_gb_get_byte(l->src, l->at + 2);

    return ret;
}

typedef struct lang_c_token_spelling lang_c_token_spelling;
struct lang_c_token_spelling {
    str spelling;
    lang_c_token_kind kind;
};

static lang_c_token_spelling lang_c_punct_spellings_2[] = {
    { STR("++"), LANG_C_TOKEN_KIND_INC                 },
    { STR("--"), LANG_C_TOKEN_KIND_DEC                 },
    { STR("->"), LANG_C_TOKEN_KIND_ARROW               },
    { STR("<<"), LANG_C_TOKEN_KIND_BITSHIFT_LEFT       },
    { STR(">>"), LANG_C_TOKEN_KIND_BITSHIFT_RIGHT      },
    { STR("<="), LANG_C_TOKEN_KIND_LESS_OR_EQUAL       },
    { STR(">="), LANG_C_TOKEN_KIND_GREATER_OR_EQUAL    },
    { STR("=="), LANG_C_TOKEN_KIND_EQUAL               },
    { STR("!="), LANG_C_TOKEN_KIND_NOT_EQUAL           },
    { STR("&&"), LANG_C_TOKEN_KIND_AND                 },
    { STR("||"), LANG_C_TOKEN_KIND_OR                  },
    { STR("+="), LANG_C_TOKEN_KIND_ADD_ASSIGN          },
    { STR("-="), LANG_C_TOKEN_KIND_SUB_ASSIGN          },
    { STR("*="), LANG_C_TOKEN_KIND_MUL_ASSIGN          },
    { STR("/="), LANG_C_TOKEN_KIND_DIV_ASSIGN          },
    { STR("%="), LANG_C_TOKEN_KIND_MOD_ASSIGN          },
    { STR("&="), LANG_C_TOKEN_KIND_BITWISE_AND_ASSIGN  },
    { STR("^="), LANG_C_TOKEN_KIND_BITWISE_XOR_ASSIGN  },
    { STR("|="), LANG_C_TOKEN_KIND_BITWISE_OR_ASSIGN   },
};

static lang_c_token_spelling lang_c_punct_spellings_3[] = {
    { STR("&&="), LANG_C_TOKEN_KIND_AND_ASSIGN             },
    { STR("||="), LANG_C_TOKEN_KIND_OR_ASSIGN              },
    { STR("<<="), LANG_C_TOKEN_KIND_BITSHIFT_LEFT_ASSIGN   },
    { STR(">>="), LANG_C_TOKEN_KIND_BITSHIFT_RIGHT_ASSIGN  },
};

#define lang_c_u8_is_whitespace(c)             (char_flags_table[(c)] & CHAR_FLAG_WHITESPACE)
#define lang_c_u8_is_digit(c)                  (char_flags_table[(c)] & CHAR_FLAG_DIGIT)
#define lang_c_u8_is_letter(c)                 (char_flags_table[(c)] & CHAR_FLAG_LETTER)
#define lang_c_u8_is_punct(c)                  (char_flags_table[(c)] & CHAR_FLAG_PUNCT)
#define lang_c_u8_is_hexdigit(c)               (char_flags_table[(c)] & CHAR_FLAG_HEXDIGIT)

//#define LANG_C_INSTRUMENTATION

#ifdef LANG_C_INSTRUMENTATION
static i64 token_kind_rdtsc[LANG_C_TOKEN_KIND_COUNT]   = { 0 };
static i64 token_kind_counter[LANG_C_TOKEN_KIND_COUNT] = { 0 };
static i64 token_kind_len[LANG_C_TOKEN_KIND_COUNT]     = { 0 };
#endif

static bool lang_c_lexer_next_token(lang_c_lexer *l, lang_c_token *token) {

    // rune: Eat whitespace
    while (u8_is_whitespace(l->peek0)) {
        lang_c_lexer_eat(l);
    }

    lang_c_token_kind kind = 0;
    while (!kind) {

        i64 begin = l->at;

#ifdef LANG_C_INSTRUMENTATION
        i64 rdtsc_begin = __rdtsc();
#endif
        // rune: EOF?
        if (l->peek0 == 0) {
            break;
        }

        // rune: Preproccesor
        else if (l->peek0 == '#') {
            kind = LANG_C_TOKEN_KIND_PREPROC;
            bool continuation = false;
            while (1) {
                u8 c = l->peek0;

                if (c == 0) {
                    break;
                }

                if (c == '\n') {
                    if (continuation) {
                        continuation = false;
                    } else {
                        break;
                    }
                }

                if (!u8_is_whitespace(c)) {
                    continuation = false;
                }

                if (c == '\\') {
                    continuation = true;
                }


                lang_c_lexer_eat(l);
            }
        }

        // rune: Line comment
        else if (l->peek0 == '/' && l->peek1 == '/') {
            kind = LANG_C_TOKEN_KIND_COMMENT;
            while (1) {
                u8 c = l->peek0;
                if (c != '\0' && c != '\n' && c != '\r') {
                    lang_c_lexer_eat(l);
                } else {
                    break;
                }
            }
        }

        // rune: Block comment
        else if (l->peek0 == '/' && l->peek1 == '*') {
            kind = LANG_C_TOKEN_KIND_COMMENT;
            u8 c0 = l->peek0;
            u8 cprev = c0;
            while (1) {
                cprev = c0;
                c0 = lang_c_lexer_eat(l);

                if (c0 == '/' && cprev == '*') {
                    break;
                }

                if (c0 == '\0') {
                    break;
                }
            }
        }

        // rune: String literal
        else if (l->peek0 == '"') {
            kind = LANG_C_TOKEN_KIND_STRING;
            lang_c_lexer_eat(l);
            bool escaped = false;
            while (1) {
                u8 c = lang_c_lexer_eat(l);
                if (c == 0) {
                    break;
                }

                if (c == '"' && !escaped) {
                    break;
                }

                if (c == '\\' && !escaped) {
                    escaped = true;
                } else {
                    escaped = false;
                }
            }
        }

        // rune: Char literal
        else if (l->peek0 == '\'') {
            kind = LANG_C_TOKEN_KIND_STRING;
            lang_c_lexer_eat(l);
            bool escaped = false;
            while (1) {
                u8 c = lang_c_lexer_eat(l);
                if (c == 0) {
                    break;
                }

                if (c == '\'' && !escaped) {
                    break;
                }

                if (c == '\\' && !escaped) {
                    escaped = true;
                } else {
                    escaped = false;
                }
            }
        }

        // rune: Hexadecimal number
        else if (l->peek0 == '0' && l->peek1 == 'x') {
            kind = LANG_C_TOKEN_KIND_NUMBER;
            lang_c_lexer_eat(l);
            lang_c_lexer_eat(l);
            while (1) {
                u8 c = l->peek0;
                if (lang_c_u8_is_hexdigit(c) || c == '\'' || c == '_') {
                    lang_c_lexer_eat(l);
                } else {
                    break;
                }
            }
        }

        // rune: Binary number
        else if (l->peek0 == '0' && l->peek1 == 'b') {
            kind = LANG_C_TOKEN_KIND_NUMBER;
            lang_c_lexer_eat(l);
            lang_c_lexer_eat(l);
            while (1) {
                u8 c = l->peek0;
                if (c == '0' || c == '1' || c == '\'' || c == '_') {
                    lang_c_lexer_eat(l);
                } else {
                    break;
                }
            }
        }

        // rune: Decimal number
        else if (lang_c_u8_is_digit(l->peek0)) {
            kind = LANG_C_TOKEN_KIND_NUMBER;
            while (1) {
                u8 c = l->peek0;
                if (lang_c_u8_is_digit(c) || c == '\'' || c == '_') {
                    lang_c_lexer_eat(l);
                } else {
                    break;
                }
            }
        }

        // rune: Operators/punctuation
        else if (lang_c_u8_is_punct(l->peek0)) {
            // rune: 3-character operator
            if (kind == 0) {
                for_sarray (lang_c_token_spelling, it, lang_c_punct_spellings_3) {
                    if (it->spelling.v[0] == l->peek0 &&
                        it->spelling.v[1] == l->peek1 &&
                        it->spelling.v[2] == l->peek2) {
                        kind = it->kind;
                        lang_c_lexer_eat(l);
                        lang_c_lexer_eat(l);
                        lang_c_lexer_eat(l);
                        break;
                    }
                }
            }

            // rune: 2-character operator
            if (kind == 0) {
                for_sarray (lang_c_token_spelling, it, lang_c_punct_spellings_2) {
                    if (it->spelling.v[0] == l->peek0 &&
                        it->spelling.v[1] == l->peek1) {
                        kind = it->kind;
                        lang_c_lexer_eat(l);
                        lang_c_lexer_eat(l);
                        break;
                    }
                }
            }

            // rune: 1-character operator/punctuation
            if (kind == 0) {
                kind = l->peek0;
                lang_c_lexer_eat(l);
            }
        }

        // rune: Ident or keyword
        else if (u8_is_letter(l->peek0) || l->peek0 == '_') {
            while (1) {
                u8 c = l->peek0;
                if (c != '\0' && (u8_is_letter(c) || u8_is_digit(c) || c == '_')) {
                    lang_c_lexer_eat(l);
                } else {
                    break;
                }
            }

            i64 len = l->at - begin;
            lang_c_token_kind keyword = lang_c_keyword_from_text(begin, len, l->src);
            if (keyword) {
                kind = keyword;
            } else {
                kind = LANG_C_TOKEN_KIND_IDENT;
            }
        }

#ifdef LANG_C_INSTRUMENTATION
        i64 rdtsc_end = __rdtsc();
#endif

        i64 end = l->at;
        if (kind) {
            *token = (lang_c_token) {
                .kind     = kind,
                .pos      = begin,
                .len      = end - begin,
            };

#ifdef LANG_C_INSTRUMENTATION
            token_kind_counter[kind] += 1;
            token_kind_len[kind]     += end - begin;
            token_kind_rdtsc[kind]   += rdtsc_end - rdtsc_begin;
#endif

            return true;
        } else {
            // NOTE(rune): We couldn't recognise any token, so we just ignore at skip to next character.
            lang_c_lexer_eat(l);
        }
    }

    return false;
}

static void lang_c_lexer_init(lang_c_lexer *l, i64 start_pos, edith_gapbuffer *src_buffer) {
    *l = (lang_c_lexer) {
        .at    = start_pos,
        .src   = src_buffer,
        .peek0 = lang_c_gb_get_byte(src_buffer, start_pos),
        .peek1 = lang_c_gb_get_byte(src_buffer, start_pos + 1),
        .peek2 = lang_c_gb_get_byte(src_buffer, start_pos + 2),
    };
}

////////////////////////////////////////////////////////////////
// rune: String to tokens

static lang_c_token_list lang_c_token_list_from_str(str s, arena *arena) {
    edith_gapbuffer gb = { .buf = s.v, .buf_cap = s.len };
    i64_range range = { 0, s.len };
    lang_c_token_list list = lang_c_token_list_from_gapbuffer_range(&gb, range, arena);
    return list;
}

static lang_c_token_list lang_c_token_list_from_gapbuffer(edith_gapbuffer *gb, arena *arena) {
    i64 len = edith_gapbuffer_len(gb);
    i64_range range = { 0, len };
    lang_c_token_list list = lang_c_token_list_from_gapbuffer_range(gb, range, arena);
    return list;
}

static lang_c_token_list lang_c_token_list_from_gapbuffer_range(edith_gapbuffer *gb, i64_range range, arena *arena) {
    lang_c_lexer lexer = { 0 };
    lang_c_lexer_init(&lexer, range.min, gb);

    lang_c_token token = { 0 };
    lang_c_token_list list = { 0 };
    while (lang_c_lexer_next_token(&lexer, &token)) {
        if (token.pos >= range.max) break;

        lang_c_token *node = arena_push_struct(arena, lang_c_token);
        *node = token;

        slist_push(&list, node);
        list.count++;
    }

    return list;
}

////////////////////////////////////////////////////////////////
// rune: Debug print

static void lang_c_debug_print_token(lang_c_token *token, edith_gapbuffer *src_buffer) {
    u8 scratch[4096];

    i64 len = clamp_top(token->len, sizeof(scratch));
    edith_gapbuffer_copy_range(src_buffer, token->pos, token->pos + len, scratch);
    str s = str_make(scratch, len);

    println("%    \t pos = % \t len = % \t %%(literal)" ANSI_FG_DEFAULT,
            LANG_C_TOKEN_KIND_infos[token->kind].name,
            token->pos,
            token->len,
            LANG_C_TOKEN_KIND_infos[token->kind].ansiconsole_color,
            s);
}
