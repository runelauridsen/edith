#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <stdio.h>
#include <stdint.h>
#include <math.h>
#include <d3d11.h>
#include <d3dcompiler.h>

#pragma comment( lib, "kernel32" )
#pragma comment( lib, "user32" )
#pragma comment( lib, "d3d11" )
#pragma comment( lib, "dxguid" )
#pragma comment( lib, "d3dcompiler" )

#pragma warning( disable : 4101) // C4201: nonstandard extension used: nameless struct/union
#pragma warning( disable : 4505) // C4505: unreferenced function with internal linkage

#include "../base/base.h"
//#include "../ui/ui_ops.h"
#include "../atlas.h"
#include "platform_win32_d3d11.h"

#include "../thirdparty/fonts/font8x8_basic.h"

#define asserthr(hr) assert(hr >= 0)

static void r_d3d11_set_viewport(r_state *state, rect rect, f32 min_z, f32 max_z) {
    D3D11_VIEWPORT vp = {};
    vp.Width    = rect_dim_x(rect);
    vp.Height   = rect_dim_y(rect);
    vp.MinDepth = min_z;
    vp.MaxDepth = max_z;
    vp.TopLeftX = rect.p0.x;
    vp.TopLeftY = rect.p0.y;
    state->device_context->RSSetViewports(1, &vp);
    state->device_context->RSSetState(state->rasterizer);
}

static void r_d3d11_set_scissor(r_state *state, rect clip) {
    D3D11_RECT rect = { 0 };
    if (clip.x0 > clip.x1 || clip.y0 > clip.y1) {
        rect.left   = 0;
        rect.right  = 0;
        rect.top    = 0;
        rect.bottom = 0;
    } else {
        rect.left   = (LONG)clip.x0;
        rect.right  = (LONG)clip.x1;
        rect.top    = (LONG)clip.y0;
        rect.bottom = (LONG)clip.y1;
    }

    state->device_context->RSSetScissorRects(1, &rect);
}

static void r_d3d11_resize_vertex_buffer(r_state *state, ID3D11Buffer **buffer, u32 *max_size, u32 want_size, u32 stride) {
    if (want_size > *max_size) {
        want_size = u32_round_up_to_pow2(want_size);

        if (*buffer) (*buffer)->Release();

        D3D11_BUFFER_DESC vbd   = {};
        vbd.ByteWidth           = want_size;
        vbd.Usage               = D3D11_USAGE_DYNAMIC;
        vbd.BindFlags           = D3D11_BIND_VERTEX_BUFFER;
        vbd.CPUAccessFlags      = D3D11_CPU_ACCESS_WRITE;
        vbd.StructureByteStride = stride;
        HRESULT hr = state->device->CreateBuffer(&vbd, 0, &state->rect_buffer);
        asserthr(hr);

        *max_size = want_size;
    }
}

static void r_d3d11_map_write_discard(r_state *state, ID3D11Resource *buffer, void *data, i64 size) {
    D3D11_MAPPED_SUBRESOURCE ms;
    state->device_context->Map(buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &ms);
    memcpy(ms.pData, data, u64(size));
    state->device_context->Unmap(buffer, null);
}

// https://learn.microsoft.com/en-us/windows/win32/direct3ddxgi/d3d10-graphics-programming-guide-dxgi#handling-window-resizing
static void r_d3d11_resize_render_target(r_state *state, ivec2 render_target_dim) {
    if (!ivec2_eq(state->render_target_dim, render_target_dim)) {
        state->render_target_dim = render_target_dim;

        state->device_context->OMSetRenderTargets(0, 0, 0);

        // Release all outstanding references to the swap chain's buffers.
        state->render_target_view->Release();

        HRESULT hr;

        // Preserve the existing buffer count and format.
        // Automatically choose the width and height to match the client rect for HWNDs.
        hr = state->swap_chain->ResizeBuffers(0, 0, 0, DXGI_FORMAT_UNKNOWN, 0);

        // Perform error handling here!

        // Get buffer and create a render-target-view.
        ID3D11Texture2D* pBuffer;
        hr = state->swap_chain->GetBuffer(0, __uuidof(ID3D11Texture2D),
                                          (void**)&pBuffer);
        // Perform error handling here!
        hr = state->device->CreateRenderTargetView(pBuffer, null,
                                                   &state->render_target_view);
        // Perform error handling here!
        pBuffer->Release();

        state->device_context->OMSetRenderTargets(1, &state->render_target_view, null);
    }
}

