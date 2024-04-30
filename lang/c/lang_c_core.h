////////////////////////////////////////////////////////////////
// rune: Configuration

#define LANG_C_DEBUG_PRINT_FRAG_TREE   0

////////////////////////////////////////////////////////////////
// rune: Tokens

typedef enum lang_c_token_kind {
    LANG_C_TOKEN_KIND_NONE = 0,

    // rune: Punctuation
    LANG_C_TOKEN_KIND_PAREN_OPEN       = '(',
    LANG_C_TOKEN_KIND_PAREN_CLOSE      = ')',
    LANG_C_TOKEN_KIND_BRACE_OPEN       = '{',
    LANG_C_TOKEN_KIND_BRACE_CLOSE      = '}',
    LANG_C_TOKEN_KIND_BRACKET_OPEN     = '[',
    LANG_C_TOKEN_KIND_BRACKET_CLOSE    = ']',
    LANG_C_TOKEN_KIND_SEMICOLON        = ';',

    // rune: Single character operators
    LANG_C_TOKEN_KIND_DOT           = '.',
    LANG_C_TOKEN_KIND_COMMA         = ',',
    LANG_C_TOKEN_KIND_ADD           = '+',
    LANG_C_TOKEN_KIND_SUB           = '-',
    LANG_C_TOKEN_KIND_MUL           = '*',
    LANG_C_TOKEN_KIND_DIV           = '/',
    LANG_C_TOKEN_KIND_MOD           = '%',
    LANG_C_TOKEN_KIND_NOT           = '!',
    LANG_C_TOKEN_KIND_ASSIGN        = '=',
    LANG_C_TOKEN_KIND_LESS          = '<',
    LANG_C_TOKEN_KIND_GREATER       = '>',
    LANG_C_TOKEN_KIND_BITWISE_AND   = '&',
    LANG_C_TOKEN_KIND_BITWISE_OR    = '|',
    LANG_C_TOKEN_KIND_BITWISE_NOT   = '~',
    LANG_C_TOKEN_KIND_BITWISE_XOR   = '^',
    LANG_C_TOKEN_KIND_QUESTION      = '?',
    LANG_C_TOKEN_KIND_COLON         = ':',

    // rune: Multi character operators
    LANG_C_TOKEN_KIND_BITSHIFT_LEFT = 128,  // <<
    LANG_C_TOKEN_KIND_BITSHIFT_RIGHT,       // >>
    LANG_C_TOKEN_KIND_EQUAL,                // ==
    LANG_C_TOKEN_KIND_NOT_EQUAL,            // !=
    LANG_C_TOKEN_KIND_AND,                  // &&
    LANG_C_TOKEN_KIND_OR,                   // ||
    LANG_C_TOKEN_KIND_INC,                  // ++
    LANG_C_TOKEN_KIND_DEC,                  // --
    LANG_C_TOKEN_KIND_ARROW,                // ->
    LANG_C_TOKEN_KIND_LESS_OR_EQUAL,        // <=
    LANG_C_TOKEN_KIND_GREATER_OR_EQUAL,     // >=

    // rune: Multi character assignment operators
    LANG_C_TOKEN_KIND_ADD_ASSIGN,             // +=
    LANG_C_TOKEN_KIND_SUB_ASSIGN,             // -=
    LANG_C_TOKEN_KIND_MUL_ASSIGN,             // *=
    LANG_C_TOKEN_KIND_DIV_ASSIGN,             // /=
    LANG_C_TOKEN_KIND_MOD_ASSIGN,             // %=
    LANG_C_TOKEN_KIND_BITWISE_AND_ASSIGN,     // &=
    LANG_C_TOKEN_KIND_BITWISE_OR_ASSIGN,      // |=
    LANG_C_TOKEN_KIND_BITWISE_XOR_ASSIGN,     // ^=
    LANG_C_TOKEN_KIND_BITSHIFT_LEFT_ASSIGN,   // <<=
    LANG_C_TOKEN_KIND_BITSHIFT_RIGHT_ASSIGN,  // >>=
    LANG_C_TOKEN_KIND_AND_ASSIGN,             // &&=
    LANG_C_TOKEN_KIND_OR_ASSIGN,              // ||=

    // rune: Genereal
    LANG_C_TOKEN_KIND_IDENT,
    LANG_C_TOKEN_KIND_STRING,
    LANG_C_TOKEN_KIND_NUMBER,

    // rune: Keywords
    LANG_C_TOKEN_KIND_AUTO,
    LANG_C_TOKEN_KIND_BOOL,
    LANG_C_TOKEN_KIND_BREAK,
    LANG_C_TOKEN_KIND_CASE,
    LANG_C_TOKEN_KIND_CHAR,
    LANG_C_TOKEN_KIND_CONST,
    LANG_C_TOKEN_KIND_CONSTEXPR,
    LANG_C_TOKEN_KIND_CONTINUE,
    LANG_C_TOKEN_KIND_DEFAULT,
    LANG_C_TOKEN_KIND_DO,
    LANG_C_TOKEN_KIND_DOUBLE,
    LANG_C_TOKEN_KIND_ELSE,
    LANG_C_TOKEN_KIND_ENUM,
    LANG_C_TOKEN_KIND_EXTERN,
    LANG_C_TOKEN_KIND_FALSE,
    LANG_C_TOKEN_KIND_FLOAT,
    LANG_C_TOKEN_KIND_FOR,
    LANG_C_TOKEN_KIND_GOTO,
    LANG_C_TOKEN_KIND_IF,
    LANG_C_TOKEN_KIND_INLINE,
    LANG_C_TOKEN_KIND_INT,
    LANG_C_TOKEN_KIND_LONG,
    LANG_C_TOKEN_KIND_NULLPTR,
    LANG_C_TOKEN_KIND_REGISTER,
    LANG_C_TOKEN_KIND_RESTRICT,
    LANG_C_TOKEN_KIND_RETURN,
    LANG_C_TOKEN_KIND_SHORT,
    LANG_C_TOKEN_KIND_SIGNED,
    LANG_C_TOKEN_KIND_SIZEOF,
    LANG_C_TOKEN_KIND_STATIC,
    LANG_C_TOKEN_KIND_STATIC_ASSERT,
    LANG_C_TOKEN_KIND_STRUCT,
    LANG_C_TOKEN_KIND_SWITCH,
    LANG_C_TOKEN_KIND_THREAD_LOCAL,
    LANG_C_TOKEN_KIND_TRUE,
    LANG_C_TOKEN_KIND_TYPEDEF,
    LANG_C_TOKEN_KIND_TYPEOF,
    LANG_C_TOKEN_KIND_TYPEOF_UNQUAL,
    LANG_C_TOKEN_KIND_UNION,
    LANG_C_TOKEN_KIND_UNSIGNED,
    LANG_C_TOKEN_KIND_VOID,
    LANG_C_TOKEN_KIND_VOLATILE,
    LANG_C_TOKEN_KIND_WHILE,
    LANG_C_TOKEN_KIND_ALIGNAS,
    LANG_C_TOKEN_KIND_ALIGNOF,
    LANG_C_TOKEN_KIND_ATOMIC,
    LANG_C_TOKEN_KIND_BITINT,
    LANG_C_TOKEN_KIND_COMPLEX,
    LANG_C_TOKEN_KIND_DECIMAL128,
    LANG_C_TOKEN_KIND_DECIMAL32,
    LANG_C_TOKEN_KIND_DECIMAL64,
    LANG_C_TOKEN_KIND_GENERIC,
    LANG_C_TOKEN_KIND_IMAGINARY,
    LANG_C_TOKEN_KIND_NORETURN,

    LANG_C_TOKEN_KIND_PREPROC,
    LANG_C_TOKEN_KIND_COMMENT,

    LANG_C_TOKEN_KIND_EOF,

    LANG_C_TOKEN_KIND_COUNT,
} lang_c_token_kind;

