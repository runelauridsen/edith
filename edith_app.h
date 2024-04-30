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
    yo_event_list events;
    vec2 mouse_pos;
    bool mouse_buttons[YO_MOUSE_BUTTON_COUNT];
    bool window_has_focus;
};

// NOTE(rune): Output from app -> platform
typedef struct edith_app_output edith_app_output;
struct edith_app_output {
    yo_node *root;
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
    f32 active_t;
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
    r_d3d11_tex atlas_tex;

    yo_font mono_font;
    yo_font sans_font;
    yo_face editor_face;

    edith_app_callbacks callbacks;

    font_backend font_backend;

    edith_editor_tab_list tabs;
    edith_editor_tab *first_free_editor_tab;

    edith_pane left_pane;
    edith_pane right_pane;
    edith_pane *active_pane;

    edith_clipboard clipboard;

    edith_app_state state;

    edith_perf_timings frame_timing;
    edith_perf_timings build_ui_timing;
    edith_perf_timings draw_ui_ops_timing;

    r_d3d11_tex title_tex;
    bool window_has_focus;

    r_state renderer;

    yo_state *yo_state;

    bool stop;
};

static edith_app_output edith_app_update_and_render(edith_app *app, edith_app_input input);