static r_d3d11_compiled_shaders r_d3d11_compile_shaders(r_state *state,
                                                        const wchar_t *file_name,
                                                        D3D11_INPUT_ELEMENT_DESC *ied,
                                                        u32 ied_count) {
    r_d3d11_compiled_shaders ret = {};
    HRESULT hr = 0;

    // rune: Compile shader files.
    ID3DBlob *vs_blob, *ps_blob, *vs_err_blob, *ps_err_blob;
    hr = D3DCompileFromFile(file_name, 0, 0, "vs_main", "vs_4_0", 0, 0, &vs_blob, &vs_err_blob);
    hr = D3DCompileFromFile(file_name, 0, 0, "ps_main", "ps_4_0", 0, 0, &ps_blob, &ps_err_blob);

    // rune: Check for compilation errors.
    if (vs_err_blob) {
        printf("%s", (char *)vs_err_blob->GetBufferPointer());
        assert(!"Vertex shader compilation error.");
    }

    if (ps_err_blob) {
        printf("%s", (char *)ps_err_blob->GetBufferPointer());
        assert(!"Pixel shader compilation error.");
    }

    // rune: Get compiled buffers.
    void *vs_buffer      = vs_blob->GetBufferPointer();
    u64   vs_buffer_size = vs_blob->GetBufferSize();
    void *ps_buffer      = ps_blob->GetBufferPointer();
    u64   ps_buffer_size = ps_blob->GetBufferSize();

    // rune: Create shaders.
    hr = state->device->CreateVertexShader(vs_buffer, vs_buffer_size, null, &ret.vs);
    asserthr(hr);
    hr = state->device->CreatePixelShader(ps_buffer, ps_buffer_size, null, &ret.ps);
    asserthr(hr);

    // rune: Create input layout.
    if (ied_count) {
        hr = state->device->CreateInputLayout(ied, ied_count, vs_buffer, vs_buffer_size, &ret.il);
        asserthr(hr);
    }

    // rune: Cleanup.
    if (vs_blob)     vs_blob->Release();
    if (ps_blob)     ps_blob->Release();
    if (vs_err_blob) vs_err_blob->Release();
    if (ps_err_blob) ps_err_blob->Release();

    return ret;
}

////////////////////////////////////////////////////////////////
// rune: Draw rects

