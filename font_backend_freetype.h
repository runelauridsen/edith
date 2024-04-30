typedef struct font_backend_slot font_backend_slot;
struct font_backend_slot {
    FT_Face face;
};

typedef struct font_backend font_backend;
struct font_backend {
    FT_Library library;
    atlas *atlas;

    font_backend_slot slots[64];
    u16 slot_count;

    glyph glyph_storage[8192];
    u64 glyph_storage_count;
    map glyph_map;
};
