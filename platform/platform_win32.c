typedef struct platform_win32 platform_win32;
struct platform_win32 {
    HWND hwnd;
    font_backend font_backend;
    atlas atlas;

    r_state renderer;
    r_d3d11_tex atlas_tex;

    yo_state *state;

    u64 prev_tick;
    u64 first_tick;
    bool running;

    arena *arena;
};

static void platform_update_and_render(platform_win32 *platform, ivec2 client_rect_dim, yo_event_list events);

static yo_key win32_key_from_vkcode(u32 vkcode) {
    static readonly yo_key table[] = {
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
        [VK_SPACE]      = YO_KEY_SPACE,
        [VK_TAB]        = YO_KEY_TAB,
        [VK_RETURN]     = YO_KEY_ENTER,
        [VK_BACK]       = YO_KEY_BACKSPACE,
        [VK_CONTROL]    = YO_KEY_CTRL,
        [VK_SHIFT]      = YO_KEY_SHIFT,
        [VK_MENU]       = YO_KEY_ALT,
        [VK_UP]         = YO_KEY_UP,
        [VK_LEFT]       = YO_KEY_LEFT,
        [VK_DOWN]       = YO_KEY_DOWN,
        [VK_RIGHT]      = YO_KEY_RIGHT,
        [VK_DELETE]     = YO_KEY_DELETE,
        [VK_PRIOR]      = YO_KEY_PAGE_UP,
        [VK_NEXT]       = YO_KEY_PAGE_DOWN,
        [VK_HOME]       = YO_KEY_HOME,
        [VK_END]        = YO_KEY_END,
        [VK_ESCAPE]     = YO_KEY_ESCAPE,
        [VK_OEM_1]      = ';',
        [VK_OEM_2]      = '/',
        [VK_OEM_3]      = 0,
        [VK_OEM_4]      = '[',
        [VK_OEM_5]      = 0,
        [VK_OEM_6]      = ']',
        [VK_OEM_7]      = '"',
        [VK_INSERT]     = YO_KEY_INSERT,
        [VK_F1]         = YO_KEY_F1,
        [VK_F2]         = YO_KEY_F2,
        [VK_F3]         = YO_KEY_F3,
        [VK_F4]         = YO_KEY_F4,
        [VK_F5]         = YO_KEY_F5,
        [VK_F6]         = YO_KEY_F6,
        [VK_F7]         = YO_KEY_F7,
        [VK_F8]         = YO_KEY_F8,
        [VK_F9]         = YO_KEY_F9,
        [VK_F10]        = YO_KEY_F10,
        [VK_F11]        = YO_KEY_F11,
        [VK_F12]        = YO_KEY_F12,
    };

    if (vkcode < countof(table)) {
        return table[vkcode];
    } else {
        return 0;
    }
}

static yo_mouse_button win32_mouse_button_from_wm(u32 wm) {
    switch (wm) {
        case WM_LBUTTONUP:      return YO_MOUSE_BUTTON_LEFT;
        case WM_LBUTTONDOWN:    return YO_MOUSE_BUTTON_LEFT;
        case WM_LBUTTONDBLCLK:  return YO_MOUSE_BUTTON_LEFT;
        case WM_RBUTTONUP:      return YO_MOUSE_BUTTON_RIGHT;
        case WM_RBUTTONDOWN:    return YO_MOUSE_BUTTON_RIGHT;
        case WM_RBUTTONDBLCLK:  return YO_MOUSE_BUTTON_RIGHT;
        case WM_MBUTTONUP:      return YO_MOUSE_BUTTON_MIDDLE;
        case WM_MBUTTONDOWN:    return YO_MOUSE_BUTTON_MIDDLE;
        case WM_MBUTTONDBLCLK:  return YO_MOUSE_BUTTON_MIDDLE;
        default:                return 0;
    }
}

static yo_modifiers win32_modifers_get(void) {
    yo_modifiers ret = { 0 };
    if (GetKeyState(VK_CONTROL) & 0x8000) ret |= YO_MODIFIER_CTRL;
    if (GetKeyState(VK_SHIFT)   & 0x8000) ret |= YO_MODIFIER_SHIFT;
    if (GetKeyState(VK_MENU)    & 0x8000) ret |= YO_MODIFIER_ALT;
    return ret;
}