static void r_d3d11_init_rects(r_state *state) {
    HRESULT hr = 0;

    // rune: Shaders
    D3D11_INPUT_ELEMENT_DESC ied[] = {
        { "SCREEN_RECT",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0,                            0, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
        { "TEX_RECT",       0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
        { "COL",            0, DXGI_FORMAT_R32G32B32A32_UINT,  0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
        { "ROUNDNESS",      0, DXGI_FORMAT_R32_FLOAT,          0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
        { "SOFTNESS",       0, DXGI_FORMAT_R32_FLOAT,          0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
        { "TEX_WEIGHT",     0, DXGI_FORMAT_R32_FLOAT,          0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
    };

    state->rect_shaders = r_d3d11_compile_shaders(state, L"W:\\edith\\platform\\platform_win32_d3d11_rect.hlsl", ied, countof(ied));

    // rune: Constant buffer
    D3D11_BUFFER_DESC cbd = {};
    cbd.ByteWidth         = sizeof(r_grid_constant_buffer);
    cbd.Usage             = D3D11_USAGE_DYNAMIC;
    cbd.BindFlags         = D3D11_BIND_CONSTANT_BUFFER;
    cbd.CPUAccessFlags    = D3D11_CPU_ACCESS_WRITE;
    hr = state->device->CreateBuffer(&cbd, 0, &state->rect_constant_buffer);
    asserthr(hr);
}

static void r_d3d11_exec_rect_drawcall_list(r_state *state, r_d3d11_rect_drawcall_list drawcalls, vec2 screen_dim) {
    for_list (r_d3d11_rect_drawcall, drawcall, drawcalls) {
        // rune: Unpack params
        rect clip = drawcall->params.clip;
        r_d3d11_tex tex = drawcall->params.tex;

        // rune: Clipping
        r_d3d11_set_scissor(state, clip);

        // rune: Upload constant buffer.
        r_rect_constant_buffer rcb = {};
        rcb.margin_top_left   = vec2(0, 0);
        rcb.screen_dim        = screen_dim;
        //rcb.tex_channels      = tex_format == R_TEX_FORMAT_R8 ? 1 : 4;
        rcb.tex_channels      = 1;
        r_d3d11_map_write_discard(state, state->rect_constant_buffer, &rcb, sizeof(rcb));

        // rune: Upload vertex buffer.
        i64   vertex_stride = sizeof(r_rect_inst);
        void *vertex_base   = drawcall->insts.v;
        i64   vertex_size   = drawcall->insts.count * vertex_stride;
        r_d3d11_resize_vertex_buffer(state, &state->rect_buffer, &state->rect_buffer_size, u32(vertex_size), u32(vertex_stride));
        r_d3d11_map_write_discard(state, state->rect_buffer, vertex_base, vertex_size);

        // rune: Texture + sampler.
        state->device_context->PSSetShaderResources(0, 1, &tex.view);
        state->device_context->PSSetSamplers(0, 1, &state->linear_sampler);

        // rune: Draw instanced triangles.
        state->device_context->VSSetShader(state->rect_shaders.vs, 0, 0);
        state->device_context->PSSetShader(state->rect_shaders.ps, 0, 0);
        state->device_context->VSSetConstantBuffers(0, 1, &state->rect_constant_buffer);
        state->device_context->PSSetConstantBuffers(0, 1, &state->rect_constant_buffer);
        UINT stride = sizeof(r_rect_inst);
        UINT offset = 0;
        state->device_context->IASetInputLayout(state->rect_shaders.il);
        state->device_context->IASetVertexBuffers(0, 1, &state->rect_buffer, &stride, &offset);
        state->device_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
        state->device_context->DrawInstanced(4, (UINT)drawcall->insts.count, 0, 0);
    }
}

extern_c void r_startup(r_state *state, HWND hwnd) {
    HRESULT hr = 0;

    // rune: Create device and swap chain.
    DXGI_SWAP_CHAIN_DESC scd = {};
    scd.BufferCount       = 1;
    scd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    scd.BufferUsage       = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    scd.OutputWindow      = hwnd;
    scd.SampleDesc.Count  = 4; // TODOrune: @Todo Multi-sampling?
    scd.Windowed          = TRUE;

    UINT create_flags = 0;

#if _DEBUG
    create_flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    hr = D3D11CreateDeviceAndSwapChain(null, D3D_DRIVER_TYPE_HARDWARE,
                                       0,
                                       create_flags,
                                       0,
                                       0,
                                       D3D11_SDK_VERSION,
                                       &scd,
                                       &state->swap_chain,
                                       &state->device,
                                       null,
                                       &state->device_context);
    asserthr(hr);

    // rune: Create render target from swap chain buffer.
    ID3D11Texture2D *swap_chain_buffer;
    hr = state->swap_chain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void **)&swap_chain_buffer);
    asserthr(hr);
    hr = state->device->CreateRenderTargetView(swap_chain_buffer, null, &state->render_target_view);
    asserthr(hr);
    state->device_context->OMSetRenderTargets(1, &state->render_target_view, 0);
    swap_chain_buffer->Release();

    // rune: Rasterizer
    D3D11_RASTERIZER_DESC desc = {};
    desc.FillMode = D3D11_FILL_SOLID;
    desc.CullMode = D3D11_CULL_BACK;
    desc.ScissorEnable = true;
    state->device->CreateRasterizerState(&desc, &state->rasterizer);

    // rune: Blend state
    D3D11_BLEND_DESC blend_desc = {};
    blend_desc.RenderTarget[0].BlendEnable            = true;
    blend_desc.RenderTarget[0].SrcBlend               = D3D11_BLEND_SRC_ALPHA;
    blend_desc.RenderTarget[0].DestBlend              = D3D11_BLEND_INV_SRC_ALPHA;
    blend_desc.RenderTarget[0].BlendOp                = D3D11_BLEND_OP_ADD;
    blend_desc.RenderTarget[0].SrcBlendAlpha          = D3D11_BLEND_ONE;
    blend_desc.RenderTarget[0].DestBlendAlpha         = D3D11_BLEND_ZERO;
    blend_desc.RenderTarget[0].BlendOpAlpha           = D3D11_BLEND_OP_ADD;
    blend_desc.RenderTarget[0].RenderTargetWriteMask  = D3D11_COLOR_WRITE_ENABLE_ALL;
    state->device->CreateBlendState(&blend_desc, &state->blend);

    // rune: Linear sampler
    D3D11_SAMPLER_DESC sampler_desc = {};
    sampler_desc.Filter         = D3D11_FILTER_MIN_LINEAR_MAG_MIP_POINT;
    sampler_desc.AddressU       = D3D11_TEXTURE_ADDRESS_WRAP;
    sampler_desc.AddressV       = D3D11_TEXTURE_ADDRESS_WRAP;
    sampler_desc.AddressW       = D3D11_TEXTURE_ADDRESS_WRAP;
    sampler_desc.ComparisonFunc = D3D11_COMPARISON_NEVER;
    sampler_desc.MinLOD         = 0;
    sampler_desc.MaxLOD         = D3D11_FLOAT32_MAX;
    hr = state->device->CreateSamplerState(&sampler_desc, &state->linear_sampler);
    asserthr(hr);

    // rune: Initialization for each pass type
    r_d3d11_init_rects(state);

    state->arena = arena_create_default();
}

extern_c void r_shutdown(r_state *state) {
    if (state->swap_chain)           state->swap_chain->Release();
    if (state->device)               state->device->Release();
    if (state->device_context)       state->device_context->Release();
    if (state->render_target_view)   state->render_target_view->Release();
    if (state->rect_shaders.vs)      state->rect_shaders.vs->Release();
    if (state->rect_shaders.ps)      state->rect_shaders.ps->Release();
    if (state->rect_shaders.il)      state->rect_shaders.il->Release();
}

extern_c void r_render(r_state *state, ivec2 target_dim, bool need_redraw, r_d3d11_render_data *data) {
    YO_PROFILE_BEGIN(r_render);

    if (need_redraw) {

        arena_reset(state->arena);

        // rune: Resize.
        r_d3d11_resize_render_target(state, target_dim);
        r_d3d11_set_viewport(state, rect_make(vec2(0, 0), vec2_from_ivec2(target_dim)), 0.0f, 1.0f);

        // rune: Clear.
        vec4 clear_color = vec4(0.0f, 0.1f, 0.1f, 1.0f);
        state->device_context->ClearRenderTargetView(state->render_target_view, clear_color.v);

        // rune: Blending.
        state->device_context->OMSetBlendState(state->blend, 0, 0xffffffff);

        // rune: Make rect instances.
        rect viewport = rect_make_dim(VEC2_ZERO, vec2_from_ivec2(target_dim));

        // rune: Upload texture changes.
        if (data->atlas->dirty) {
            data->atlas->dirty = false;
            r_tex_update(state, data->atlas_tex, data->atlas->pixels, data->atlas->dim.x * data->atlas->dim.y);
        }

        r_d3d11_rect_batch_list batches      = r_d3d11_rect_batches_from_nodes(data->root, state->arena, data);
        r_d3d11_rect_drawcall_list drawcalls = r_d3d11_rect_drawcalls_from_batches(batches, state->arena, data);

        r_d3d11_exec_rect_drawcall_list(state, drawcalls, vec2_from_ivec2(target_dim));


        // rune: Swap buffers.
        state->swap_chain->Present(1, 0);
    } else {
        state->swap_chain->Present(1, DXGI_PRESENT_DO_NOT_SEQUENCE);
    }

    YO_PROFILE_END(r_render);
}

////////////////////////////////////////////////////////////////
// rune: Textures

extern_c r_d3d11_tex r_tex_create(r_state *state, uvec2 dim, r_tex_format format) {
    r_d3d11_tex tex = {};

    HRESULT hr = 0;

    D3D11_TEXTURE2D_DESC tex_desc = {};
    tex_desc.Width            = dim.x;
    tex_desc.Height           = dim.y;
    tex_desc.MipLevels        = 1;
    tex_desc.ArraySize        = 1;
    tex_desc.SampleDesc.Count = 1;
    tex_desc.Usage            = D3D11_USAGE_DYNAMIC;
    tex_desc.BindFlags        = D3D11_BIND_SHADER_RESOURCE;
    tex_desc.CPUAccessFlags   = D3D11_CPU_ACCESS_WRITE;
    tex_desc.MiscFlags        = 0;

    if (format == R_TEX_FORMAT_R8) tex_desc.Format = DXGI_FORMAT_R8_UNORM;
    if (format == R_TEX_FORMAT_R8G8B8A8) tex_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;

    hr = state->device->CreateTexture2D(&tex_desc, null, &tex.obj);
    asserthr(hr);

    hr = state->device->CreateShaderResourceView(tex.obj, null, &tex.view);
    asserthr(hr);

    return tex;
}

extern_c void r_tex_destroy(r_state *state, r_d3d11_tex tex) {
    if (tex.obj)  tex.obj->Release();
    if (tex.view) tex.view->Release();
}

extern_c void r_tex_update(r_state *state, r_d3d11_tex tex, void *data, i64 data_size) {
    r_d3d11_map_write_discard(state, tex.obj, data, data_size);
}
