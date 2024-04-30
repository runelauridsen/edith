////////////////////////////////////////////////////////////////
// rune: Include Win32

#ifndef UNICODE
#define UNICODE
#endif
#define WIN32_LEAN_AND_MEAN

#define NOMINMAX
#include <windows.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <math.h>
#include <d3d11.h>

#pragma comment ( lib, "kernel32")
#pragma comment ( lib, "user32")

////////////////////////////////////////////////////////////////
// rune: Include Tracy profiler

#ifdef TRACY_ENABLE
#   include "thirdparty/tracy/tracy/TracyC.h"
#   define YO_PROFILE_FRAME_MARK() TracyCFrameMark
#   define YO_PROFILE_BEGIN(name)  TracyCZoneN(__##name, #name, true)
#   define YO_PROFILE_END(name)    TracyCZoneEnd(__##name)
#else
#   define YO_PROFILE_FRAME_MARK()
#   define YO_PROFILE_BEGIN(name)
#   define YO_PROFILE_END(name)
#endif

////////////////////////////////////////////////////////////////
// rune: Include base

#include "base/base.h"

#include "edith_memory.h"
#include "edith_memory.c"

#include "ui/ui.h"

////////////////////////////////////////////////////////////////
// rune: Include font backend

#define STBTT_STATIC
#define STBTT_assert assert
#define STB_TRUETYPE_IMPLEMENTATION

#include "thirdparty/stb_truetype.h"

#pragma warning(push, 0)
#define STB_IMAGE_STATIC
#define STBI_ASSERT assert
#define STB_IMAGE_IMPLEMENTATION
#include "thirdparty/stb_image.h"
#pragma warning(pop)

////////////////////////////////////////////////////////////////
// rune: Include static data

#include "thirdparty/fonts/liberation_mono.h"
#include "thirdparty/fonts/opensans_stripped_regular.h"
#include "thirdparty/fonts/opensans_stripped_bold.h"
#include "thirdparty/fonts/opensans_stripped_bold_italic.h"
#include "thirdparty/fonts/opensans_stripped_italic.h"

////////////////////////////////////////////////////////////////
// rune: Include rendering

#include "atlas.h"
#include "atlas.c"
#include "font_backend_stb.h"
#include "font_backend_stb.c"
#include "render.h"
#include "render.c"
#include "render_d3d11.h"
//#include "render_d3d11.cpp"
//#include "render_dummy.h"
//#include "render_dummy.c"
#include "draw.h"
#include "draw.c"

////////////////////////////////////////////////////////////////
// rune: Include app

// TODO(rune): Remove global. Currently used bu the query-layer to decide,
// when query results can be evicted from the query cache.
static i64 edith_global_frame_counter = 0;

#include "edith_edits.h"
#include "edith_edits.c"
#include "edith_doc.h"
#include "edith_doc.c"
#include "edith_clipboard.h"

#include "edith_pos_buffer.h"
#include "edith_pos_buffer.c"

// TODO(rune): Some kind of abstraction-layer to support multiple language
#include "lang/c/lang_c.h"

#include "edith_find_and_replace.h"
#include "edith_query.h"
#include "edith_editor_history.h"
#include "edith_editor_linemarks.h"
#include "edith_textbuf.h"
#include "edith_textview.h"
#include "edith_commands.h"

#include "edith_find_and_replace.c"
#include "edith_query.c"
#include "edith_editor_history.c"
#include "edith_editor_linemarks.c"
#include "edith_textbuf.c"
#include "edith_textview.c"
#include "edith_commands.c"

#include "edith_ui_editor.h"
#include "edith_ui_editor.c"
#include "edith_ui_dialogs.h"
#include "edith_ui_dialogs.c"
#include "edith_app.h"
#include "edith_app.c"

////////////////////////////////////////////////////////////////
// rune: Include tests

#include "tests/test_gapbuffer.c"
#include "tests/test_linemarks.c"
#include "tests/test_textview.c"
#include "tests/test_find_and_replace.c"
#include "tests/test_edits.c"
#include "tests/test_lang_c.c"
#include "tests/tests.c"

