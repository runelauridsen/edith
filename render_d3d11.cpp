////////////////////////////////////////////////////////////////
// rune: Include Win32 and D3D11

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <d3d11.h>
#include <d3dcompiler.h>

#pragma comment ( lib, "kernel32")
#pragma comment ( lib, "user32")
#pragma comment ( lib, "d3d11")
#pragma comment ( lib, "dxguid")
#pragma comment ( lib, "d3dcompiler")

////////////////////////////////////////////////////////////////
// rune: Include standard library

#include <stdio.h>
#include <stdint.h>
#include <math.h>

////////////////////////////////////////////////////////////////
// rune: Include app

#include "base/base.h"
#include "render.h"
#include "render_d3d11.h"

////////////////////////////////////////////////////////////////
// rune: Include static data

#include "thirdparty/fonts/font8x8_basic.h"

////////////////////////////////////////////////////////////////
// rune: D3D11 rendering implementation

#define assert_hr(hr) assert(hr >= 0) // TODO(rune): More robust error handling

static void r_d3d11_set_viewport(r_state *state, rect rect, f32 min_z, f32 max_z) {
    D3D11_VIEWPORT vp = {};
    vp.Width    = rect_dim_x(rect);
    vp.Height   = rect_dim_y(rect);
    vp.MinDepth = min_z;
    vp.MaxDepth = max_z;
    vp.TopLeftX = rect.p0.x;
    vp.TopLeftY = rect.p0.y;
    state->device_context->RSSetViewports(1, &vp);
}

static void r_d3d11_resize_vertex_buffer(r_state *state, ID3D11Buffer **buffer, i32 *max_size, i32 want_size, i32 stride) {
    if (want_size > *max_size) {
        want_size = i32_round_up_to_pow2(want_size);

        if (*buffer) (*buffer)->Release();

        D3D11_BUFFER_DESC vbd   = {};
        vbd.ByteWidth           = u32(want_size);
        vbd.Usage               = D3D11_USAGE_DYNAMIC;
        vbd.BindFlags           = D3D11_BIND_VERTEX_BUFFER;
        vbd.CPUAccessFlags      = D3D11_CPU_ACCESS_WRITE;
        vbd.StructureByteStride = u32(stride);
        HRESULT hr = state->device->CreateBuffer(&vbd, 0, &state->rect_buffer);
        assert_hr(hr);

        *max_size = want_size;
    }
}

static void r_d3d11_map_write_discard(r_state *state, ID3D11Resource *buffer, void *data, i64 size) {
    D3D11_MAPPED_SUBRESOURCE ms;
    state->device_context->Map(buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &ms);
    memcpy(ms.pData, data, u64(size));
    state->device_context->Unmap(buffer, null);
}

static void r_d3d11_resize_cell_buffer(r_state *state, i32 cell_count) {
    if (state->max_cell_count < cell_count) {
        if (state->cell_buffer)  state->cell_buffer->Release();
        if (state->cell_view)    state->cell_view->Release();

        {
            D3D11_BUFFER_DESC buffer_desc   = {};
            buffer_desc.ByteWidth           = cell_count * sizeof(r_gridcell);
            buffer_desc.Usage               = D3D11_USAGE_DYNAMIC;
            buffer_desc.BindFlags           = D3D11_BIND_SHADER_RESOURCE;
            buffer_desc.CPUAccessFlags      = D3D11_CPU_ACCESS_WRITE;
            buffer_desc.MiscFlags           = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
            buffer_desc.StructureByteStride = sizeof(r_gridcell);

            HRESULT hr = state->device->CreateBuffer(&buffer_desc, 0, &state->cell_buffer);
            assert_hr(hr);
        }

        {
            D3D11_SHADER_RESOURCE_VIEW_DESC view_desc = {};
            view_desc.ViewDimension                   = D3D11_SRV_DIMENSION_BUFFER;
            view_desc.Buffer.NumElements              = u32(cell_count);

            HRESULT hr = state->device->CreateShaderResourceView(state->cell_buffer, &view_desc, &state->cell_view);
            assert_hr(hr);
        }

        state->max_cell_count = cell_count;
    }
}

