////////////////////////////////////////////////////////////////
// rune: Text buffer types

// TODO(rune): De-OOP-ify this. No need to have seperatre files for line marks and edit history,
typedef struct edith_textbuf edith_textbuf;
struct edith_textbuf {
    i64 id;

    edith_gapbuffer gb;
    edith_edit_history history;
    edith_linemarks linemarks;

    lang_c_indexer indexer; // TODO(rune): Should not be owned by textbuf.

    struct {
        arena *arena;
        edith_edit_batch waiting_for_commit;
        bool submit_in_progress;
    } submit;
};

////////////////////////////////////////////////////////////////
// rune: Lifetime

static void edith_textbuf_create(edith_textbuf *tb);
static void edith_textbuf_destroy(edith_textbuf *tb);

////////////////////////////////////////////////////////////////
// rune: Editing

typedef enum edith_textbuf_edit_flags {
    EDITH_TEXTBUF_EDIT_FLAG_NO_HISTORY = 0x1, // TODO(rune): Explanation Comment
    EDITH_TEXTBUF_EDIT_FLAG_SUBMIT     = 0x2, // TODO(rune): Explanation Comment
} edith_textbuf_edit_flags;

static void edith_textbuf_insert(edith_textbuf *tb, i64 pos, str data, edith_textbuf_edit_flags flags);
static void edith_textbuf_apply_edit(edith_textbuf *tb, edith_edit edit, edith_edit_kind kind, edith_textbuf_edit_flags flags);
static void edith_textbuf_apply_edit_array(edith_textbuf *tb, edith_edit_array edits, edith_textbuf_edit_flags flags);
static bool edith_textbuf_commit(edith_textbuf *tb);

////////////////////////////////////////////////////////////////
// rune: Coordinates

// TODO(rune): To support unicode we need some kind of distinction between "column" and "apparent column"

typedef struct edith_textbuf_coord edith_textbuf_coord;
struct edith_textbuf_coord {
    i64 row;
    i64 col;
};

static edith_textbuf_coord edith_textbuf_coord_make(i64 x, i64 y);
static edith_textbuf_coord edith_textbuf_coord_from_pos(edith_textbuf *tb, i64 pos);
static i64                 edith_textbuf_pos_from_coord(edith_textbuf *tb, edith_textbuf_coord coord);
static i64                 edith_textbuf_row_from_pos(edith_textbuf *tb, i64 pos);
static i64                 edith_textbuf_col_from_pos(edith_textbuf *tb, i64 pos);
static i64                 edith_textbuf_pos_from_row(edith_textbuf *tb, i64 y);
static i64_range           edith_textbuf_pos_range_from_row(edith_textbuf *tb, i64 y);
static i64                 edith_textbuf_row_count(edith_textbuf *tb);

////////////////////////////////////////////////////////////////
// rune: Textbuf storage

typedef struct edith_textbuf_node edith_textbuf_node;
struct edith_textbuf_node {
    edith_textbuf v;
    edith_textbuf *next;
};

typedef struct edith_textbuf_list edith_textbuf_list;
struct edith_textbuf_list {
    edith_textbuf_node *first;
    edith_textbuf_node *last;
};

typedef struct edith_textbuf_handle edith_textbuf_handle;
struct edith_textbuf_handle {
    u32 id;
};

typedef struct edith_textbuf_store edith_textbuf_store;
struct edith_textbuf_store {
    u32 id_counter;

    edith_textbuf_list open_buffers;
    edith_textbuf_node *first_free;
};
