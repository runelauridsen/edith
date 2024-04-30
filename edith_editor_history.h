////////////////////////////////////////////////////////////////
// rune: Edit history

typedef enum edith_unredo {
    EDITH_UNREDO_UNDO,
    EDITH_UNREDO_REDO,
} edith_unredo;

typedef struct edith_saved_cursor_array edith_saved_cursor_array;
struct edith_saved_cursor_array {
    struct edith_saved_cursor *v;
    i64 count;
    struct edith_textview *tv; // TODO(rune): Needs to be some kind of handle, since editor structs are reused
};

typedef struct edith_edit_history_record edith_edit_history_record;
struct edith_edit_history_record {
    arena_mark arena_mark;
    i64 debug_id;

    edith_edit_batch batch;

    edith_saved_cursor_array saved_cursors[2];

    edith_edit_history_record *next;
    edith_edit_history_record *prev;
};

typedef struct edith_edit_history edith_edit_history;
struct edith_edit_history {
    arena *arena;

    edith_edit_history_record first_record;
    edith_edit_history_record *at;

    i64 debug_id_counter;
};

static void                       edith_edit_history_begin_record(edith_edit_history *history);
static void                       edith_edit_history_add_to_record(edith_edit_history *history, edith_edit_array new_edits);
static edith_edit_history_record *edith_edit_history_step(edith_edit_history *h, edith_unredo unredo);

static edith_saved_cursor_array edith_saved_cursor_array_reserve(i64 count, arena *arena);