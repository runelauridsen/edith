#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#include "base/base.h"
#include "render.h"

////////////////////////////////////////////////////////////////
//
//
// Main
//
//
////////////////////////////////////////////////////////////////

extern_c void r_startup(r_state *state, HWND hwnd) {
    unused(state, hwnd);
}

extern_c void r_shutdown(r_state *state) {
    unused(state);
}

extern_c void r_render(r_state *state, ivec2 target_dim, r_pass_list pass_list, bool need_redraw) {
    unused(state, target_dim, pass_list);
}


////////////////////////////////////////////////////////////////
//
//
// Textures
//
//
////////////////////////////////////////////////////////////////

extern_c r_tex r_create_texture(r_state *state, ivec2 dim, r_tex_format format) {
    unused(state, dim);
    return (r_tex) { 0 };
}

extern_c void r_destroy_texture(r_state *state, r_tex tex) {
    unused(state, tex);
}

extern_c void r_update_texture(r_state *state, r_tex tex, void *data, i64 data_size) {
    unused(state, tex, data, data_size);
}

