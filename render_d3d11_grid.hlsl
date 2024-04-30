Texture2D obj_texture : register(t1);
SamplerState obj_sampler_state;

struct VOut {
    float4 position : SV_POSITION;
    float4 color : COLOR;
};

struct Cell {
    uint index;
    uint foreground;
    uint background;
};

StructuredBuffer<Cell> cells : register(t0);

cbuffer Grid_Constant_Buffer : register(b0) {
    uint2 cell_dim;
    uint2 cell_count;
    uint2 margin_top_left;
    uint2 screen_dim;
};

float4 float4_from_uint(uint u) {
    float4 ret;
    ret.r = ((u >> 0) & 0xff) / 255.0f;
    ret.g = ((u >> 8) & 0xff) / 255.0f;
    ret.b = ((u >> 16) & 0xff) / 255.0f;
    ret.a = ((u >> 24) & 0xff) / 255.0f;
    return ret;
}

static float2 corners[4] = {
    float2(-1.0f, -1.0f),
    float2(-1.0f, +1.0f),
    float2(+1.0f, -1.0f),
    float2(+1.0f, +1.0f),
};

VOut vs_main(uint vertex_id : SV_VERTEXID) {
    VOut ret;
    ret.position = float4(corners[vertex_id], 0, 1);
    ret.color = float4((ret.position.xy + 1) / 2, 0, 1);
    return ret;
}

float4 ps_main(float4 screen_pos : SV_POSITION, float4 color : COLOR) : SV_TARGET {
    float4 ret = 0;

    uint2 cell_index = uint2(screen_pos.xy - margin_top_left) / cell_dim;
    uint2 cell_pos = uint2(screen_pos.xy - margin_top_left) % cell_dim;

    if ((cell_index.x < cell_count.x) && (cell_index.y < cell_count.y)) {
        Cell cell = cells[cell_index.x + cell_index.y * cell_count.x];

        uint2 glyph_idx;
        glyph_idx.x = (cell.index >> 0) & 0xffff;
        glyph_idx.y = (cell.index >> 16) & 0xffff;

        uint2 glyph_pos = glyph_idx * cell_dim;

        float2 uv = float2(glyph_pos + cell_pos) / 256;

        float4 sample = obj_texture.Sample(obj_sampler_state, uv);

        // NOTE(rune): Fancy gradient:
        // ret.xy *= ((screen_pos.xy - margin_top_left) / screen_dim);

        float4 foreground = float4_from_uint(cell.foreground);
        float4 background = float4_from_uint(cell.background);
        ret = lerp(background, foreground, sample.a);
    } else {
        ret.x = 0;
    }

    return ret;

}