// TODO(rune): yo_mouse_buttons are no longer flags
static yo_mouse_button win32_mouse_buttons_get(void) {
    yo_mouse_button ret = { 0 };
    if (GetKeyState(VK_LBUTTON) & 0x8000) ret |= YO_MOUSE_BUTTON_LEFT;
    if (GetKeyState(VK_RBUTTON) & 0x8000) ret |= YO_MOUSE_BUTTON_RIGHT;
    if (GetKeyState(VK_MBUTTON) & 0x8000) ret |= YO_MOUSE_BUTTON_MIDDLE;
    return ret;
}

static vec2 win32_get_mouse_pos(HWND hwnd) {
    POINT point = { 0 };
    GetCursorPos(&point);
    ScreenToClient(hwnd, &point);
    vec2 ret = vec2((f32)point.x, (f32)point.y);
    return ret;
}

static irect platform_win32_get_client_rect(HWND hwnd) {
    RECT client_rect;
    GetClientRect(hwnd, &client_rect);
    irect ret = {
        .x0 = client_rect.left,
        .y0 = client_rect.top,
        .x1 = client_rect.right,
        .y1 = client_rect.bottom,
    };
    return ret;
}

static LRESULT CALLBACK win32_window_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
    switch (msg) {
        case WM_SIZE: {
            platform_win32 *platform = (platform_win32 *)GetWindowLongPtrW(hwnd, GWLP_USERDATA);

            yo_event resize_event = {
                .kind = YO_EVENT_KIND_RESIZE,
            };
            yo_event_list events = {
                .first = &resize_event,
                .last = &resize_event,
                .count = 1,
            };

            // rune: Re-render at new window size.
            irect client_rect = platform_win32_get_client_rect(hwnd);
            ivec2 client_rect_dim = irect_dim(client_rect);
            platform_update_and_render(platform, client_rect_dim, events);
        } break;

        case WM_DESTROY: {
            PostQuitMessage(0);
            return 0;
        }
    }

    return DefWindowProcW(hwnd, msg, wparam, lparam);
}

static yo_font g_sans_font;

#if 0
static void spotify_like_bottom_bar(ui_id id, ui_area area) {
    vec2 pref_dim = vec2(F32_MAX, 80);
    vec2 dim = ui_dim_from_constraints(area, ui_get_stretch(), pref_dim);

    ui_push_id();
    ui_set_id(id);

    ui_border(ui_make_border(0, rgb(10, 10, 10), 0)) {
        ui_padding(10, 10) {
            ui_layout_x() {
                f32 icon_len = dim.y; // ui_get_box()->avail_dim.x; // TODO(rune): @uiapi some way to query the available area.
                vec2 icon_dim = vec2(icon_len, icon_len);

                ui_area_scope(((ui_area) { .avail = vec2(icon_len, icon_len) })) {
                    ui_set_stretch(UI_STRETCH_XY);
                    ui_fill(rgb(110, 100, 120));
                }
            }
        }
    }

    ui_pop_id();
}

static void ui_debug_draw_atlas(atlas *atlas) {
    vec2 p0 = vec2(0, 500);
    ui_draw_textured(rect_make_dim(p0, vec2((f32)atlas->dim.x, (f32)atlas->dim.y)), rgb(255, 255, 255), rect_make_xyxy(0, 0, 1, 1));

    for_list (atlas_shelf, shelf, atlas->shelves) {
        bool shelf_is_recent = false;

        for_list (atlas_slot, slot, shelf->slots) {
            u32 color = 0;
            if (slot->timestamp + 1 < atlas->current_timestamp) {
                color = rgba(0, 0, 0, 0);
            } else {
                color = rgba(0, 255, 0, 50);
                shelf_is_recent = true;
            }

            rect r = {
                .x0 = p0.x + slot->rect.x0,
                .x1 = p0.x + slot->rect.x1,
                .y0 = p0.y + slot->rect.y0,
                .y1 = p0.y + slot->rect.y1,
            };

            ui_draw_rect(r, color);
        }

        if (shelf_is_recent == false) {
            rect r = {
                .x0 = p0.x,
                .x1 = p0.x + atlas->dim.x,
                .y0 = p0.y + shelf->base_y,
                .y1 = p0.y + shelf->base_y + shelf->dim_y,
            };

            ui_draw_rect(r, rgba(0, 0, 0, 200));
        }

        rect r = {
            .x0 = p0.x,
            .x1 = p0.x + atlas->dim.x,
            .y0 = p0.y + shelf->base_y,
            .y1 = p0.y + shelf->base_y + 1,
        };

        ui_draw_rect(r, rgba(255, 0, 0, 255));
    }
}
#endif

