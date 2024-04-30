typedef struct glyph glyph;
struct glyph {
    f32 advance;
    vec2 bearing;

    // NOTE(rune): Index into atlas.node_storage.
    u32 atlas_idx;

    u32 lru_next;
    u32 lru_prev;
};

typedef struct font_metrics font_metrics;
struct font_metrics {
    f32 ascent;
    f32 descent;
    f32 linegap;
    f32 lineheight;
};

typedef struct font_backend_stb_slot font_backend_stb_slot;
struct font_backend_stb_slot {
    stbtt_fontinfo stb_info;
    f32 advance_cache_ascii[128];

    f32 ascent;
    f32 descent;
    f32 linegap;
    f32 lineheight;

    // @Todo Non-ascii cache.
};

typedef struct font_backend_stb font_backend_stb;
struct font_backend_stb {
    font_backend_stb_slot slots[64];
    u16 slot_count;

    glyph glyph_storage[8192];
    i64 glyph_storage_count;
    map glyph_map;

    atlas *atlas;
};
