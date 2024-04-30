typedef struct glyph glyph;
struct glyph {
    f32 advance;
    vec2 bearing;

    atlas_slot_key atlas_key;
};

typedef struct font_metrics font_metrics;
struct font_metrics {
    f32 ascent;
    f32 descent;
    f32 linegap;
    f32 lineheight;
};

// NOTE(rune): Defined in backend-specific .h file.
typedef struct font_backend font_backend;

// NOTE(rune): Implemented in backend-specific .c file.
static bool         font_backend_startup(font_backend *backend, atlas *atlas);
static yo_font      font_backend_init_font(font_backend *backend, str data);
static f32          font_backend_get_advance(void *userdata, yo_face face, u32 codepoint);
static f32          font_backend_get_lineheight(void *userdata, yo_face face);
static font_metrics font_backend_get_font_metrics(font_backend *backend, yo_face face);
static glyph        font_backend_get_glyph(font_backend *backend, yo_face face, u32 codepoint);
