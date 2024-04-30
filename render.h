typedef struct r_gridcell r_gridcell;
struct r_gridcell {
    u32 glyph_idx; // NOTE(rune): Packed u16 coordinates of glyph in glyph atlas texutre.
    u32 foreground;
    u32 background;
};

typedef struct r_grid r_grid;
struct r_grid {
    r_gridcell *cells;
    ivec2 cell_count;
    ivec2 cell_dim;
};

typedef struct r_pass_grid r_pass_grid;
struct r_pass_grid {
    rect viewport;
    r_grid grid;
};

typedef struct r_rect_instance r_rect_instance;
struct r_rect_instance {
    rect dst_rect;
    rect tex_rect;
    u32  color[4];
    f32  roundness;
    f32  softness;
    f32  tex_weight;
};

typedef struct r_tex r_tex;
struct r_tex {
    u64 data[2];
};

typedef enum r_tex_format {
    R_TEX_FORMAT_R8,
    R_TEX_FORMAT_R8G8B8A8
} r_tex_format;

typedef struct r_rect_instance_array r_rect_instance_array;
struct r_rect_instance_array {
    r_rect_instance *v;
    i64 count;
};

typedef struct r_pass_rects r_pass_rects;
struct r_pass_rects {
    rect viewport;
    r_rect_instance_array instances;
    r_tex tex;
    r_tex_format tex_format;
};

typedef enum r_pass_type {
    R_PASS_TYPE_GRID = 1,
    R_PASS_TYPE_RECT,
    R_PASS_TYPE_COUNT,
} r_pass_type;

typedef struct r_pass r_pass;
struct r_pass {
    r_pass_type type;
    union {
        r_pass_grid type_grid;
        r_pass_rects type_rects;
    };

    r_pass *next;
};

typedef list(r_pass) r_pass_list;

// NOTE(rune): Defined in rendering backend specifc header.
typedef struct r_state r_state;

////////////////////////////////////////////////////////////////
// rune: Renderer

extern_c void r_startup(r_state *state, HWND hwnd);
extern_c void r_shutdown(r_state *state);
extern_c void r_render(r_state *state, ivec2 target_dim, r_pass_list pass_list, bool need_redraw);

////////////////////////    ////////////////////////////////////////
// rune: Textures

extern_c r_tex r_create_texture(r_state *state, ivec2 dim, r_tex_format format);
extern_c void r_destroy_texture(r_state *state, r_tex tex);
extern_c void r_update_texture(r_state *state, r_tex tex, void *data, i64 data_size);