typedef enum lang_c_token_flags {
    LANG_C_TOKEN_FLAG_NONE,
} lang_c_token_flags;

typedef struct lang_c_token lang_c_token;
struct lang_c_token {
    lang_c_token_kind kind;
    lang_c_token_flags flags;
    i64 len;
    i64 pos;

    // TODO(rune): Semantic classification
    lang_c_token *next;
};

static readonly lang_c_token lang_c_null_token = { 0 };

////////////////////////////////////////////////////////////////
// rune: Fragment tree

typedef enum lang_c_frag_kind {
    LANG_C_FRAG_KIND_NONE,
    LANG_C_FRAG_KIND_CODE,
    LANG_C_FRAG_KIND_OPEN,
    LANG_C_FRAG_KIND_CLOSE,
    LANG_C_FRAG_KIND_PREPROC,
    LANG_C_FRAG_KIND_COMMENT,

    LANG_C_FRAG_KIND_COUNT,
} lang_c_frag_kind;

typedef struct lang_c_frag lang_c_frag;
struct lang_c_frag {
    i64 debug_id; // DEBUG

    i64 pos;
    i64 len;
    lang_c_frag_kind kind;

    lang_c_frag *next;
    lang_c_frag *prev;

    struct lang_c_scope *scope;
};

typedef struct lang_c_frag_list lang_c_frag_list;
struct lang_c_frag_list {
    lang_c_frag *first;
    lang_c_frag *last;
    i64 count;
};

