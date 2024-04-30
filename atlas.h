typedef struct atlas_slot atlas_slot;
struct atlas_slot {
    urect rect;
    u32 gen;
    u32 timestamp;
    atlas_slot *next;
};

typedef struct atlas_slot_key atlas_slot_key;
struct atlas_slot_key {
    u32 idx;
    u32 gen;
};

typedef struct atlas_shelf atlas_shelf;
struct atlas_shelf {
    u32 base_y;
    u32 used_x;
    u32 dim_y;
    atlas_shelf *next, *prev;
    list(atlas_slot) slots;
};

typedef struct atlas atlas;
struct atlas {
    u8 *pixels;
    uvec2 dim;

    list(atlas_shelf) shelves;

    list(atlas_shelf) shelf_freelist;
    list(atlas_slot)  slot_freelist;

    atlas_slot  *slots;
    u32          slot_count;

    bool dirty;
    u32 curr_timestamp;

    arena *arena;
};

static void             atlas_init(atlas *atlas, uvec2 dim, u32 max_slots, arena *arena);
static atlas_slot_key   atlas_new_slot(atlas *atlas, uvec2 dim);
static bool             atlas_get_slot(atlas *atlas, atlas_slot_key key, urect *rect);
static rect             atlas_get_uv(atlas *atlas, urect r);
static void             atlas_next_timestamp(atlas *atlas);
