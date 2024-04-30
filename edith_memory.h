////////////////////////////////////////////////////////////////
// rune: Temporary storage

static thread_local arena *edith_thread_local_arena;

static arena *edith_temp(void);
#define       edith_tprint(...) edith_tprint_(argsof(__VA_ARGS__))
static str    edith_tprint_(args args);

////////////////////////////////////////////////////////////////
// rune: Heap allocation wrappers

static void *   edith_heap_alloc(i64 size, str name);
static void *   edith_heap_realloc(void *ptr, i64 size);
static void     edith_heap_free(void *ptr);

////////////////////////////////////////////////////////////////
// rune: Arena allocation wrappers

static arena *  edith_arena_create(i64 cap, arena_kind kind, str name);
static arena *  edith_arena_create_default(str name);
static void     edith_arena_destroy(arena *arena);

////////////////////////////////////////////////////////////////
// rune: Memory tracking types

typedef enum edith_mem_track_kind {
    EDITH_MEM_TRACK_KIND_NONE,
    EDITH_MEM_TRACK_KIND_HEAP,
    EDITH_MEM_TRACK_KIND_ARENA,
} edith_mem_track_kind;

typedef enum edith_mem_track_action {
    EDITH_MEM_TRACK_ACTION_NONE,
    EDITH_MEM_TRACK_ACTION_ALLOC,
    EDITH_MEM_TRACK_ACTION_REALLOC,
    EDITH_MEM_TRACK_ACTION_FREE,
} edith_mem_track_action;

typedef struct edith_mem_track_node edith_mem_track_node;
struct edith_mem_track_node {
    // rune: Identification
    edith_mem_track_kind kind;
    str name;
    void *ptr;

    // rune: Heap tracking
    i64 size;
    u64 realloc_count;

    // rune: List links
    edith_mem_track_node *next;
    edith_mem_track_node *prev;
};

typedef struct edith_mem_track_list edith_mem_track_list;
struct edith_mem_track_list {
    edith_mem_track_node *first;
    edith_mem_track_node *last;
};

static arena *              edith_global_mem_track_arena = 0;
static edith_mem_track_list edith_global_mem_track_list = { 0 };
static os_handle            edith_global_mem_track_mutex = { 0 };

static void                  edith_mem_init(void);
static edith_mem_track_node *edith_mem_track(void *param, edith_mem_track_kind kind, edith_mem_track_action action);
static void                  edith_mem_track_print(void);