static i64 lang_c_global_debug_id_counter;

typedef struct lang_c_frag_tree lang_c_frag_tree;
struct lang_c_frag_tree {
    // TODO(rune): Make it an actual tree.
    list(lang_c_frag) frags;
};

////////////////////////////////////////////////////////////////
// rune: Scope tree

typedef struct lang_c_scope lang_c_scope;
struct lang_c_scope {
    // NOTE(rune): Links to open and close brace. These also store the scope's byte offset in the document.
    lang_c_frag *begin;
    lang_c_frag *end;

    // NOTE(rune): Hierarchy links.
    lang_c_scope *next;
    lang_c_scope *prev;
    lang_c_scope *parent;
    list(lang_c_scope) children;

    i64 _unused;
};

typedef struct lang_c_scope_tree lang_c_scope_tree;
struct lang_c_scope_tree {
    list(lang_c_scope) roots;
};

////////////////////////////////////////////////////////////////
// rune: C indexer

typedef struct lang_c_indexer lang_c_indexer;
struct lang_c_indexer {
    lang_c_frag_tree frag_tree;
    lang_c_scope_tree scope_tree;
    edith_gapbuffer *src_buffer;
    arena *arena;

    bool init;
};

static edith_gapbuffer *lang_c_global_src_buffer; // TODO(rune): Remove global.

////////////////////////////////////////////////////////////////
// rune: Fragments

typedef struct lang_c_frag_kind_info lang_c_frag_kind_info;
struct lang_c_frag_kind_info {
    str name;
    str ansi_color;
};

static readonly lang_c_frag_kind_info lang_c_frag_kind_infos[] = {
    [LANG_C_FRAG_KIND_NONE] =    { STR("LANG_C_FRAG_KIND_NONE"),    STR(ANSI_FG_RED),     },
    [LANG_C_FRAG_KIND_CODE]    = { STR("LANG_C_FRAG_KIND_CODE"),    STR(ANSI_FG_DEFAULT), },
    [LANG_C_FRAG_KIND_OPEN]    = { STR("LANG_C_FRAG_KIND_OPEN"),    STR(ANSI_FG_MAGENTA), },
    [LANG_C_FRAG_KIND_CLOSE]   = { STR("LANG_C_FRAG_KIND_CLOSE"),   STR(ANSI_FG_MAGENTA), },
    [LANG_C_FRAG_KIND_PREPROC] = { STR("LANG_C_FRAG_KIND_PREPROC"), STR(ANSI_FG_YELLOW),  },
    [LANG_C_FRAG_KIND_COMMENT] = { STR("LANG_C_FRAG_KIND_COMMENT"), STR(ANSI_FG_GREEN),   },
};

static lang_c_frag_kind_info *lang_c_frag_kind_info_get(lang_c_frag_kind a);
static lang_c_frag *          lang_c_frag_from_pos(lang_c_frag_tree *tree, i64 pos);
static i64                    lang_c_pos_from_frag(lang_c_frag_tree *tree, lang_c_frag *frag);
static bool                   lang_c_next_frag(edith_gapbuffer *gb, i64 *pos, i64 *out_frag_pos, i64 *out_frag_len, lang_c_frag_kind *out_frag_kind);

