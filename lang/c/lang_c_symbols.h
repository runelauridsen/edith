////////////////////////////////////////////////////////////////
// rune: Symbols

typedef enum lang_c_symbol_kind {
    LANG_C_SYMBOL_KIND_NONE,
    LANG_C_SYMBOL_KIND_TYPE,
    LANG_C_SYMBOL_KIND_FUNC,
    LANG_C_SYMBOL_KIND_VAR,

    LANG_C_SYMBOL_KIND_COUNT,
} lang_c_symbol_kind;

typedef struct lang_c_symbol lang_c_symbol;

typedef struct lang_c_symbol_list lang_c_symbol_list;
struct lang_c_symbol_list {
    lang_c_symbol *first;
    lang_c_symbol *last;
    i64 count;
};

typedef enum lang_c_type_kind {
    LANG_C_TYPE_KIND_NONE,
    LANG_C_TYPE_KIND_BASIC,
    LANG_C_TYPE_KIND_POINTER,
    LANG_C_TYPE_KIND_ARRAY,
    LANG_C_TYPE_KIND_FUNC,

    LANG_C_TYPE_KIND_COUNT,
} lang_c_type_kind;

typedef struct lang_c_type lang_c_type;
struct lang_c_type {
    lang_c_type_kind kind;
    str text;
    lang_c_symbol_list args;
    lang_c_type *next;
};

typedef struct lang_c_type_list lang_c_type_list;
struct lang_c_type_list {
    lang_c_type *first;
    lang_c_type *last;
};

typedef struct lang_c_symbol lang_c_symbol;
struct lang_c_symbol {
    str name;
    lang_c_symbol_kind kind;
    lang_c_symbol *next;
    lang_c_type_list type;
};

static arena lang_c_symbol_arena = { 0 }; // TODO(rune): Remove global.

////////////////////////////////////////////////////////////////
// rune: Symbol list

static lang_c_symbol *lang_c_symbol_list_push(lang_c_symbol_list *symbols, str name, lang_c_symbol_kind kind);

////////////////////////////////////////////////////////////////
// rune: String conversion

static str_list lang_c_str_list_from_type(lang_c_type *type, arena *arena);
static str_list lang_c_str_list_from_type_list(lang_c_type_list types, arena *arena);
static str_list lang_c_str_list_from_symbol(lang_c_symbol *symbol, arena *arena);
static str_list lang_c_str_list_from_symbol_list(lang_c_symbol_list symbols, arena *arena);

static str lang_c_str_from_type(lang_c_type *type, arena *arena);
static str lang_c_str_from_type_list(lang_c_type_list types, arena *arena);
static str lang_c_str_from_type_kind(lang_c_type_kind type_kind);
static str lang_c_str_from_symbol(lang_c_symbol *symbol, arena *arena);
static str lang_c_str_from_symbol_list(lang_c_symbol_list symbols, arena *arena);
static str lang_c_str_from_symbol_kind(lang_c_symbol_kind symbol_kind);

////////////////////////////////////////////////////////////////
// rune: Symbol kind

static lang_c_symbol_kind lang_c_symbol_kind_from_type(lang_c_type *type);

static lang_c_symbol_list lang_c_symbols_from_str(str input, arena *arena);
static lang_c_symbol_list lang_c_symbols_from_range(i64_range range, edith_gapbuffer *src_buffer);