////////////////////////////////////////////////////////////////
// rune: Win32 platform layer

static bool global_clipboard_was_set_during_this_frame = false;
static bool global_clipboard_changed_externally = true;

typedef struct platform platform;
struct platform {
    edith_app app;
    HWND hwnd;
    r_state renderer;
    ui_event event_storage[1000];

    i64 prev_tick;
    i64 first_tick;
    bool running;
};

typedef enum ui_cursor_kind {
    UI_CURSOR_KIND_POINTER,
    UI_CURSOR_KIND_HAND,
    UI_CURSOR_KIND_BUSY,
    UI_CURSOR_KIND_CROSS,
    UI_CURSOR_KIND_IBEAM,
    UI_CURSOR_KIND_BLOCKED,
    UI_CURSOR_KIND_RESIZE_X,
    UI_CURSOR_KIND_RESIZE_Y,
    UI_CURSOR_KIND_RESIZE_XY,
    UI_CURSOR_KIND_RESIZE_DIAG1,
    UI_CURSOR_KIND_RESIZE_DIAG2,

    UI_CURSOR_KIND_RESIZE_COUNT,
} ui_cursor_kind;

static HCURSOR global_win32_cursor_table[UI_CURSOR_KIND_RESIZE_COUNT];

static void platform_update_and_render(platform *platform, edith_app_input input, ivec2 client_rect_dim);

static LRESULT CALLBACK win32_window_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
    switch (msg) {
        case WM_SIZE: {
            platform *platform = (struct platform *)GetWindowLongPtrW(hwnd, GWLP_USERDATA);

            ui_event resize_event = {
                .type = UI_EVENT_TYPE_RESIZE,
            };

            edith_app_input input = {
                .events.v     = &resize_event,
                .events.count = 1,
            };

            // Client rect.
            RECT client_rect_win32;
            GetClientRect(platform->hwnd, &client_rect_win32);
            ivec2 client_rect_dim = ivec2(client_rect_win32.right, client_rect_win32.bottom);
            input.client_rect.x0 = (f32)client_rect_win32.left;
            input.client_rect.y0 = (f32)client_rect_win32.top;
            input.client_rect.x1 = (f32)client_rect_win32.right;
            input.client_rect.y1 = (f32)client_rect_win32.bottom;

            // Re-render at new window size.
            platform_update_and_render(platform, input, client_rect_dim);
        } break;

        case WM_SETCURSOR: {
            SetCursor(global_win32_cursor_table[UI_CURSOR_KIND_POINTER]);
            return 0;
        } break;

        case WM_CLIPBOARDUPDATE: {
            global_clipboard_changed_externally = !global_clipboard_was_set_during_this_frame;
        } break;

        case WM_DESTROY: {
            PostQuitMessage(0);
            return 0;
        }
    }

    return DefWindowProcW(hwnd, msg, wparam, lparam);
}