static f32 g_deltatime;

static void platform_update_and_render(platform_win32 *platform, ivec2 client_rect_dim, yo_event_list events) {
    YO_PROFILE_BEGIN(platform_update_and_render);

    YO_PROFILE_BEGIN(platform_update_and_render__input);
    // rune: Input.
    yo_input input = { 0 };
    input.client_rect = rect_from_irect(platform_win32_get_client_rect(platform->hwnd));
    input.events = events;
    input.mouse_pos = win32_get_mouse_pos(platform->hwnd);
    input.mouse_buttons = win32_mouse_buttons_get();


    // rune: Delta time.
    u64 this_tick = GetTickCount64();
    g_deltatime = (f32)(this_tick - platform->prev_tick) / 1000.0f; // TODO(rune): Remove global deltatime.
    platform->prev_tick = this_tick;
    input.delta_time = g_deltatime;
    YO_PROFILE_END(platform_update_and_render__input);

    YO_PROFILE_BEGIN(platform_update_and_render__update);

#if 0
    ui_select_ctx(&platform->ui_ctx);
    ui_next_frame(&input);

    ui_set_face(ui_make_face(global_sans_font, 16));

    atlas_next_timestamp(&platform->atlas);
    global_text_field_style = (ui_text_field_style) {
        .text_color        = rgb(220, 220, 220),
        .text_color_hot    = rgb(220, 220, 220),
        .text_color_active = rgb(255, 255, 255),

        .ghost_color        = rgb(120, 120, 120),
        .ghost_color_hot    = rgb(120, 120, 120),
        .ghost_color_active = rgb(120, 120, 120),

        .border        = ui_make_border(1, rgb(70, 70, 70), 0),
        .border_hot    = ui_make_border(1, rgb(100, 100, 100), 0),
        .border_active = ui_make_border(1, rgb(100, 100, 100), 0),
        .border_bad_value = ui_make_border(1, rgb(170, 90, 90), 0),

        .background        = rgb(50, 50, 50),
        .background_hot    = rgb(40, 40, 40),
        .background_active = rgb(35, 35, 35),

        .padding          = vec2(5, 5),
        .min_width        = 100,
        .max_width        = F32_MAX,
    };
#endif

    yo_frame_begin(&input);
    app_build_ui(input.client_rect);
    yo_frame_end();

#if 0
    string text = str("I am the Walrus!");
    vec2 text_dim = ui_measure_text(text);
    vec2 text_p = vec2(0, 100);
    ui_draw_rect(rect_make_dim(text_p, text_dim), rgb(0, 0, 0));
    ui_draw_text(text_p, text, rgb(255, 255, 255));
    ui_draw_glyph(vec2(200, 200), 'A', rgb(0, 0, 0));
#endif

#if 0 // Build test UI
    ui_box *b = ui_begin_widget(0);
    b->transform = vec2(10, 10);
    ui_draw_glyph(vec2(200, 200), 'A', rgb(150, 0, 0));
    ui_end_widget();

    ui_border(ui_make_border(2, rgb(20, 20, 20), 0)) {
        ui_layout_y() {
            ui_set_area((ui_area) { .avail = vec2(600.0f, F32_MAX) });

            if (ui_button(ui_auto_id(), str("Click me!"))) { println("You clicked me :)"); }

            ui_set_area((ui_area) { .avail = vec2(600.0f, F32_MAX) });
            ui_set_stretch(UI_STRETCH_X);
            ui_border(ui_make_border(20, rgb(0, 200, 100), 1)) {
                ui_border(ui_make_border(20, rgb(0, 100, 200), 1)) {
                    ui_button(ui_auto_id(), str("Click me too!"));
                }
            }

            ui_set_stretch(UI_STRETCH_NONE);
            ui_button(ui_auto_id(), str("Click me plz!"));

            ui_set_area((ui_area) { .avail = vec2(600.0f, F32_MAX) });
            static u32 color;
            ui_color_picker(ui_auto_id(), &color);

            ui_area c = { 0 };
            c.avail.x = 600.0f;
            c.avail.y = F32_MAX;
            ui_set_area(c);

            static f32 slider_value;
            ui_slider_base(ui_auto_id(), &slider_value, UI_AXIS_X);

            ui_text(ui_fmt(slider_value), rgb(200, 200, 200));
#if 0
            for_n (u64, i, 1000) {
                static u8 txt[256];
                static u64 txt_len = 0;
                ui_text_field(ui_id(i + 1), spanof(txt), &txt_len, &global_text_field_style);
            }
#elif 0
            for_n (u64, i, 1000) {
                static u8 txt[256];
                static u64 txt_len = 0;
                ui_button(ui_id(i + 1), str("a"));
            }
#endif

            spotify_like_bottom_bar(ui_id("spotify_bar"), (ui_area) { .avail = VEC2_MAX });
        }
    }

    ui_draw_rect(rect_make_xxyy(100, 200, 300, 400), rgb(255, 255, 0));
    ui_draw_rect(rect_make_xxyy(110, 210, 310, 410), rgb(255, 100, 255));
#endif
    YO_PROFILE_END(platform_update_and_render__update);

    YO_PROFILE_BEGIN(platform_update_and_render__render);
    // TODO(rune): Check need_redraw.
    if (platform->running) {
        r_d3d11_render_data render_data = {
            .atlas_tex    = platform->atlas_tex,
            .atlas        = &platform->atlas,
            .font_backend = &platform->font_backend,
            .root         = platform->state->root,
        };
        r_render(&platform->renderer, client_rect_dim, true, &render_data);
    }
    YO_PROFILE_END(platform_update_and_render__render);

    YO_PROFILE_END(platform_update_and_render);
}

