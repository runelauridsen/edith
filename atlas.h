typedef struct atlas_node atlas_node;
struct atlas_node {
    irect rect;
    u64 key;
    i64 last_accessed;
    atlas_node *next;
};

typedef struct atlas_shelf atlas_shelf;
struct atlas_shelf {
    i32 base_y;
    i32 used_x;
    i32 dim_y;
    i64 last_accessed;
    atlas_shelf *next, *prev;
    list(atlas_node) nodes;
};

typedef struct atlas atlas;
struct atlas {
    u8 *pixels;
    i64 pixel_size;

    ivec2 dim;
    list(atlas_shelf) shelves;

    list(atlas_shelf) shelf_freelist;
    list(atlas_node)  node_freelist;

    atlas_shelf *shelf_storage;
    i64          shelf_storage_count;

    atlas_node  *node_storage;
    i64          node_storage_count;

    i32 shelf_storage_used;
    i32 node_storage_used;

    bool dirty;
    i64 current_generation;
};

static bool         atlas_create(atlas *atlas, ivec2 initial_dim, i32 max_nodes);
static void         atlas_destroy(atlas *atlas);
static void         atlas_reset(atlas *atlas);
static atlas_node * atlas_new_node(atlas *atlas, u64 key, ivec2 dim);
static atlas_node * atlas_get_node(atlas *atlas, u64 key);
static rect         atlas_get_node_uv(atlas *atlas, atlas_node *node);