static ui_key win32_translate_vkcode_to_key(u32 vkcode) {
    static readonly ui_key table[] = {
        [0] = 0,
        ['A']           = 'A',
        ['B']           = 'B',
        ['C']           = 'C',
        ['D']           = 'D',
        ['E']           = 'E',
        ['F']           = 'F',
        ['G']           = 'G',
        ['H']           = 'H',
        ['I']           = 'I',
        ['J']           = 'J',
        ['K']           = 'K',
        ['L']           = 'L',
        ['M']           = 'M',
        ['N']           = 'N',
        ['O']           = 'O',
        ['P']           = 'P',
        ['Q']           = 'Q',
        ['R']           = 'R',
        ['S']           = 'S',
        ['T']           = 'T',
        ['U']           = 'U',
        ['V']           = 'V',
        ['W']           = 'W',
        ['X']           = 'X',
        ['Y']           = 'Y',
        ['Z']           = 'Z',
        [VK_OEM_PERIOD] = '.',
        [VK_OEM_COMMA]  = ';',
        [VK_OEM_MINUS]  = '-',
        [VK_OEM_PLUS]   = '=',
        [VK_SPACE]      = UI_KEY_SPACE,
        [VK_TAB]        = UI_KEY_TAB,
        [VK_RETURN]     = UI_KEY_ENTER,
        [VK_BACK]       = UI_KEY_BACKSPACE,
        [VK_CONTROL]    = UI_KEY_CTRL,
        [VK_SHIFT]      = UI_KEY_SHIFT,
        [VK_MENU]       = UI_KEY_ALT,
        [VK_UP]         = UI_KEY_UP,
        [VK_LEFT]       = UI_KEY_LEFT,
        [VK_DOWN]       = UI_KEY_DOWN,
        [VK_RIGHT]      = UI_KEY_RIGHT,
        [VK_DELETE]     = UI_KEY_DELETE,
        [VK_PRIOR]      = UI_KEY_PAGE_UP,
        [VK_NEXT]       = UI_KEY_PAGE_DOWN,
        [VK_HOME]       = UI_KEY_HOME,
        [VK_END]        = UI_KEY_END,
        [VK_ESCAPE]     = UI_KEY_ESCAPE,
        [VK_OEM_1]      = ';',
        [VK_OEM_2]      = '/',
        [VK_OEM_3]      = 0,
        [VK_OEM_4]      = '[',
        [VK_OEM_5]      = 0,
        [VK_OEM_6]      = ']',
        [VK_OEM_7]      = '"',
        [VK_INSERT]     = UI_KEY_INSERT,
        [VK_F1]         = UI_KEY_F1,
        [VK_F2]         = UI_KEY_F2,
        [VK_F3]         = UI_KEY_F3,
        [VK_F4]         = UI_KEY_F4,
        [VK_F5]         = UI_KEY_F5,
        [VK_F6]         = UI_KEY_F6,
        [VK_F7]         = UI_KEY_F7,
        [VK_F8]         = UI_KEY_F8,
        [VK_F9]         = UI_KEY_F9,
        [VK_F1]         = UI_KEY_F10,
        [VK_F1]         = UI_KEY_F11,
        [VK_F1]         = UI_KEY_F12,
    };

    if (vkcode < countof(table)) {
        return table[vkcode];
    } else {
        return 0;
    }
}

static ui_mouse_buttons win32_translate_wm_to_mousebutton(u32 wm) {
    switch (wm) {
        case WM_LBUTTONUP:      return UI_MOUSE_BUTTON_LEFT;
        case WM_LBUTTONDOWN:    return UI_MOUSE_BUTTON_LEFT;
        case WM_LBUTTONDBLCLK:  return UI_MOUSE_BUTTON_LEFT;
        case WM_RBUTTONUP:      return UI_MOUSE_BUTTON_RIGHT;
        case WM_RBUTTONDOWN:    return UI_MOUSE_BUTTON_RIGHT;
        case WM_RBUTTONDBLCLK:  return UI_MOUSE_BUTTON_RIGHT;
        case WM_MBUTTONUP:      return UI_MOUSE_BUTTON_MIDDLE;
        case WM_MBUTTONDOWN:    return UI_MOUSE_BUTTON_MIDDLE;
        case WM_MBUTTONDBLCLK:  return UI_MOUSE_BUTTON_MIDDLE;
        default:                return 0;
    }
}

static ui_modifiers win32_get_modifers(void) {
    ui_modifiers ret = { 0 };
    if (GetKeyState(VK_CONTROL) & 0x8000) ret |= UI_MODIFIER_CTRL;
    if (GetKeyState(VK_SHIFT)   & 0x8000) ret |= UI_MODIFIER_SHIFT;
    if (GetKeyState(VK_MENU)    & 0x8000) ret |= UI_MODIFIER_ALT;
    return ret;
}

