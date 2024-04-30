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

    ID3D11BlendState            *blend;

    i32                          max_cell_count;
    ID3D11Buffer                *cell_buffer;
    ID3D11ShaderResourceView    *cell_view;
    ID3D11Buffer                *cell_constant_buffer;

    i32                          rect_buffer_size;
    ID3D11Buffer                *rect_buffer;
    ID3D11Buffer                *rect_constant_buffer;

    r_d3d11_compiled_shaders      rect_shaders;
    r_d3d11_compiled_shaders      grid_shaders;
};

// NOTE(rune): Must be padded to 16 byte multiple as required by D3D11.
// Must match grid_constant_buffer in render_d3d11_grid.hlsl.
typedef struct r_grid_constant_buffer r_grid_constant_buffer;
struct r_grid_constant_buffer {
    ivec2 cell_dim;
    ivec2 cell_count;
    ivec2 margin_top_left;
    ivec2 screen_dim;
};

// NOTE(rune): Must be padded to 16 byte multiple as required by D3D11.
// Must match rect_constant_buffer in render_draw_d3d1_rect.hlsl.
typedef struct r_rect_constant_buffer r_rect_constant_buffer;
struct r_rect_constant_buffer {
    vec2 margin_top_left;
    vec2 screen_dim;
    i32 tex_channels;

    u8 _unused[8];
};

typedef struct r_d3d11_tex r_d3d11_tex;
struct r_d3d11_tex {
    ID3D11Texture2D             *obj;
    ID3D11ShaderResourceView    *view;
};