static void r_d3d11_resize_render_target(r_state *state, ivec2 render_target_dim) {
    if (!ivec2_eq(state->render_target_dim, render_target_dim)) {
        state->render_target_dim = render_target_dim;

        // https://learn.microsoft.com/en-us/windows/win32/direct3ddxgi/d3d10-graphics-programming-guide-dxgi#handling-window-resizing
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

    // (rune): Compile shader files.
    ID3DBlob *vs_blob, *ps_blob, *vs_err_blob, *ps_err_blob;
    hr = D3DCompileFromFile(file_name, 0, 0, "vs_main", "vs_4_0", 0, 0, &vs_blob, &vs_err_blob);
    hr = D3DCompileFromFile(file_name, 0, 0, "ps_main", "ps_4_0", 0, 0, &ps_blob, &ps_err_blob);

    // (rune): Check for compilation errors.
    if (vs_err_blob) {
        printf("%s", (char *)vs_err_blob->GetBufferPointer());
        assert(!"Vertex shader compilation error.");
    }

    if (ps_err_blob) {
        printf("%s", (char *)ps_err_blob->GetBufferPointer());
        assert(!"Pixel shader compilation error.");
    }

    // (rune): Get compiled buffers.
    void *vs_buffer      = vs_blob->GetBufferPointer();
    u64   vs_buffer_size = vs_blob->GetBufferSize();
    void *ps_buffer      = ps_blob->GetBufferPointer();
    u64   ps_buffer_size = ps_blob->GetBufferSize();

    // (rune): Create shaders.
    hr = state->device->CreateVertexShader(vs_buffer, vs_buffer_size, null, &ret.vs);
    assert_hr(hr);
    hr = state->device->CreatePixelShader(ps_buffer, ps_buffer_size, null, &ret.ps);
    assert_hr(hr);

    // (rune): Create input layout.
    if (ied_count) {
        hr = state->device->CreateInputLayout(ied, ied_count, vs_buffer, vs_buffer_size, &ret.il);
        assert_hr(hr);
    }

    // (rune): Cleanup.
    if (vs_blob)     vs_blob->Release();
    if (ps_blob)     ps_blob->Release();
    if (vs_err_blob) vs_err_blob->Release();
    if (ps_err_blob) ps_err_blob->Release();

    return ret;
}

////////////////////////////////////////////////////////////////
//
//
// Draw grid
//
//
////////////////////////////////////////////////////////////////

static void r_d3d11_init_grid(r_state *state) {
    HRESULT hr = 0;

    state->grid_shaders = r_d3d11_compile_shaders(state, L"W:\\edith\\render_d3d11_grid.hlsl", 0, 0);

    // (rune): Constant buffer.
    D3D11_BUFFER_DESC constant_buffer_desc = {};
    constant_buffer_desc.ByteWidth         = sizeof(r_grid_constant_buffer);
    constant_buffer_desc.Usage             = D3D11_USAGE_DYNAMIC;
    constant_buffer_desc.BindFlags         = D3D11_BIND_CONSTANT_BUFFER;
    constant_buffer_desc.CPUAccessFlags    = D3D11_CPU_ACCESS_WRITE;
    hr = state->device->CreateBuffer(&constant_buffer_desc, 0, &state->cell_constant_buffer);
    assert_hr(hr);

    // (rune): Allocate texture data.
    ivec2 tex_dim = ivec2(256, 256);
    i64   tex_size = tex_dim.x * tex_dim.y * isizeof(u32);
    u32  *tex_data = (u32 *)heap_alloc(tex_size);
    for_n (i32, y, tex_dim.y) {
        for_n (i32, x, tex_dim.x) {
            i32 i = x + y * tex_dim.x;

            i32 c = ((x / 8) + 32 * (y / 8)) % 128;

            i32 cx = x % 8;
            i32 cy = y % 8;

            u8 row = font8x8_basic[c][cy];
            u8 bit = u8(row & (1 << cx));

            tex_data[i] = bit ? 0xffffffff : 0;
        }
    }

    // (rune): Create texture.
    D3D11_TEXTURE2D_DESC tex_desc = {};
    tex_desc.Width            = u32(tex_dim.x);
    tex_desc.Height           = u32(tex_dim.y);
    tex_desc.MipLevels        = 1;
    tex_desc.ArraySize        = 1;
    tex_desc.Format           = DXGI_FORMAT_R8G8B8A8_UNORM;
    tex_desc.SampleDesc.Count = 1;
    tex_desc.Usage            = D3D11_USAGE_DYNAMIC;
    tex_desc.BindFlags        = D3D11_BIND_SHADER_RESOURCE;
    tex_desc.CPUAccessFlags   = D3D11_CPU_ACCESS_WRITE;
    tex_desc.MiscFlags        = 0;

    hr = state->device->CreateTexture2D(&tex_desc, null, &state->tex);
    assert_hr(hr);

    // (rune): Fill texture data.
    r_d3d11_map_write_discard(state, state->tex, tex_data, tex_size);

    // (rune): Create texture view.
    hr = state->device->CreateShaderResourceView(state->tex, null, &state->tex_view);
    assert_hr(hr);

    // (rune): Create sampler.
    D3D11_SAMPLER_DESC sampler_desc = {};
    sampler_desc.Filter         = D3D11_FILTER_MIN_LINEAR_MAG_MIP_POINT;
    sampler_desc.AddressU       = D3D11_TEXTURE_ADDRESS_WRAP;
    sampler_desc.AddressV       = D3D11_TEXTURE_ADDRESS_WRAP;
    sampler_desc.AddressW       = D3D11_TEXTURE_ADDRESS_WRAP;
    sampler_desc.ComparisonFunc = D3D11_COMPARISON_NEVER;
    sampler_desc.MinLOD         = 0;
    sampler_desc.MaxLOD         = D3D11_FLOAT32_MAX;

    hr = state->device->CreateSamplerState(&sampler_desc, &state->linear_sampler);
    assert_hr(hr);
}

static void r_d3d11_render_grid(r_state *state, r_pass_grid pass) {
    r_d3d11_set_viewport(state, pass.viewport, 0.0f, 1.0f);

    r_grid grid = pass.grid;
    r_d3d11_resize_cell_buffer(state, grid.cell_count.x * grid.cell_count.y);

    // (rune): Upload constant buffer.
    ivec2 cell_dim = ivec2(8, 8); // TODO(rune): Hardcoded glyph dimensions.
    r_grid_constant_buffer gcb = {};
    gcb.cell_dim          = cell_dim;
    gcb.cell_count         = grid.cell_count;
    gcb.margin_top_left.x = (i32)pass.viewport.p0.x;
    gcb.margin_top_left.y = (i32)pass.viewport.p0.y;
    gcb.screen_dim.x      = (i32)rect_dim_x(pass.viewport);
    gcb.screen_dim.y      = (i32)rect_dim_y(pass.viewport);
    r_d3d11_map_write_discard(state, state->cell_constant_buffer, &gcb, sizeof(gcb));

    // (rune): Upload cell buffer.
    r_d3d11_map_write_discard(state, state->cell_buffer, grid.cells, grid.cell_count.x * grid.cell_count.y * isizeof(r_gridcell));

    // (rune): Draw indexed triangles.
    state->device_context->VSSetShader(state->grid_shaders.vs, 0, 0);
    state->device_context->PSSetShader(state->grid_shaders.ps, 0, 0);
    state->device_context->PSSetConstantBuffers(0, 1, &state->cell_constant_buffer);

    ID3D11ShaderResourceView *shader_resources[] = {
        state->cell_view,   // NOTE(rune): register(t0)
        state->tex_view     // NOTE(rune): register(t1)
    };

    state->device_context->PSSetShaderResources(0, countof(shader_resources), shader_resources);
    state->device_context->PSSetSamplers(0, 1, &state->linear_sampler);

    state->device_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
    state->device_context->Draw(4, 0);
}

////////////////////////////////////////////////////////////////
//
//
// Draw rects
//
//
////////////////////////////////////////////////////////////////

static void r_d3d11_init_rects(r_state *state) {
    HRESULT hr = 0;

    // (rune): Shaders.
    D3D11_INPUT_ELEMENT_DESC ied[] = {
        { "SCREEN_RECT",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0,                            0, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
        { "TEX_RECT",       0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
        { "COL",            0, DXGI_FORMAT_R32G32B32A32_UINT,  0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA, 1},
        { "ROUNDNESS",      0, DXGI_FORMAT_R32_FLOAT,          0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
        { "SOFTNESS",       0, DXGI_FORMAT_R32_FLOAT,          0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
        { "TEX_WEIGHT",     0, DXGI_FORMAT_R32_FLOAT,          0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
    };

    state->rect_shaders = r_d3d11_compile_shaders(state, L"W:\\edith\\render_d3d11_rect.hlsl", ied, countof(ied));

    // (rune): Constant buffer..
    D3D11_BUFFER_DESC cbd = {};
    cbd.ByteWidth         = sizeof(r_grid_constant_buffer);
    cbd.Usage             = D3D11_USAGE_DYNAMIC;
    cbd.BindFlags         = D3D11_BIND_CONSTANT_BUFFER;
    cbd.CPUAccessFlags    = D3D11_CPU_ACCESS_WRITE;
    hr = state->device->CreateBuffer(&cbd, 0, &state->rect_constant_buffer);
    assert_hr(hr);
}

static void r_d3d11_render_rects(r_state *state, r_pass_rects pass) {
    if (!pass.instances.count) return;

    // (rune): Set viewport.
    r_d3d11_set_viewport(state, pass.viewport, 0.0f, 1.0f);

    // (rune): Upload constant buffer.
    r_rect_constant_buffer rcb = {};
    rcb.margin_top_left   = pass.viewport.p0;
    rcb.screen_dim        = rect_dim(pass.viewport);
    rcb.tex_channels      = pass.tex_format == R_TEX_FORMAT_R8 ? 1 : 4;
    r_d3d11_map_write_discard(state, state->rect_constant_buffer, &rcb, sizeof(rcb));

    // (rune): Upload vertex buffer.
    r_d3d11_resize_vertex_buffer(state, &state->rect_buffer, &state->rect_buffer_size, i32(pass.instances.count * sizeof(r_rect_instance)), sizeof(r_rect_instance));
    r_d3d11_map_write_discard(state, state->rect_buffer, pass.instances.v, pass.instances.count * isizeof(r_rect_instance));

    // (rune): Texture + sampler.
    r_d3d11_tex tex = *(r_d3d11_tex *)pass.tex.data;
    state->device_context->PSSetShaderResources(0, 1, &tex.view);
    state->device_context->PSSetSamplers(0, 1, &state->linear_sampler);

    // (rune): Draw instanced triangles.
    state->device_context->VSSetShader(state->rect_shaders.vs, 0, 0);
    state->device_context->PSSetShader(state->rect_shaders.ps, 0, 0);
    state->device_context->VSSetConstantBuffers(0, 1, &state->rect_constant_buffer);
    state->device_context->PSSetConstantBuffers(0, 1, &state->rect_constant_buffer);
    UINT stride = sizeof(r_rect_instance);
    UINT offset = 0;
    state->device_context->IASetInputLayout(state->rect_shaders.il);
    state->device_context->IASetVertexBuffers(0, 1, &state->rect_buffer, &stride, &offset);
    state->device_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
    state->device_context->DrawInstanced(4, (UINT)pass.instances.count, 0, 0);
}

extern_c void r_startup(r_state *state, HWND hwnd) {
    HRESULT hr = 0;

    // (rune): Create device and swap chain.
    DXGI_SWAP_CHAIN_DESC scd = {};
    scd.BufferCount       = 1;
    scd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    scd.BufferUsage       = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    scd.OutputWindow      = hwnd;
    scd.SampleDesc.Count  = 4; // TODO(rune): @Todo Multi-sampling?
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
    assert_hr(hr);

    // (rune): Create render target from swap chain buffer.
    ID3D11Texture2D *swap_chain_buffer;
    hr = state->swap_chain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void **)&swap_chain_buffer);
    assert_hr(hr);
    hr = state->device->CreateRenderTargetView(swap_chain_buffer, null, &state->render_target_view);
    assert_hr(hr);
    state->device_context->OMSetRenderTargets(1, &state->render_target_view, 0);
    swap_chain_buffer->Release();

    // (rune): Blend state.
    D3D11_BLEND_DESC desc = {};
    desc.RenderTarget[0].BlendEnable            = true;
    desc.RenderTarget[0].SrcBlend               = D3D11_BLEND_SRC_ALPHA;
    desc.RenderTarget[0].DestBlend              = D3D11_BLEND_INV_SRC_ALPHA;
    desc.RenderTarget[0].BlendOp                = D3D11_BLEND_OP_ADD;
    desc.RenderTarget[0].SrcBlendAlpha          = D3D11_BLEND_ONE;
    desc.RenderTarget[0].DestBlendAlpha         = D3D11_BLEND_ZERO;
    desc.RenderTarget[0].BlendOpAlpha           = D3D11_BLEND_OP_ADD;
    desc.RenderTarget[0].RenderTargetWriteMask  = D3D11_COLOR_WRITE_ENABLE_ALL;

    state->device->CreateBlendState(&desc, &state->blend);

    // (rune): Initialization for each pass type.
    r_d3d11_init_grid(state);
    r_d3d11_init_rects(state);
}

extern_c void r_shutdown(r_state *state) {
    if (state->swap_chain)           state->swap_chain->Release();
    if (state->device)               state->device->Release();
    if (state->device_context)       state->device_context->Release();
    if (state->render_target_view)   state->render_target_view->Release();
    if (state->grid_shaders.vs)      state->grid_shaders.vs->Release();
    if (state->grid_shaders.ps)      state->grid_shaders.ps->Release();
    if (state->grid_shaders.il)      state->grid_shaders.il->Release();
    if (state->rect_shaders.vs)      state->rect_shaders.vs->Release();
    if (state->rect_shaders.ps)      state->rect_shaders.ps->Release();
    if (state->rect_shaders.il)      state->rect_shaders.il->Release();
}

extern_c void r_render(r_state *state, ivec2 target_dim, r_pass_list pass_list, bool need_redraw) {
    if (need_redraw) {

        // (rune): Resize.
        r_d3d11_resize_render_target(state, target_dim);

        // (rune): Clear.
        vec4 clear_color = vec4(0.0f, 0.1f, 0.1f, 1.0f);
        state->device_context->ClearRenderTargetView(state->render_target_view, clear_color.v);

        // (rune): Blending.
        state->device_context->OMSetBlendState(state->blend, 0, 0xffffffff);

        for_list (r_pass, it, pass_list) {
            switch (it->type) {
                case R_PASS_TYPE_GRID: {
                    r_d3d11_render_grid(state, it->type_grid);
                } break;

                case R_PASS_TYPE_RECT: {
                    r_d3d11_render_rects(state, it->type_rects);
                } break;

                default: {
                    assert(!"Invalid render pass type.");
                } break;
            }
        }

        // (rune): Swap buffers.
        state->swap_chain->Present(1, 0);
    } else {
        state->swap_chain->Present(1, DXGI_PRESENT_DO_NOT_SEQUENCE);
    }
}

////////////////////////////////////////////////////////////////
//
//
// Textures
//
//
////////////////////////////////////////////////////////////////

extern_c r_tex r_create_texture(r_state *state, ivec2 dim, r_tex_format format) {
    r_tex ret = {};
    r_d3d11_tex *tex = (r_d3d11_tex *)ret.data;

    HRESULT hr = 0;

    D3D11_TEXTURE2D_DESC tex_desc = {};
    tex_desc.Width            = u32(dim.x);
    tex_desc.Height           = u32(dim.y);
    tex_desc.MipLevels        = 1;
    tex_desc.ArraySize        = 1;
    tex_desc.SampleDesc.Count = 1;
    tex_desc.Usage            = D3D11_USAGE_DYNAMIC;
    tex_desc.BindFlags        = D3D11_BIND_SHADER_RESOURCE;
    tex_desc.CPUAccessFlags   = D3D11_CPU_ACCESS_WRITE;
    tex_desc.MiscFlags        = 0;

    if (format == R_TEX_FORMAT_R8) tex_desc.Format = DXGI_FORMAT_R8_UNORM;
    if (format == R_TEX_FORMAT_R8G8B8A8) tex_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;

    hr = state->device->CreateTexture2D(&tex_desc, null, &tex->obj);
    assert_hr(hr);

    hr = state->device->CreateShaderResourceView(tex->obj, null, &tex->view);
    assert_hr(hr);

    return ret;
}

extern_c void r_destroy_texture(r_state *state, r_tex tex_) {
    unused(state);
    r_d3d11_tex *tex = (r_d3d11_tex *)tex_.data;

    if (tex->obj)  tex->obj->Release();
    if (tex->view) tex->view->Release();
}

extern_c void r_update_texture(r_state *state, r_tex tex_, void *data, i64 data_size) {
    r_d3d11_tex *tex = (r_d3d11_tex *)tex_.data;
    r_d3d11_map_write_discard(state, tex->obj, data, data_size);
}