static ui_mouse_buttons win32_get_mouse_buttons(void) {
    ui_mouse_buttons ret = { 0 };
    if (GetKeyState(VK_LBUTTON) & 0x8000) ret |= UI_MOUSE_BUTTON_LEFT;
    if (GetKeyState(VK_RBUTTON) & 0x8000) ret |= UI_MOUSE_BUTTON_RIGHT;
    if (GetKeyState(VK_MBUTTON) & 0x8000) ret |= UI_MOUSE_BUTTON_MIDDLE;
    return ret;
}

static vec2 win32_get_mouse_pos(HWND hwnd) {
    POINT point = { 0 };
    GetCursorPos(&point);
    ScreenToClient(hwnd, &point);
    vec2 ret = vec2((f32)point.x, (f32)point.y);
    return ret;
}

static void platform_callback_set_clipboard(str data) {
    if (OpenClipboard(0)) {
        EmptyClipboard();
        HANDLE hmem = GlobalAlloc(GMEM_MOVEABLE, data.len + 1);
        if (hmem) {
            u8 *dst = GlobalLock(hmem);
            if (dst) {
                memcpy(dst, data.v, data.len);
                dst[data.len] = '\0';
                GlobalUnlock(hmem);
                SetClipboardData(CF_TEXT, hmem);
                global_clipboard_was_set_during_this_frame = true;
            }
        }
        CloseClipboard();
    }
}

static str platform_callback_get_clipboard(arena *arena) {
    str ret = { 0 };
    if (IsClipboardFormatAvailable(CF_TEXT) &&
        OpenClipboard(0)) {
        HANDLE hmem = GetClipboardData(CF_TEXT);
        if (hmem) {
            u8 *src = GlobalLock(hmem);
            if (src) {
                i64 len = cstr_len((char *)src);
                ret = arena_copy_str(arena, str_make(src, len));
                GlobalUnlock(hmem);
            }
        }
        CloseClipboard();
    }
    return ret;
}

static bool platform_callback_clipboard_changed_externally(void) {
    return global_clipboard_changed_externally;
}

static void platform_update_and_render(platform *platform, edith_app_input input, ivec2 client_rect_dim) {
    edith_app_output output = { 0 };
    output = edith_app_update_and_render(&platform->app, input);
    if (platform->app.stop) {
        platform->running = false;
    }

    r_render(&platform->renderer, client_rect_dim, output.render_passes, output.need_redraw);
}

