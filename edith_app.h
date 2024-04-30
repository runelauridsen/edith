////////////////////////////////////////////////////////////////
// rune: Performance timing

typedef struct edith_perf_timings edith_perf_timings;
struct edith_perf_timings {
    f64 samples[60];
    i64 samples_count;
    i64 rolling_idx;

    f64 min;
    f64 max;
    f64 avg;

    u64 t_begin;
    u64 t_end;
};

////////////////////////////////////////////////////////////////
// rune: App

// NOTE(rune): Input from platform -> app
typedef struct edith_app_input edith_app_input;
struct edith_app_input {
    rect client_rect;
    f32 deltatime;
    i64 tick;
    ui_events events;
    vec2 mouse_pos;
    ui_mouse_buttons mouse_buttons;
    bool window_has_focus;
};

// NOTE(rune): Output from app -> platform
typedef struct edith_app_output edith_app_output;
struct edith_app_output {
    r_pass_list render_passes;
    bool need_redraw;
};

typedef enum edith_app_state {
    EDITH_APP_STATE_DEFAULT,
    EDITH_APP_STATE_OPEN_FILE,
    EDITH_APP_STATE_TAB_SWITCHER,
    EDITH_APP_STATE_FIND,
    EDITH_APP_STATE_FIND_ALL,
    EDITH_APP_STATE_GOTO_LINE,
    EDITH_APP_STATE_COMMAND_PALLETTE,
} edith_app_state;

typedef struct edith_pane edith_pane;
struct edith_pane {
    edith_editor_tab *tab;
    ui_animated_f32 active_t;
};

typedef struct edith_app_callbacks edith_app_callbacks;
struct edith_app_callbacks {
    void    (*set_clipboard)(str data);
    str_    (*get_clipboard)(arena *arena);
    bool    (*clipboard_changed_externally)(void); // NOTE(rune): Returns true if clipboard changed since last set_clipboard()
};

typedef struct edith_app edith_app;
struct edith_app {
    bool initialized;
    arena *perm;
    arena *temp;
    atlas atlas;
    ui_font mono_font;
    ui_font sans_font;
    ui_face editor_face;

    edith_app_callbacks callbacks;

    draw_ctx draw_ctx;

    struct {
        edith_dialog_open_file_data   dialog_open_file;
        edith_dialog_find_data        dialog_find;
        edith_dialog_goto_line_data   dialog_goto_line;
        edith_dialog_commands_data    dialog_commands;

        ui_ctx ui_ctx;

        ui_toast ui_toast_storage[64];
        ui_toast_ctx ui_toast_ctx;

        bool window_has_focus; // TODO(rune): Cleanup
    };

    font_backend_stb font_backend;

    edith_editor_tab_list tabs;
    edith_editor_tab *first_free_editor_tab;

    edith_pane left_pane;
    edith_pane right_pane;
    edith_pane *active_pane;

    edith_clipboard clipboard;

    edith_app_state state;

    darray(r_rect_instance) ui_rect_storage;
    darray(r_rect_instance) misc_rect_storage;

    i64 last_frame_rect_instance_count;

    edith_perf_timings frame_timing;
    edith_perf_timings build_ui_timing;
    edith_perf_timings draw_ui_ops_timing;

    bool stop;
};

static edith_app_output edith_app_update_and_render(edith_app *app, edith_app_input input);
