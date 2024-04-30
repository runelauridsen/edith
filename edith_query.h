////////////////////////////////////////////////////////////////
// rune: Find substring

typedef struct edith_search_result edith_search_result;
struct edith_search_result {
    i64_range range;
};

typedef struct edith_search_result_node edith_search_result_node;
struct edith_search_result_node {
    edith_search_result result;
    edith_search_result_node *next;
};

typedef struct edith_search_result_list edith_search_result_list;
struct edith_search_result_list {
    edith_search_result_node *first;
    edith_search_result_node *last;
    i64 count;
};

typedef struct edith_search_result_array edith_search_result_array;
struct edith_search_result_array {
    edith_search_result *v;
    i64 count;
};

static arena *edith_global_search_result_arena;

typedef struct edith_textbuf edith_textbuf;

static edith_search_result_array edith_submit_textbuf_query(u128 key, str needle, edith_textbuf *textbuf, bool *completed);
static edith_search_result_array edith_search_results_from_key(u128 key, bool *completed, u32 timeout_ms);

// NOTE(rune): i64_range.min is first-less-or-equal and i64_range.max is last-greater-or-equal.
static i64_range edith_search_result_idx_from_pos(edith_search_result_array array, i64 pos);

////////////////////////////////////////////////////////////////
// rune: Query cache

typedef struct edith_query_slot edith_query_slot;
struct edith_query_slot {
    u128 key;
    str needle;
    edith_textbuf *textbuf; // TODO(rune): Should be some kind of handle
    arena *arena;

    edith_search_result_array results;

    edith_query_slot *next;
    edith_query_slot *prev;

    i64 accessed_frame_count;
    bool completed;
};

typedef struct edith_query_slot_list edith_query_slot_list;
struct edith_query_slot_list {
    edith_query_slot *first;
    edith_query_slot *last;
    i64 count;
};

typedef struct edith_query_cache edith_query_cache;
struct edith_query_cache {
    edith_query_slot_list requests;
    edith_query_slot_list free_slots;

    os_handle mutex;
    os_handle cond;

    arena *arena;
};

static edith_query_cache edith_global_query_cache = { 0 };

// rune: Query slot list
static void              edith_query_slot_list_push(edith_query_slot_list *list, edith_query_slot *node);
static edith_query_slot *edith_query_slot_list_pop(edith_query_slot_list *list);
static void              edith_query_slot_list_remove(edith_query_slot_list *list, edith_query_slot *node);

// rune: Query slot allocation
static edith_query_slot *edith_query_slot_alloc(edith_query_cache *cache);
static void              edith_query_slot_free(edith_query_cache *cache, edith_query_slot *slot);

// rune: Query thread
static void edith_query_thread_entry_point(void *param);

