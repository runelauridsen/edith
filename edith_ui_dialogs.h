////////////////////////////////////////////////////////////////
// rune: Common

typedef struct theme theme;
struct theme {
    u32 bg_main;
    u32 bg_hover;
    u32 bg_hover1;
    u32 bg_active;
    u32 bg_border;
    u32 fg_main;
    u32 fg_inactive;
    u32 fg_active;
    f32 roundness;

    u32 button_bg;
    u32 button_bg_hot;
    u32 button_bg_active;

    u32 button_fg;
    u32 button_fg_hot;
    u32 button_fg_active;

    u32 button_bc;
    u32 button_bc_hot;
    u32 button_bc_active;

    f32 button_border_thickness;
    f32 button_border_radius;

    u32 bg_bad;
};

static ui_font global_mono_font;
static ui_font global_sans_font;

static theme default_theme(void);
static rect calc_popup_area(rect client_rect);
static void cut_and_draw_popup_frame(rect *area);

typedef struct field_frame field_frame;
struct field_frame {
    vec2 content_pref_dim;
    f32 padding;
    f32 margin;
    bool bad_value;
};

static rect cut_and_draw_field_frame(rect *area, field_frame *frame);
static void ui_popup_frame_box(void);

#define ui_popup_frame()   defer((ui_popup_frame_box(), ui_children_begin()), ui_children_end())

static void ui_fuzzy_matched_string(str s, fuzzy_match_list matches);

////////////////////////////////////////////////////////////////
// rune: Find incremental dialog

typedef struct edith_dialog_find_data edith_dialog_find_data;
struct edith_dialog_find_data  {
    u8 text_buf[kilobytes(1)];
    i64 text_len;
    bool case_sensitive;
    bool has_moved_to_result;
};

static void edith_dialog_find(rect r, edith_dialog_find_data *data, edith_textview *tv);

////////////////////////////////////////////////////////////////
// rune: Goto line dialog

typedef struct edith_dialog_goto_line_data edith_dialog_goto_line_data;
struct edith_dialog_goto_line_data {
    u8 text_buf[kilobytes(1)];
    i64 text_len;
};

static void edith_dialog_goto_line(rect r, edith_dialog_goto_line_data *data);

////////////////////////////////////////////////////////////////
// rune: Commands dialog

typedef struct dialog_commands_data edith_dialog_commands_data;
struct dialog_commands_data {
    u8 text_buf[kilobytes(1)];
    i64 text_len;

    i64 selected_idx;
};

typedef struct dialog_command_item dialog_command_item;
struct dialog_command_item {
    edith_cmd_spec *spec;
    fuzzy_match_list fuzzy_matches;
};

typedef struct dialog_command_node dialog_command_node;
struct dialog_command_node {
    dialog_command_item v;
    dialog_command_node *next;
};

typedef struct dialog_command_list dialog_command_list;
struct dialog_command_list {
    dialog_command_node *first;
    dialog_command_node *last;
    u64 count;
};

static void edith_dialog_commands(rect client_rect, edith_dialog_commands_data *data);

////////////////////////////////////////////////////////////////
// rune: Tab switcher

static str  str_chop_file_name(str *s);
static void ui_cut_and_draw_border(rect *r, ui_border border, u32 color);
static void edith_dialog_tabswitcher(ui_id id, rect client_rect, edith_editor_tab_list tabs, edith_editor_tab *active_tab);

////////////////////////////////////////////////////////////////
// rune: Open file dialog

typedef enum lister_item_type {
    UI_LISTER_ITEM_TYPE_FILE,
    UI_LISTER_ITEM_TYPE_DIRECTORY,
    UI_LISTER_ITEM_TYPE_DRIVE,
} lister_item_type;

typedef struct dialog_open_file_item dialog_open_file_item;
struct dialog_open_file_item {
    str string;
    lister_item_type type;
    i32 match_idx;
};

typedef struct dialog_open_file_data edith_dialog_open_file_data;
struct dialog_open_file_data {
    bool init;

    arena *scratch; // TODO(rune): Does it really need it's own arena?

    dialog_open_file_item items[256];
    i64 item_count;

    u8 path_buf[kilobytes(1)]; // NOTE(rune): Current path.
    u8 name_buf[kilobytes(1)]; // NOTE(rune): text_field input buffer.

    i64 path_len;
    i64 name_len;

    f32 target_scroll;
    ui_animated_f32 animated_scroll;

    bool need_refresh;
    bool should_reset_caret;
};

static void edith_dialog_open_file_refresh(edith_dialog_open_file_data *data);
static void edith_dialog_open_file_init(edith_dialog_open_file_data *data);
static void edith_dialog_open_file(rect area, ui_id id, edith_dialog_open_file_data *data, bool is_active, ui_events events);
