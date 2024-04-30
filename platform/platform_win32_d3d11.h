typedef struct r_rect_inst r_rect_inst;
struct r_rect_inst {
    rect dst_rect;
    rect tex_rect;
    u32  color[4];
    f32  roundness;
    f32  softness;
    f32  tex_weight;
};

typedef struct r_rect_inst_array r_rect_inst_array;
struct r_rect_inst_array {
    r_rect_inst *v;
    i64 count;
};

typedef struct r_rect_inst_span r_rect_inst_span;
struct r_rect_inst_span {
    r_rect_inst *v;
    i64 count;
};

typedef struct r_d3d11_compiled_shaders r_d3d11_compiled_shaders;
struct r_d3d11_compiled_shaders {
    ID3D11VertexShader      *vs;
    ID3D11PixelShader       *ps;
    ID3D11InputLayout       *il;
};

typedef struct r_state r_state;
struct r_state {
    IDXGISwapChain              *swap_chain;
    ID3D11Device                *device;
    ID3D11DeviceContext         *device_context;
    ID3D11RenderTargetView      *render_target_view;
    ivec2                        render_target_dim;

    ID3D11Texture2D             *tex;
    ID3D11ShaderResourceView    *tex_view;
    ID3D11SamplerState          *linear_sampler;

    ID3D11RasterizerState       *rasterizer;

    ID3D11BlendState            *blend;

    u32                          rect_buffer_size;
    ID3D11Buffer                *rect_buffer;
    ID3D11Buffer                *rect_constant_buffer;
    r_d3d11_compiled_shaders     rect_shaders;

    arena *arena;
};

// NOTE(rune): GPU/CPU shared structure
//  Must match grid_constant_buffer in render_d3d11_grid.hlsl.
//  Must be padded to 16 byte multiple.
typedef struct r_grid_constant_buffer r_grid_constant_buffer;
struct r_grid_constant_buffer {
    ivec2 cell_dim;
    ivec2 cell_count;
    ivec2 margin_top_left;
    ivec2 screen_dim;
};

// NOTE(rune): GPU/CPU shared structure
//  Must match rect_constant_buffer in render_draw_d3d1_rect.hlsl.
//  Must be padded to 16 byte multiple.
typedef struct r_rect_constant_buffer r_rect_constant_buffer;
struct r_rect_constant_buffer {
    vec2 margin_top_left;
    vec2 screen_dim;
    u32 tex_channels;

    u8 _unused[8];
};

typedef enum r_tex_format {
    R_TEX_FORMAT_R8,
    R_TEX_FORMAT_R8G8B8A8
} r_tex_format;

typedef struct r_d3d11_tex r_d3d11_tex;
struct r_d3d11_tex {
    ID3D11Texture2D             *obj;
    ID3D11ShaderResourceView    *view;
};

typedef struct r_d3d11_render_data r_d3d11_render_data;
struct r_d3d11_render_data {
    struct yo_node *root;
    struct atlas *atlas;
    struct font_backend *font_backend;
    r_d3d11_tex atlas_tex;
};

typedef struct r_d3d11_rect_params r_d3d11_rect_params;
struct r_d3d11_rect_params {
    r_d3d11_tex tex;
    rect clip;
    f32 transparency;
};

typedef struct r_d3d11_rect_drawcall r_d3d11_rect_drawcall;
struct r_d3d11_rect_drawcall {
    r_d3d11_rect_params params;

    r_rect_inst_array insts;

    r_d3d11_rect_drawcall *next;
};

typedef struct r_d3d11_rect_drawcall_list r_d3d11_rect_drawcall_list;
struct r_d3d11_rect_drawcall_list {
    r_d3d11_rect_drawcall *first;
    r_d3d11_rect_drawcall *last;
    i64 count;
};

typedef struct r_d3d11_rect_chunk r_d3d11_rect_chunk;
struct r_d3d11_rect_chunk {
    r_rect_inst *v;
    i64 count;
    i64 cap;

    r_d3d11_rect_chunk *next;
};

typedef struct r_d3d11_rect_chunk_list r_d3d11_rect_chunk_list;
struct r_d3d11_rect_chunk_list {
    r_d3d11_rect_chunk *first;
    r_d3d11_rect_chunk *last;
    i64 count;
    i64 total_inst_count;
};

typedef struct r_d3d11_rect_batch r_d3d11_rect_batch;
struct r_d3d11_rect_batch {
    r_d3d11_rect_params params;
    r_d3d11_rect_chunk_list chunks;
    r_d3d11_rect_batch *next;
};

typedef struct r_d3d11_rect_batch_list r_d3d11_rect_batch_list;
struct r_d3d11_rect_batch_list {
    r_d3d11_rect_batch *first;
    r_d3d11_rect_batch *last;
    i64 count;
};

////////////////////////////////////////////////////////////////
// rune: Main

extern_c void r_startup(r_state *state, HWND hwnd);
extern_c void r_shutdown(r_state *state);
extern_c void r_render(r_state *state, ivec2 target_dim, bool need_redraw, r_d3d11_render_data *data);

////////////////////////////////////////////////////////////////
// rune: Textures

extern_c r_d3d11_tex r_tex_create(r_state *state, uvec2 dim, r_tex_format format);
extern_c void        r_tex_destroy(r_state *state, r_d3d11_tex tex);
extern_c void        r_tex_update(r_state *state, r_d3d11_tex tex, void *data, i64 data_size);

////////////////////////////////////////////////////////////////
// rune: Internal

extern_c r_d3d11_rect_batch_list      r_d3d11_rect_batches_from_nodes(yo_node *root, arena *arena, r_d3d11_render_data *data);
extern_c r_d3d11_rect_drawcall_list   r_d3d11_rect_drawcalls_from_batches(r_d3d11_rect_batch_list batches, arena *arena, r_d3d11_render_data *data);