////////////////////////////////////////////////////////////////
// rune: Scope tree debug

static void lang_c_debug_check_scope_tree_(lang_c_scope *parent);
static void lang_c_debug_check_scope_tree(lang_c_scope_tree *tree);
static void lang_c_debug_check_frag_tree(lang_c_frag_tree *tree);
static void lang_c_debug_print_frag_tree(lang_c_frag_tree *tree);

////////////////////////////////////////////////////////////////
// rune: Scope tree implementation

static lang_c_scope *   lang_c_alloc_scope(lang_c_indexer *cix);
static lang_c_frag *    lang_c_add_frag(lang_c_indexer *cix, i64 pos, i64 len, lang_c_frag_kind kind);
static void             lang_c_remove_frag(lang_c_frag_tree *tree, lang_c_frag *frag);
static void             lang_c_scope_tree_fixup(lang_c_indexer *cix, lang_c_frag *from);
static void             lang_c_scope_tree_rebuild(lang_c_indexer *cix);
static void             lang_c_rebuild(lang_c_indexer *cix, edith_gapbuffer *src_buffer);

////////////////////////////////////////////////////////////////
// rune: Syntax ranges

// TODO(rune): This could be part a furture language support abstraction-layer,
// so we can support multiple languages.

typedef enum edith_syntax_kind {
    EDITH_SYNTAX_KIND_NONE,
    EDITH_SYNTAX_KIND_KEYWORD,
    EDITH_SYNTAX_KIND_LITERAL,
    EDITH_SYNTAX_KIND_COMMENT,
    EDITH_SYNTAX_KIND_OPERATOR,
    EDITH_SYNTAX_KIND_PUNCT,
    EDITH_SYNTAX_KIND_PREPROC,

    EDITH_SYNTAX_KIND_COUNT,
} edith_syntax_kind;

typedef struct edith_syntax_range edith_syntax_range;
struct edith_syntax_range {
    i64_range range;
    edith_syntax_kind kind;
    edith_syntax_range *next;
};

typedef struct edith_syntax_range_list edith_syntax_range_list;
struct edith_syntax_range_list {
    edith_syntax_range *first;
    edith_syntax_range *last;
};

