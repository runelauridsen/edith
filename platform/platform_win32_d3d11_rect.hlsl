struct vertex_out {
    float4 position         : SV_POSITION;
    float4 color            : COLOR;
    float2 rect_center      : RECT_CENTER;
    float2 rect_half_dim    : RECT_HALF_DIM;
    float2 tex_coord        : TEX_COORD;
    float  roundness        : ROUNDNESS;
    float  softness         : SOFTNESS;
    float  tex_weight       : TEX_WEIGHT;
};

struct vertex_in {
    float4 screen_rect      : SCREEN_RECT;
    float4 tex_rect         : TEX_RECT;
    uint4  color            : COL;
    float  roundness        : ROUNDNESS;
    float  softness         : SOFTNESS;
    float  tex_weight       : TEX_WEIGHT;
};

#define GAMMA 2.2

cbuffer rect_constant_buffer : register(b0) {
    float2 margin_top_left;
    float2 screen_dim;
    uint tex_channels; // @Todo How to support single and multi channel textures in same shader properly?
};

Texture2D        obj_tex     : register(t0);
SamplerState     obj_sampler : register(s0);

// https://iquilezles.org/articles/distfunctions2d/
float rounded_rect_sdf(in float2 p, in float2 b, in float4 r) {
    r.xy = (p.x > 0.0) ? r.xy : r.zw;
    r.x  = (p.y > 0.0) ? r.x  : r.y;
    float2 q = abs(p) - b + r.x;
    return min(max(q.x, q.y), 0.0) + length(max(q, 0.0)) - r.x;
}

float4 unpack_rgba(uint u) {
    float4 ret;
    ret.r = ((u >> 0) & 0xff) / 255.0f;
    ret.g = ((u >> 8) & 0xff) / 255.0f;
    ret.b = ((u >> 16) & 0xff) / 255.0f;
    ret.a = ((u >> 24) & 0xff) / 255.0f;
    return ret;
}

// @Todo Is this the correct terminology?
float4 percieved_to_physical(float4 v) {
    float4 ret;
    ret.r = pow(abs(v.r), GAMMA);
    ret.g = pow(abs(v.g), GAMMA);
    ret.b = pow(abs(v.b), GAMMA);
    ret.a = v.a;
    return ret;
}

float4 physical_to_percieved(float4 v) {
    float4 ret;
    ret.r = pow(abs(v.r), 1 / GAMMA);
    ret.g = pow(abs(v.g), 1 / GAMMA);
    ret.b = pow(abs(v.b), 1 / GAMMA);
    ret.a = v.a;
    return ret;
}

vertex_out vs_main(uint vertex_id : SV_VERTEXID, vertex_in vertex_in) {
    vertex_out vertex_out;

    float2 screen_rect_p0 = vertex_in.screen_rect.xy;
    float2 screen_rect_p1 = vertex_in.screen_rect.zw;

    float2 tex_rect_p0 = vertex_in.tex_rect.xy;
    float2 tex_rect_p1 = vertex_in.tex_rect.zw;

    float2 screen_pos_arr[4] = {
        float2(screen_rect_p0.x, screen_rect_p0.y),
        float2(screen_rect_p1.x, screen_rect_p0.y),
        float2(screen_rect_p0.x, screen_rect_p1.y),
        float2(screen_rect_p1.x, screen_rect_p1.y),
    };

    float2 tex_coord_arr[4] = {
        float2(tex_rect_p0.x, tex_rect_p0.y),
        float2(tex_rect_p1.x, tex_rect_p0.y),
        float2(tex_rect_p0.x, tex_rect_p1.y),
        float2(tex_rect_p1.x, tex_rect_p1.y),
    };

    uint color_arr[4] = {
        vertex_in.color.x,
        vertex_in.color.y,
        vertex_in.color.z,
        vertex_in.color.w,
    };

    float2 screen_pos = screen_pos_arr[vertex_id];
    float2 tex_coord  = tex_coord_arr[vertex_id];

    float2 norm_pos = lerp(-1.0f,  1.0f, screen_pos / screen_dim);
    norm_pos.y *= -1.0f;

    float2 screen_rect_half_dim = (screen_rect_p1 - screen_rect_p0) / 2;
    float2 screen_rect_center   = (screen_rect_p0 + screen_rect_half_dim);

    vertex_out.position      = float4(norm_pos, 0.0f, 1.0f);
    vertex_out.rect_center   = screen_rect_center;
    vertex_out.rect_half_dim = screen_rect_half_dim;
    vertex_out.color         = physical_to_percieved(unpack_rgba(color_arr[vertex_id]));
    vertex_out.roundness     = vertex_in.roundness;
    vertex_out.softness      = vertex_in.softness;
    vertex_out.tex_weight    = vertex_in.tex_weight;
    vertex_out.tex_coord     = tex_coord;
    return vertex_out;
}

float4 ps_main(vertex_out from_vertex) : SV_TARGET {
    float4 ret = percieved_to_physical(from_vertex.color);

    float roundness  = from_vertex.roundness;
    float softness   = from_vertex.softness;
    float tex_weight = from_vertex.tex_weight;

    // (rune): Calculate distance to rounded screen rect.
    float d = rounded_rect_sdf(from_vertex.position.xy - from_vertex.rect_center,
                                       from_vertex.rect_half_dim,
                                       roundness);

    // (rune): Roundness.
    float round_factor = 1 - clamp(d + 0.5, 0, 1);
    ret.a *= round_factor;

    // (rune): Softness.
    float soft_factor = smoothstep(1, 0, (d + softness - 1) / (softness + 1));
    ret.a *= soft_factor;

    // (rune): Single channel texture.
    float2 uv = from_vertex.tex_coord;
    if(tex_channels == 1) {
        float sample = obj_tex.Sample(obj_sampler, uv).r;
        float weighted_sample = lerp(sample, 1, 1 - tex_weight);
        ret.a *= weighted_sample;
    }

    // (rune): Multi channel texture.
    if(tex_channels == 4) {
        float4 sample = obj_tex.Sample(obj_sampler, uv);
        float4 weighted_sample = lerp(sample, float4(1, 1, 1, 1), 1 - tex_weight);
        ret *= weighted_sample;
    }

    return ret;
}