static void platform_win32_run(void) {
    platform_win32 *platform = null;

    // rune: First time initialization
    {
        platform = heap_alloc(sizeof(*platform));
        memset(platform, 0, sizeof(*platform));
        platform->arena = arena_create_default();
        platform->state = yo_state_create();

        yo_state_select(platform->state);

        // rune: Initialize font backend and atlas.
        atlas_init(&platform->atlas, uvec2(512, 512), 4096, platform->arena);
        font_backend_startup(&platform->font_backend, &platform->atlas);
        yo_font_backend font_backend = {
            .userdata       = &platform->font_backend,
            .get_advance    = font_backend_get_advance,
            .get_lineheight = font_backend_get_lineheight,
        };
        platform->state->font_backend = font_backend;

        g_sans_font = font_backend_init_font(&platform->font_backend, str_make(opensans_stripped_regular_data, sizeof(opensans_stripped_regular_data)));
        g_mono_font = font_backend_init_font(&platform->font_backend, str_make(liberation_mono_data, sizeof(liberation_mono_data)));

        // rune: We handle dpi scaling manually.
        SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

        // rune: Register window class.
        WNDCLASSW window_class = { 0 };
        window_class.lpszClassName = L"yo_window_class";
        window_class.lpfnWndProc = &win32_window_proc;
        if (!RegisterClassW(&window_class)) {
            println("RegisterClassA failed (%).", (u32)GetLastError());
            return;
        }

        // rune: Create window.
        platform->hwnd = CreateWindowExW(0, L"yo_window_class", L"Yo", WS_OVERLAPPEDWINDOW,
                                         CW_USEDEFAULT, CW_USEDEFAULT, 800, 600,
                                         0, 0, 0, 0);

        if (!platform->hwnd) {
            println("CreateWindowExW failed (%).", (u32)GetLastError());
            return;
        }

        // rune: Initialize D3D11.
        r_startup(&platform->renderer, platform->hwnd);
        platform->atlas_tex = r_create_texture(&platform->renderer, platform->atlas.dim, R_TEX_FORMAT_R8);

        // rune: Store pointer to platform struct on hwnd, so we can access it in the wndproc.
        SetWindowLongPtrW(platform->hwnd, GWLP_USERDATA, (LONG_PTR)platform);

        // rune: Show window.
        ShowWindow(platform->hwnd, SW_SHOW);
        platform->first_tick = GetTickCount64();
        platform->prev_tick = platform->first_tick;
        platform->running =  true;
    }

    // rune: Main loop.
    while (platform->running) {
        YO_PROFILE_FRAME_MARK();

        // rune: Poll win32 messages.
        yo_event_list events = { 0 };
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
            switch (msg.message) {
                case WM_SYSKEYDOWN:
                case WM_KEYDOWN: {
                    yo_event *e  = yo_event_list_push(&events, platform->arena); // TODO(rune): Frame arena
                    e->kind      = YO_EVENT_KIND_KEY_PRESS;
                    e->mods      = win32_modifers_get();
                    e->key       = win32_key_from_vkcode((u32)wparam);
                    e->codepoint = 0;

                    if (e->key == YO_KEY_ESCAPE) {
                        platform->running = false; // TODO(rune): Should not be part of the platform layer.
                    }
                } break;

                case WM_SYSKEYUP:
                case WM_KEYUP: {
                    yo_event *e  = yo_event_list_push(&events, platform->arena); // TODO(rune): Frame arena
                    e->kind      = YO_EVENT_KIND_KEY_RELEASE;
                    e->mods      = win32_modifers_get();
                    e->key       = win32_key_from_vkcode((u32)wparam);
                    e->codepoint = 0;
                } break;

                case WM_CHAR: {
                    if (wparam >= 32 && wparam != 127) {
                        yo_event *e  = yo_event_list_push(&events, platform->arena); // TODO(rune): Frame arena
                        e->kind      = YO_EVENT_KIND_CODEPOINT;
                        e->mods      = win32_modifers_get();
                        e->codepoint = (u32)wparam;
                    }
                } break;

                case WM_LBUTTONDOWN:
                    // NOTE(rune): Tell windows that we still want mouse-move messages,
                    // when the user is dragging outside our own window.
                    SetCapture(platform->hwnd);
                case WM_RBUTTONDOWN:
                case WM_MBUTTONDOWN: {
                    yo_event *e     = yo_event_list_push(&events, platform->arena); // TODO(rune): Frame arena
                    e->kind         = YO_EVENT_KIND_MOUSE_PRESS;
                    e->mods         = win32_modifers_get();
                    e->mouse_button = win32_mouse_button_from_wm(msg.message);
                    e->pos.x        = (f32)LOWORD(lparam);
                    e->pos.y        = (f32)HIWORD(lparam);
                } break;

                case WM_LBUTTONUP:
                    ReleaseCapture();
                case WM_RBUTTONUP:
                case WM_MBUTTONUP: {
                    // TODO(rune): Currently the YO_EVENT_KIND_MOUSE_UP event is only used for
                    // triggering af ui rebuild.
                    yo_event *e     = yo_event_list_push(&events, platform->arena); // TODO(rune): Frame arena
                    e->kind         = YO_EVENT_KIND_MOUSE_RELEASE;
                    e->mouse_button = win32_mouse_button_from_wm(msg.message);
                } break;

                case WM_MOUSEWHEEL: {
                    yo_event *e  = yo_event_list_push(&events, platform->arena); // TODO(rune): Frame arena
                    e->kind      = YO_EVENT_KIND_SCROLL;
                    e->mods      = win32_modifers_get();
                    e->scroll.x  = 0.0f; // TODO(rune): Horizontal scroll.
                    e->scroll.y  = -(f32)(i16)HIWORD(msg.wParam) / 120.0f;
                } break;

#if 0
                case WM_MOUSEMOVE: {
                    // TODO(rune): Currently the YO_EVENT_KIND_MOUSE_MOVE event is only used for
                    // triggering af ui rebuild.
                    yo_event *e  = yo_event_list_push(&events, platform->arena); // TODO(rune): Frame arena
                    e->kind      = YO_EVENT_KIND_MOUSE_MOVE;
                } break;
#endif
            }
        }

#if 0
        // rune: Mouse.
        input.mouse_buttons = win32_get_mouse_buttons();
        input.mouse_pos = win32_get_mouse_pos(platform->hwnd);
#endif
        // rune: Window size.
        irect client_rect = platform_win32_get_client_rect(platform->hwnd);
        ivec2 client_rect_dim = irect_dim(client_rect);

#if 0
        // rune: Deltatime.
        u64 this_tick = GetTickCount64();
        input.deltatime = (f32)(this_tick - platform->prev_tick) / 1000.0f;
        input.tick = this_tick - platform->first_tick;
        platform->prev_tick = this_tick;
#endif

        // rune: Update.
        platform_update_and_render(platform, client_rect_dim, events);
    }
}
