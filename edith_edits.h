////////////////////////////////////////////////////////////////
// rune: Edits

typedef enum edith_edit_kind {
    EDITH_EDIT_KIND_NONE,
    EDITH_EDIT_KIND_INSERT,
    EDITH_EDIT_KIND_DELETE,

    EDITH_EDIT_KIND_COUNT,
} edith_edit_kind;

typedef struct edith_edit edith_edit;
struct edith_edit {
    i64 pos;
    str data;
};

typedef struct edith_edit_node edith_edit_node;
struct edith_edit_node {
    edith_edit edit;
    edith_edit_node *next;
};

typedef struct edith_edit_list edith_edit_list;
struct edith_edit_list {
    edith_edit_node *first;
    edith_edit_node *last;
    i64 count;
};

typedef struct edith_edit_array edith_edit_array;
struct edith_edit_array {
    edith_edit *v;
    i64 count;
    edith_edit_kind kind;
};

static edith_edit *     edith_edit_list_push(edith_edit_list *list, arena *arena);
static edith_edit *     edith_edit_array_pop(edith_edit_array *edits);
static edith_edit_array edith_edit_array_reserve(arena *arena, i64 count, edith_edit_kind kind);
static edith_edit_array edith_edit_array_copy(arena *arena, edith_edit_array src, bool deep);
static edith_edit_array edith_edit_array_from_list(edith_edit_list list, edith_edit_kind kind, arena *arena);
static edith_edit_array edith_edit_array_invert(edith_edit_array src, arena *arena);

static str              edith_str_from_edit_kind(edith_edit_kind kind);
static str              edith_ansi_color_from_edit_kind(edith_edit_kind kind);

////////////////////////////////////////////////////////////////
// rune: Edit batch

#define EDITH_DEBUG_PRINT_EDIT_BATCH_CONSOLIDATE 0

typedef struct edith_edit_batch_node edith_edit_batch_node;
struct edith_edit_batch_node {
    edith_edit_array edits;
    edith_edit_batch_node *next;
    edith_edit_batch_node *prev;
};

typedef struct edith_edit_batch edith_edit_batch;
struct edith_edit_batch {
    edith_edit_batch_node *first;
    edith_edit_batch_node *last;
};

static void             edith_edit_batch_push(edith_edit_batch *b, edith_edit_array a, arena *arena);
static edith_edit_batch edith_edit_batch_consolidate(edith_edit_batch batch, arena *arena);

////////////////////////////////////////////////////////////////
// rune: Gapbuffer for i64

typedef struct edith_gapbuffer64 edith_gapbuffer64;
struct edith_gapbuffer64 {
    i64 *buf;
    i64 buflen;
    i64 gap_min;
    i64 gap_max;
};

static void edith_gb64_create(edith_gapbuffer64 *gb, str name);
static void edith_gb64_destroy(edith_gapbuffer64 *gb);
static void edith_gb64_reset(edith_gapbuffer64 *gb);

static i64  edith_gb64_count(edith_gapbuffer64 *gb);
static void edith_gb64_reserve_gap(edith_gapbuffer64 *gb);
static void edith_gb64_put(edith_gapbuffer64 *gb, i64 idx, i64 val);
static void edith_gb64_delete(edith_gapbuffer64 *gb, i64 idx, i64 count);
static i64  edith_gb64_binary_search_bot(edith_gapbuffer64 *gb, i64 needle);
static i64  edith_gb64_binary_search_top(edith_gapbuffer64 *gb, i64 needle);

////////////////////////////////////////////////////////////////
// rune: Shifting

static void edith_shift_after_edits_flat(i64 *flat, i64 num, edith_edit_array edits);
static void edith_shift_after_edits_struct(void *structs, i64 num, i64 stride, i64 member_offset, edith_edit_array edits);
static void edith_shift_after_edits_gb64(edith_gapbuffer64 *gb, edith_edit_array edits);
