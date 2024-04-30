typedef struct font_backend_slot font_backend_slot;
struct font_backend_slot {
    stbtt_fontinfo stb_info;
    f32 advance_cache_ascii[128];

    f32 ascent;
    f32 descent;
    f32 linegap;
    f32 lineheight;

    // @Todo Non-ascii cache.
};

typedef struct font_backend font_backend;
struct font_backend {
    font_backend_slot slots[64];
    u16 slot_count;

    glyph glyph_storage[8192];
    u64 glyph_storage_count;
    map glyph_map;

    atlas *atlas;
};
