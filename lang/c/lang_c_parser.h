////////////////////////////////////////////////////////////////
// rune: Parser

typedef struct lang_c_frag_parser lang_c_frag_parser;
struct lang_c_frag_parser {
    lang_c_token_list token_list; // TODO(rune): Only for debug print
    lang_c_token *peek;
    bool error;
    edith_gapbuffer *gb;
};

static void           lang_c_frag_parser_init(lang_c_frag_parser *p, i64_range range, edith_gapbuffer *src_buffer);
static lang_c_token  *lang_c_eat_token(lang_c_frag_parser *p);

////////////////////////////////////////////////////////////////
// rune: Token range

typedef struct lang_c_token_range_node lang_c_token_range_node;
struct lang_c_token_range_node {
    lang_c_token *token;
    lang_c_token_range_node *next;
    lang_c_token_range_node *prev;
};

typedef struct lang_c_token_range lang_c_token_range;
struct lang_c_token_range {
    lang_c_token_range_node *first;
    lang_c_token_range_node *last;
    i64 count;
};

static void          lang_c_token_range_push(lang_c_token_range *range, lang_c_token *token, arena *arena);
static lang_c_token *lang_c_token_range_pop(lang_c_token_range *range);

static str lang_c_str_from_token(lang_c_token *token, edith_gapbuffer *gb, arena *arena);
static str lang_c_str_from_token_list(lang_c_token_list tokens, edith_gapbuffer *gb, arena *arena);
static str lang_c_str_from_token_range(lang_c_token_range range, edith_gapbuffer *gb, arena *arena);

////////////////////////////////////////////////////////////////
// rune: Declaration parsing

typedef struct lang_c_declarator lang_c_declarator;
struct lang_c_declarator {
    lang_c_token *name_token;
    lang_c_type_list type_list;
};

typedef enum lang_c_frag_parse_flags {
    LANG_C_FRAG_PARSE_FLAG_ARG = 1,
} lang_c_frag_parse_flags;

static void                 lang_c_frag_parse_declarator_recurse(lang_c_frag_parser *p, lang_c_declarator *decl, arena *arena);
static lang_c_symbol_list   lang_c_frag_parse_declaration(lang_c_frag_parser *p, arena *arena, lang_c_frag_parse_flags flags);
static void                 lang_c_frag_parse_declaration_list(lang_c_frag_parser *p);