static bool do_main_loop_iteration(void **user_data) {
    YO_PROFILE_BEGIN(do_main_loop_iteration);

    platform *platform = *user_data;

    ////////////////////////////////////////////////////////////////
    // First time initialization

    if (platform == null) {
        platform = edith_heap_alloc(sizeof(struct platform), str("platform struct"));
        zero_struct(platform);
        *user_data = platform;

        SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

        // rune: Load cursor handles
        global_win32_cursor_table[UI_CURSOR_KIND_POINTER]      = LoadCursorW(0, IDC_ARROW);
        global_win32_cursor_table[UI_CURSOR_KIND_HAND]         = LoadCursorW(0, IDC_HAND);
        global_win32_cursor_table[UI_CURSOR_KIND_BUSY]         = LoadCursorW(0, IDC_WAIT);
        global_win32_cursor_table[UI_CURSOR_KIND_CROSS]        = LoadCursorW(0, IDC_CROSS);
        global_win32_cursor_table[UI_CURSOR_KIND_IBEAM]        = LoadCursorW(0, IDC_IBEAM);
        global_win32_cursor_table[UI_CURSOR_KIND_BLOCKED]      = LoadCursorW(0, IDC_NO);
        global_win32_cursor_table[UI_CURSOR_KIND_RESIZE_X]     = LoadCursorW(0, IDC_SIZENS);
        global_win32_cursor_table[UI_CURSOR_KIND_RESIZE_Y]     = LoadCursorW(0, IDC_SIZEWE);
        global_win32_cursor_table[UI_CURSOR_KIND_RESIZE_XY]    = LoadCursorW(0, IDC_SIZEALL);
        global_win32_cursor_table[UI_CURSOR_KIND_RESIZE_DIAG1] = LoadCursorW(0, IDC_SIZENWSE);
        global_win32_cursor_table[UI_CURSOR_KIND_RESIZE_DIAG2] = LoadCursorW(0, IDC_SIZENESW);

        // rune: Register window class.
        WNDCLASSW window_class = { 0 };
        window_class.lpszClassName = L"edith_window_class";
        window_class.lpfnWndProc = &win32_window_proc;
        window_class.hCursor = global_win32_cursor_table[UI_CURSOR_KIND_POINTER];
        if (!RegisterClassW(&window_class)) {
            println("RegisterClassA failed (%).", (u32)GetLastError());
            return false;
        }

        // rune: Create window.
        platform->hwnd = CreateWindowExW(0, L"edith_window_class", L"Edith", WS_OVERLAPPEDWINDOW,
                                         CW_USEDEFAULT, CW_USEDEFAULT, 800, 600,
                                         0, 0, 0, 0);

        if (!platform->hwnd) {
            println("CreateWindowExW failed (%).", (u32)GetLastError());
            return 0;
        }

        // rune: Initialize D3D11.
        r_startup(&platform->renderer, platform->hwnd);

        // rune: Initialize draw.
        draw_create_ctx(&platform->app.draw_ctx, &platform->renderer);

        // rune: Store pointer to platform struct on hwnd, so we can access it in the wndproc.
        SetWindowLongPtrW(platform->hwnd, GWLP_USERDATA, (LONG_PTR)platform);

        // rune: Show window.
        ShowWindow(platform->hwnd, SW_SHOW);
        platform->first_tick = GetTickCount64();
        platform->prev_tick = platform->first_tick;
        platform->running =  true;

        // rune: Platform callbacks.
        platform->app.callbacks.set_clipboard                = platform_callback_set_clipboard;
        platform->app.callbacks.get_clipboard                = platform_callback_get_clipboard;
        platform->app.callbacks.clipboard_changed_externally = platform_callback_clipboard_changed_externally;

        // rune: Receive WM_CLIPBOARDUPDATE messages.
        AddClipboardFormatListener(platform->hwnd);

        // rune: Launch background query thread
        os_thread_create(edith_query_thread_entry_point, null);

        // rune: Setup global command ringbuffer
        edith_global_cmd_ring.ring.size = kilobytes(64);
        edith_global_cmd_ring.ring.base = edith_heap_alloc(edith_global_cmd_ring.ring.size, str("cmd ring"));
    }

    edith_app_input input = { 0 };
    input.events.v = platform->event_storage;
    input.events.count = 0;

    // Poll win32 messages.
    MSG msg = { 0 };
    while (PeekMessageA(&msg, 0, 0, 0, PM_REMOVE)) {
        if (msg.message == WM_QUIT) {
            platform->running = false;
            break;
        }

        TranslateMessage(&msg);
        DispatchMessageA(&msg);

        WPARAM wparam = msg.wParam;
        LPARAM lparam = msg.lParam;

        if (input.events.count < countof(platform->event_storage)) {
            switch (msg.message) {
                case WM_SYSKEYDOWN:
                case WM_KEYDOWN: {
                    ui_event *e  = &input.events.v[input.events.count++];
                    e->type      = UI_EVENT_TYPE_KEYPRESS;
                    e->modifiers = win32_get_modifers();
                    e->key       = win32_translate_vkcode_to_key((u32)wparam);
                    e->codepoint = 0;
                } break;

                case WM_SYSKEYUP:
                case WM_KEYUP: {
                    ui_event *e  = &input.events.v[input.events.count++];
                    e->type      = UI_EVENT_TYPE_KEY_RELEASE;
                    e->modifiers = win32_get_modifers();
                    e->key       = win32_translate_vkcode_to_key((u32)wparam);
                    e->codepoint = 0;
                } break;

                case WM_CHAR: {
                    if (wparam >= 32 && wparam != 127) {
                        ui_event *e  = &input.events.v[input.events.count++];
                        e->type      = UI_EVENT_TYPE_CODEPOINT;
                        e->modifiers = win32_get_modifers();
                        e->codepoint = (u32)wparam;
                    }
                } break;

                case WM_LBUTTONDOWN:
                    // NOTE(rune): Tell windows that we still want mouse-move messages, when the user is dragging outside our own window.
                    SetCapture(platform->hwnd);
                case WM_RBUTTONDOWN:
                case WM_MBUTTONDOWN: {
                    ui_event *e    = &input.events.v[input.events.count++];
                    e->type        = UI_EVENT_TYPE_CLICK;
                    e->modifiers   = win32_get_modifers();
                    e->mousebutton = win32_translate_wm_to_mousebutton(msg.message);
                    e->pos.x       = (f32)LOWORD(lparam);
                    e->pos.y       = (f32)HIWORD(lparam);
                } break;

                case WM_LBUTTONUP:
                    ReleaseCapture();
                case WM_RBUTTONUP:
                case WM_MBUTTONUP: {
                    // TODO(rune): Currently the UI_EVENT_TYPE_MOUSE_UP event is only used for
                    // triggering af ui rebuild.
                    ui_event *e    = &input.events.v[input.events.count++];
                    e->type        = UI_EVENT_TYPE_MOUSE_UP;
                    e->mousebutton = win32_translate_wm_to_mousebutton(msg.message);
                } break;

                case WM_MOUSEWHEEL: {
                    ui_event *e  = &input.events.v[input.events.count++];
                    e->type      = UI_EVENT_TYPE_SCROLL;
                    e->modifiers = win32_get_modifers();
                    e->scroll.x  = 0.0f; // TODO(rune): Horizontal scroll.
                    e->scroll.y  = -(f32)(i16)HIWORD(msg.wParam) / 120.0f;
                } break;

                case WM_MOUSEMOVE: {
                    // TODO(rune): Currently the UI_EVENT_TYPE_MOUSE_MOVE event is only used for
                    // triggering af ui rebuild.
                    ui_event *e  = &input.events.v[input.events.count++];
                    e->type      = UI_EVENT_TYPE_MOUSE_MOVE;
                } break;
            }
        }
    }

    input.window_has_focus = (GetActiveWindow() == platform->hwnd);

    // Globals.
    global_clipboard_was_set_during_this_frame = false;

    // Mouse.
    input.mouse_buttons = win32_get_mouse_buttons();
    input.mouse_pos     = win32_get_mouse_pos(platform->hwnd);

    // Window size.
    RECT client_rect_win32;
    GetClientRect(platform->hwnd, &client_rect_win32);
    ivec2 client_rect_dim = ivec2(client_rect_win32.right, client_rect_win32.bottom);
    input.client_rect.x0 = (f32)client_rect_win32.left;
    input.client_rect.y0 = (f32)client_rect_win32.top;
    input.client_rect.x1 = (f32)client_rect_win32.right;
    input.client_rect.y1 = (f32)client_rect_win32.bottom;

    // Deltatime.
    i64 this_tick = GetTickCount64();
    input.deltatime = (f32)(this_tick - platform->prev_tick) / 1000.0f;
    input.tick = this_tick - platform->first_tick;
    platform->prev_tick = this_tick;

    // Update.
    if (platform->running) {
        platform_update_and_render(platform, input, client_rect_dim);
    }

    YO_PROFILE_END(do_main_loop_iteration);
    return platform->running;
}

int main(void) {
    SetConsoleOutputCP(65001); // NOTE(rune): UTF8 code page

    os_init();
    edith_mem_init();

#ifdef RUN_TESTS
    run_tests();
#else
    void *user_data = null;
    while (do_main_loop_iteration(&user_data)) {
    }
#endif

    return 0;
}

int WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd) {
    unused(hInstance, hPrevInstance, lpCmdLine, nShowCmd);
    return main();
}