static edith_syntax_kind lang_c_token_kind_to_syntax_kind_table[] = {
    [LANG_C_TOKEN_KIND_NONE] = EDITH_SYNTAX_KIND_NONE,

    // rune: Punctuation
    [LANG_C_TOKEN_KIND_PAREN_OPEN]    = EDITH_SYNTAX_KIND_PUNCT,
    [LANG_C_TOKEN_KIND_PAREN_CLOSE]   = EDITH_SYNTAX_KIND_PUNCT,
    [LANG_C_TOKEN_KIND_BRACE_OPEN]    = EDITH_SYNTAX_KIND_PUNCT,
    [LANG_C_TOKEN_KIND_BRACE_CLOSE]   = EDITH_SYNTAX_KIND_PUNCT,
    [LANG_C_TOKEN_KIND_BRACKET_OPEN]  = EDITH_SYNTAX_KIND_PUNCT,
    [LANG_C_TOKEN_KIND_BRACKET_CLOSE] = EDITH_SYNTAX_KIND_PUNCT,
    [LANG_C_TOKEN_KIND_SEMICOLON]     = EDITH_SYNTAX_KIND_PUNCT,

    // rune: Single character operators
    [LANG_C_TOKEN_KIND_DOT]         = EDITH_SYNTAX_KIND_OPERATOR,
    [LANG_C_TOKEN_KIND_COMMA]       = EDITH_SYNTAX_KIND_OPERATOR,
    [LANG_C_TOKEN_KIND_ADD]         = EDITH_SYNTAX_KIND_OPERATOR,
    [LANG_C_TOKEN_KIND_SUB]         = EDITH_SYNTAX_KIND_OPERATOR,
    [LANG_C_TOKEN_KIND_MUL]         = EDITH_SYNTAX_KIND_OPERATOR,
    [LANG_C_TOKEN_KIND_DIV]         = EDITH_SYNTAX_KIND_OPERATOR,
    [LANG_C_TOKEN_KIND_MOD]         = EDITH_SYNTAX_KIND_OPERATOR,
    [LANG_C_TOKEN_KIND_NOT]         = EDITH_SYNTAX_KIND_OPERATOR,
    [LANG_C_TOKEN_KIND_ASSIGN]      = EDITH_SYNTAX_KIND_OPERATOR,
    [LANG_C_TOKEN_KIND_LESS]        = EDITH_SYNTAX_KIND_OPERATOR,
    [LANG_C_TOKEN_KIND_GREATER]     = EDITH_SYNTAX_KIND_OPERATOR,
    [LANG_C_TOKEN_KIND_BITWISE_AND] = EDITH_SYNTAX_KIND_OPERATOR,
    [LANG_C_TOKEN_KIND_BITWISE_OR]  = EDITH_SYNTAX_KIND_OPERATOR,
    [LANG_C_TOKEN_KIND_BITWISE_NOT] = EDITH_SYNTAX_KIND_OPERATOR,
    [LANG_C_TOKEN_KIND_BITWISE_XOR] = EDITH_SYNTAX_KIND_OPERATOR,
    [LANG_C_TOKEN_KIND_QUESTION]    = EDITH_SYNTAX_KIND_OPERATOR,
    [LANG_C_TOKEN_KIND_COLON]       = EDITH_SYNTAX_KIND_OPERATOR,

    // rune: Multi character operators
    [LANG_C_TOKEN_KIND_BITSHIFT_LEFT]    = EDITH_SYNTAX_KIND_OPERATOR, // <<
    [LANG_C_TOKEN_KIND_BITSHIFT_RIGHT]   = EDITH_SYNTAX_KIND_OPERATOR, // >>
    [LANG_C_TOKEN_KIND_EQUAL]            = EDITH_SYNTAX_KIND_OPERATOR, // ==
    [LANG_C_TOKEN_KIND_NOT_EQUAL]        = EDITH_SYNTAX_KIND_OPERATOR, // !=
    [LANG_C_TOKEN_KIND_AND]              = EDITH_SYNTAX_KIND_OPERATOR, // &&
    [LANG_C_TOKEN_KIND_OR]               = EDITH_SYNTAX_KIND_OPERATOR, // ||
    [LANG_C_TOKEN_KIND_INC]              = EDITH_SYNTAX_KIND_OPERATOR, // ++
    [LANG_C_TOKEN_KIND_DEC]              = EDITH_SYNTAX_KIND_OPERATOR, // --
    [LANG_C_TOKEN_KIND_ARROW]            = EDITH_SYNTAX_KIND_OPERATOR, // ->
    [LANG_C_TOKEN_KIND_LESS_OR_EQUAL]    = EDITH_SYNTAX_KIND_OPERATOR, // <=
    [LANG_C_TOKEN_KIND_GREATER_OR_EQUAL] = EDITH_SYNTAX_KIND_OPERATOR, // >=

    // rune: Multi character assignment operators
    [LANG_C_TOKEN_KIND_ADD_ASSIGN]            = EDITH_SYNTAX_KIND_OPERATOR, // +=
    [LANG_C_TOKEN_KIND_SUB_ASSIGN]            = EDITH_SYNTAX_KIND_OPERATOR, // -=
    [LANG_C_TOKEN_KIND_MUL_ASSIGN]            = EDITH_SYNTAX_KIND_OPERATOR, // *=
    [LANG_C_TOKEN_KIND_DIV_ASSIGN]            = EDITH_SYNTAX_KIND_OPERATOR, // /=
    [LANG_C_TOKEN_KIND_MOD_ASSIGN]            = EDITH_SYNTAX_KIND_OPERATOR, // %=
    [LANG_C_TOKEN_KIND_BITWISE_AND_ASSIGN]    = EDITH_SYNTAX_KIND_OPERATOR, // &=
    [LANG_C_TOKEN_KIND_BITWISE_OR_ASSIGN]     = EDITH_SYNTAX_KIND_OPERATOR, // |=
    [LANG_C_TOKEN_KIND_BITWISE_XOR_ASSIGN]    = EDITH_SYNTAX_KIND_OPERATOR, // ^=
    [LANG_C_TOKEN_KIND_BITSHIFT_LEFT_ASSIGN]  = EDITH_SYNTAX_KIND_OPERATOR, // <<=
    [LANG_C_TOKEN_KIND_BITSHIFT_RIGHT_ASSIGN] = EDITH_SYNTAX_KIND_OPERATOR, // >>=
    [LANG_C_TOKEN_KIND_AND_ASSIGN]            = EDITH_SYNTAX_KIND_OPERATOR, // &&=
    [LANG_C_TOKEN_KIND_OR_ASSIGN]             = EDITH_SYNTAX_KIND_OPERATOR, // ||=

    // rune: Genereal
    [LANG_C_TOKEN_KIND_IDENT]                 = EDITH_SYNTAX_KIND_NONE,
    [LANG_C_TOKEN_KIND_STRING]                = EDITH_SYNTAX_KIND_LITERAL,
    [LANG_C_TOKEN_KIND_NUMBER]                = EDITH_SYNTAX_KIND_LITERAL,

    // rune: Keywords
    [LANG_C_TOKEN_KIND_AUTO]          = EDITH_SYNTAX_KIND_KEYWORD,
    [LANG_C_TOKEN_KIND_BOOL]          = EDITH_SYNTAX_KIND_KEYWORD,
    [LANG_C_TOKEN_KIND_BREAK]         = EDITH_SYNTAX_KIND_KEYWORD,
    [LANG_C_TOKEN_KIND_CASE]          = EDITH_SYNTAX_KIND_KEYWORD,
    [LANG_C_TOKEN_KIND_CHAR]          = EDITH_SYNTAX_KIND_KEYWORD,
    [LANG_C_TOKEN_KIND_CONST]         = EDITH_SYNTAX_KIND_KEYWORD,
    [LANG_C_TOKEN_KIND_CONSTEXPR]     = EDITH_SYNTAX_KIND_KEYWORD,
    [LANG_C_TOKEN_KIND_CONTINUE]      = EDITH_SYNTAX_KIND_KEYWORD,
    [LANG_C_TOKEN_KIND_DEFAULT]       = EDITH_SYNTAX_KIND_KEYWORD,
    [LANG_C_TOKEN_KIND_DO]            = EDITH_SYNTAX_KIND_KEYWORD,
    [LANG_C_TOKEN_KIND_DOUBLE]        = EDITH_SYNTAX_KIND_KEYWORD,
    [LANG_C_TOKEN_KIND_ELSE]          = EDITH_SYNTAX_KIND_KEYWORD,
    [LANG_C_TOKEN_KIND_ENUM]          = EDITH_SYNTAX_KIND_KEYWORD,
    [LANG_C_TOKEN_KIND_EXTERN]        = EDITH_SYNTAX_KIND_KEYWORD,
    [LANG_C_TOKEN_KIND_FALSE]         = EDITH_SYNTAX_KIND_KEYWORD,
    [LANG_C_TOKEN_KIND_FLOAT]         = EDITH_SYNTAX_KIND_KEYWORD,
    [LANG_C_TOKEN_KIND_FOR]           = EDITH_SYNTAX_KIND_KEYWORD,
    [LANG_C_TOKEN_KIND_GOTO]          = EDITH_SYNTAX_KIND_KEYWORD,
    [LANG_C_TOKEN_KIND_IF]            = EDITH_SYNTAX_KIND_KEYWORD,
    [LANG_C_TOKEN_KIND_INLINE]        = EDITH_SYNTAX_KIND_KEYWORD,
    [LANG_C_TOKEN_KIND_INT]           = EDITH_SYNTAX_KIND_KEYWORD,
    [LANG_C_TOKEN_KIND_LONG]          = EDITH_SYNTAX_KIND_KEYWORD,
    [LANG_C_TOKEN_KIND_NULLPTR]       = EDITH_SYNTAX_KIND_KEYWORD,
    [LANG_C_TOKEN_KIND_REGISTER]      = EDITH_SYNTAX_KIND_KEYWORD,
    [LANG_C_TOKEN_KIND_RESTRICT]      = EDITH_SYNTAX_KIND_KEYWORD,
    [LANG_C_TOKEN_KIND_RETURN]        = EDITH_SYNTAX_KIND_KEYWORD,
    [LANG_C_TOKEN_KIND_SHORT]         = EDITH_SYNTAX_KIND_KEYWORD,
    [LANG_C_TOKEN_KIND_SIGNED]        = EDITH_SYNTAX_KIND_KEYWORD,
    [LANG_C_TOKEN_KIND_SIZEOF]        = EDITH_SYNTAX_KIND_KEYWORD,
    [LANG_C_TOKEN_KIND_STATIC]        = EDITH_SYNTAX_KIND_KEYWORD,
    [LANG_C_TOKEN_KIND_STATIC_ASSERT] = EDITH_SYNTAX_KIND_KEYWORD,
    [LANG_C_TOKEN_KIND_STRUCT]        = EDITH_SYNTAX_KIND_KEYWORD,
    [LANG_C_TOKEN_KIND_SWITCH]        = EDITH_SYNTAX_KIND_KEYWORD,
    [LANG_C_TOKEN_KIND_THREAD_LOCAL]  = EDITH_SYNTAX_KIND_KEYWORD,
    [LANG_C_TOKEN_KIND_TRUE]          = EDITH_SYNTAX_KIND_KEYWORD,
    [LANG_C_TOKEN_KIND_TYPEDEF]       = EDITH_SYNTAX_KIND_KEYWORD,
    [LANG_C_TOKEN_KIND_TYPEOF]        = EDITH_SYNTAX_KIND_KEYWORD,
    [LANG_C_TOKEN_KIND_TYPEOF_UNQUAL] = EDITH_SYNTAX_KIND_KEYWORD,
    [LANG_C_TOKEN_KIND_UNION]         = EDITH_SYNTAX_KIND_KEYWORD,
    [LANG_C_TOKEN_KIND_UNSIGNED]      = EDITH_SYNTAX_KIND_KEYWORD,
    [LANG_C_TOKEN_KIND_VOID]          = EDITH_SYNTAX_KIND_KEYWORD,
    [LANG_C_TOKEN_KIND_VOLATILE]      = EDITH_SYNTAX_KIND_KEYWORD,
    [LANG_C_TOKEN_KIND_WHILE]         = EDITH_SYNTAX_KIND_KEYWORD,
    [LANG_C_TOKEN_KIND_ALIGNAS]       = EDITH_SYNTAX_KIND_KEYWORD,
    [LANG_C_TOKEN_KIND_ALIGNOF]       = EDITH_SYNTAX_KIND_KEYWORD,
    [LANG_C_TOKEN_KIND_ATOMIC]        = EDITH_SYNTAX_KIND_KEYWORD,
    [LANG_C_TOKEN_KIND_BITINT]        = EDITH_SYNTAX_KIND_KEYWORD,
    [LANG_C_TOKEN_KIND_COMPLEX]       = EDITH_SYNTAX_KIND_KEYWORD,
    [LANG_C_TOKEN_KIND_DECIMAL128]    = EDITH_SYNTAX_KIND_KEYWORD,
    [LANG_C_TOKEN_KIND_DECIMAL32]     = EDITH_SYNTAX_KIND_KEYWORD,
    [LANG_C_TOKEN_KIND_DECIMAL64]     = EDITH_SYNTAX_KIND_KEYWORD,
    [LANG_C_TOKEN_KIND_GENERIC]       = EDITH_SYNTAX_KIND_KEYWORD,
    [LANG_C_TOKEN_KIND_IMAGINARY]     = EDITH_SYNTAX_KIND_KEYWORD,
    [LANG_C_TOKEN_KIND_NORETURN]      = EDITH_SYNTAX_KIND_KEYWORD,

    [LANG_C_TOKEN_KIND_PREPROC]       = EDITH_SYNTAX_KIND_PREPROC,
    [LANG_C_TOKEN_KIND_COMMENT]       = EDITH_SYNTAX_KIND_COMMENT,
};

static edith_syntax_range_list  lang_c_get_syntax_ranges(lang_c_indexer *cix, edith_gapbuffer *src_buffer, i64_range src_range, arena *arena);
static void                     lang_c_apply_edits(lang_c_indexer *cix, edith_gapbuffer *src_buffer, edith_edit_array edits);
static edith_gapbuffer          lang_c_immutable_gapbuffer(str s);
static void                     lang_c_free(lang_c_indexer *cix);
