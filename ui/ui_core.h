////////////////////////////////////////////////////////////////
// rune: References

// Casey Muratori: Immediate-mode Graphical User Interfaces
// https://caseymuratori.com/blog_0001
//
// Ryan Fluery: UI Series
// https://www.rfleury.com/p/ui-series-table-of-contents
//
// Halt: RectCut for dead simple UI layouts
// https://halt.software/dead-simple-layouts/

////////////////////////////////////////////////////////////////
// rune: Events

//#define APP_DOES_SCISSOR_INTERSECT_AND_TRANSFORM_ADD

typedef enum ui_event_type {
    UI_EVENT_TYPE_NONE,
    UI_EVENT_TYPE_CLICK,
    UI_EVENT_TYPE_KEYPRESS,
    UI_EVENT_TYPE_KEY_RELEASE,
    UI_EVENT_TYPE_CODEPOINT,
    UI_EVENT_TYPE_SCROLL,
    UI_EVENT_TYPE_MOUSE_MOVE,
    UI_EVENT_TYPE_MOUSE_UP,
    UI_EVENT_TYPE_MOUSE_DOWN,
    UI_EVENT_TYPE_RESIZE,
    UI_EVENT_TYPE_COUNT,
} ui_event_type;

typedef enum ui_modifiers {
    UI_MODIFIER_NONE  = 0,
    UI_MODIFIER_SHIFT = 1,
    UI_MODIFIER_CTRL  = 2,
    UI_MODIFIER_ALT   = 4,
} ui_modifiers;

typedef enum ui_mouse_buttons {
    UI_MOUSE_BUTTON_NONE   = 0,
    UI_MOUSE_BUTTON_LEFT   = 1,
    UI_MOUSE_BUTTON_RIGHT  = 2,
    UI_MOUSE_BUTTON_MIDDLE = 4,
} ui_mouse_buttons;

typedef enum ui_key {
    UI_KEY_NONE = 0,

    UI_KEY_A = 'A',
    UI_KEY_B = 'B',
    UI_KEY_C = 'C',
    UI_KEY_D = 'D',
    UI_KEY_E = 'E',
    UI_KEY_F = 'F',
    UI_KEY_G = 'G',
    UI_KEY_H = 'H',
    UI_KEY_I = 'I',
    UI_KEY_J = 'J',
    UI_KEY_K = 'K',
    UI_KEY_L = 'L',
    UI_KEY_M = 'M',
    UI_KEY_N = 'N',
    UI_KEY_O = 'O',
    UI_KEY_P = 'P',
    UI_KEY_Q = 'Q',
    UI_KEY_R = 'R',
    UI_KEY_S = 'S',
    UI_KEY_T = 'T',
    UI_KEY_U = 'U',
    UI_KEY_V = 'V',
    UI_KEY_W = 'W',
    UI_KEY_X = 'X',
    UI_KEY_Y = 'Y',
    UI_KEY_Z = 'Z',

    UI_KEY_SPACE = ' ',
    UI_KEY_TAB = '\t',
    UI_KEY_ENTER = '\n',

    UI_KEY_PERIOD = '.',
    UI_KEY_COMMA = ';',
    UI_KEY_MINUS = '-',
    UI_KEY_PLUS = '=',

    UI_KEY_SHIFT = 256,
    UI_KEY_CTRL,
    UI_KEY_ALT,
    UI_KEY_LEFT,
    UI_KEY_RIGHT,
    UI_KEY_UP,
    UI_KEY_DOWN,
    UI_KEY_PAGE_UP,
    UI_KEY_PAGE_DOWN,
    UI_KEY_HOME,
    UI_KEY_END,
    UI_KEY_INSERT,
    UI_KEY_DELETE,
    UI_KEY_BACKSPACE,
    UI_KEY_ESCAPE,

    UI_KEY_F1,
    UI_KEY_F2,
    UI_KEY_F3,
    UI_KEY_F4,
    UI_KEY_F5,
    UI_KEY_F6,
    UI_KEY_F7,
    UI_KEY_F8,
    UI_KEY_F9,
    UI_KEY_F10,
    UI_KEY_F11,
    UI_KEY_F12,

    UI_KEY_COUNT,
} ui_key;

typedef struct ui_event ui_event;
struct ui_event {
    ui_event_type type;
    ui_modifiers modifiers;
    ui_mouse_buttons mousebutton;
    ui_key key;
    vec2 pos;
    vec2 scroll;
    u32 codepoint;
};

typedef struct ui_events ui_events;
struct ui_events {
    ui_event *v;
    u64 count;
};

static ui_events ui_empty_events(void);
static bool ui_event_is_keypress(ui_event *e, ui_key key);
static bool ui_event_is_keypress_with_modifiers(ui_event *e, ui_key key, ui_modifiers modifiers);

////////////////////////////////////////////////////////////////
// rune: Fonts

// NOTE(rune): We limit the number of fonts and fontsize to u16,
// so they can fit together with a u32 codepoint in a single u64.

typedef struct ui_font ui_font;
struct ui_font {
    u16 u16;
};

typedef union ui_face ui_face;
union ui_face {
    struct { ui_font font; u16 fontsize; };
    struct { u16 u16[2]; };
    struct { u32 u32; };
};

typedef f32 ui_get_advance_fn(void *userdata, ui_face face, u32 glyph);
typedef f32 ui_get_lineheight_fn(void *userdata, ui_face face);

typedef struct ui_font_backend ui_font_backend;
struct ui_font_backend {
    void *userdata;
    ui_get_advance_fn *get_advance;
    ui_get_lineheight_fn *get_lineheight;
};

////////////////////////////////////////////////////////////////
// rune: Commands

typedef struct ui_cmd ui_cmd;
struct ui_cmd {
    i32 tag;
    void *data;
    ui_cmd *next;
};

typedef struct ui_cmds ui_cmds;
struct ui_cmds {
    ui_cmd *first;
    ui_cmd *last;
    i32 count;
};

////////////////////////////////////////////////////////////////
// rune: Ops

typedef enum ui_op_type {
    UI_OP_TYPE_NONE,
    UI_OP_TYPE_GLYPH,
    UI_OP_TYPE_TEXT,
    UI_OP_TYPE_AABB,
    UI_OP_TYPE_AABB_EX,
    UI_OP_TYPE_QUAD,
    UI_OP_TYPE_SCISSOR,
    UI_OP_TYPE_PUSH_SCISSOR,
    UI_OP_TYPE_POP_SCISSOR,
    UI_OP_TYPE_TRANSFORM,
    UI_OP_TYPE_PUSH_TRANSFORM,
    UI_OP_TYPE_POP_TRANSFORM,
} ui_op_type;

typedef struct ui_op_glyph ui_op_glyph;
struct ui_op_glyph {
    u32 color;
    ui_face face;
    u32 codepoint;
    vec2 p;
};

typedef struct ui_op_text ui_op_text;
struct ui_op_text {
    u32 color;
    ui_face face;
    vec2 p;
    str s;
};

typedef struct ui_op_aabb ui_op_aabb;
struct ui_op_aabb {
    u32 color;
    rect rect;
};

typedef struct ui_op_aabb_ex ui_op_aabb_ex;
struct ui_op_aabb_ex {
    u32 color[4];
    f32 corner_radius[4];

    rect dst_rect;
    rect tex_rect;

    f32  softness;
    f32  tex_weight;
};

typedef struct ui_op_quad ui_op_quad;
struct ui_op_quad {
    vec4 p[4];
    u32 color[4];
};

typedef enum ui_scissor_flags {
    UI_SCISSOR_FLAG_NO_INTERSECT = 1,
} ui_scissor_flags;

typedef struct ui_op_set_scissor ui_op_scissor;
struct ui_op_set_scissor {
    rect rect;
    ui_scissor_flags flags;
};

typedef enum ui_transform_flags {
    UI_TRANSFORM_FLAG_NO_RELATIVE = 1,
} ui_transform_flags;

typedef struct ui_op_set_transform ui_op_set_transform;
struct ui_op_set_transform {
    rect rect;
    ui_scissor_flags flags;
};

typedef struct ui_op_transform ui_op_transform;
struct ui_op_transform {
    vec2 vec;
    ui_transform_flags flags;
};

typedef struct ui_op_def ui_op_def;
struct ui_op_def {
    ui_op_type type;
    u32 data_offset;
};

typedef struct ui_ops ui_ops;
struct ui_ops {
    ui_op_def *defs;
    u32 def_count;

    u8 *data;
    u64 data_size;
};

////////////////////////////////////////////////////////////////
// rune: Id

typedef u64 ui_id;

typedef enum ui_interations {
    UI_INTERACTION_NONE           = 0,
    UI_INTERACTION_HOT_HOVER      = 1,
    UI_INTERACTION_ACTIVATE_CLICK = 2,
    UI_INTERACTION_ACTIVATE_HOLD  = 4,
} ui_interactions;

////////////////////////////////////////////////////////////////
// rune: Layout

typedef u8 ui_axis_align;
enum ui_axis_align {
    UI_AXIS_ALIGN_STRETCH,
    UI_AXIS_ALIGN_CENTER,
    UI_AXIS_ALIGN_MIN,
    UI_AXIS_ALIGN_MAX,
};

typedef struct ui_align ui_align;
struct ui_align {
    ui_axis_align x : 2;
    ui_axis_align y : 2;
};

#define UI_ALIGN_NONE       UI_ALIGN_STRETCH

#define UI_ALIGN_LEFT           ((ui_align) { UI_AXIS_ALIGN_MIN,     UI_AXIS_ALIGN_STRETCH })
#define UI_ALIGN_RIGHT          ((ui_align) { UI_AXIS_ALIGN_MAX,     UI_AXIS_ALIGN_STRETCH })
#define UI_ALIGN_TOP            ((ui_align) { UI_AXIS_ALIGN_STRETCH, UI_AXIS_ALIGN_MIN })
#define UI_ALIGN_BOT            ((ui_align) { UI_AXIS_ALIGN_STRETCH, UI_AXIS_ALIGN_MAX })
#define UI_ALIGN_CENTER         ((ui_align) { UI_AXIS_ALIGN_CENTER,  UI_AXIS_ALIGN_CENTER })
#define UI_ALIGN_STRETCH        ((ui_align) { UI_AXIS_ALIGN_STRETCH, UI_AXIS_ALIGN_STRETCH })

#define UI_ALIGN_LEFT_CENTER    ((ui_align) { UI_AXIS_ALIGN_MIN,     UI_AXIS_ALIGN_CENTER })
#define UI_ALIGN_RIGHT_CENTER   ((ui_align) { UI_AXIS_ALIGN_MAX,     UI_AXIS_ALIGN_CENTER })

#define UI_ALIGN_TOP_LEFT       ((ui_align) { UI_AXIS_ALIGN_MIN,     UI_AXIS_ALIGN_MIN })
#define UI_ALIGN_TOP_RIGHT      ((ui_align) { UI_AXIS_ALIGN_MAX,     UI_AXIS_ALIGN_MIN })
#define UI_ALIGN_TOP_CENTER     ((ui_align) { UI_AXIS_ALIGN_CENTER,  UI_AXIS_ALIGN_MIN })
#define UI_ALIGN_BOT_LEFT       ((ui_align) { UI_AXIS_ALIGN_MIN,     UI_AXIS_ALIGN_MAX })
#define UI_ALIGN_BOT_RIGHT      ((ui_align) { UI_AXIS_ALIGN_MAX,     UI_AXIS_ALIGN_MAX })
#define UI_ALIGN_BOT_CENTER     ((ui_align) { UI_AXIS_ALIGN_CENTER,  UI_AXIS_ALIGN_MAX })

typedef enum ui_cut {
    UI_CUT_NONE,
    UI_CUT_LEFT,
    UI_CUT_RIGHT,
    UI_CUT_TOP,
    UI_CUT_BOT,
} ui_cut;

typedef struct ui_layout ui_layout;
struct ui_layout {
    rect rect;
    ui_cut cut;
    ui_align align;
    f32 spacing;
};

typedef struct ui_pushed_layout ui_pushed_layout;
struct ui_pushed_layout {
    ui_layout v;
    bool next_auto_spacing;
};

typedef union ui_sides_f32 ui_sides_f32;
union ui_sides_f32 {
    struct { f32 v[4]; };
    struct { f32 left, top, right, bot; };
    struct { f32 x0, y0, x1, y1; };
    struct { vec2 leading, trailing; };
    struct { vec2 p0, p1; };
};

typedef struct ui_border ui_border;
struct ui_border {
    ui_sides_f32 radius;
    ui_sides_f32 thickness;
    u32 color;
};

typedef enum ui_text_align {
    UI_TEXT_ALIGN_LEFT,
    UI_TEXT_ALIGN_RIGHT,
    UI_TEXT_ALIGN_CENTER,
    UI_TEXT_ALIGN_JUSTIFY,
} ui_text_align;

////////////////////////////////////////////////////////////////
// rune: Animation

typedef struct ui_animated_f32 ui_animated_f32;
struct ui_animated_f32 {
    f32 pos;
    f32 vel;
    bool started;
    bool completed;
};

typedef struct ui_animated_vec2 ui_animated_vec2;
struct ui_animated_vec2 {
    vec2 pos;
    vec2 vel;
    bool started;
    bool completed;
};

typedef struct ui_animated_vec3 ui_animated_vec3;
struct ui_animated_vec3 {
    vec3 pos;
    vec3 vel;
    bool started;
    bool completed;
};

typedef struct ui_animated_vec4 ui_animated_vec4;
struct ui_animated_vec4 {
    vec4 pos;
    vec4 vel;
    bool started;
    bool completed;
};

typedef struct ui_animated_rect ui_animated_rect;
struct ui_animated_rect {
    rect pos;
    rect vel;
    bool started;
    bool completed;
};

////////////////////////////////////////////////////////////////
// rune: Box

#include "../edith_ui_box.h" // TODO(rune): This depencecy can be removed, once the UI rewrite is complete

////////////////////////////////////////////////////////////////
// rune: Context

// Externally chainged hash table rebuilt every frame.
typedef struct ui_map ui_map;
struct ui_map {
    ui_box **slots;
    u64 slot_count;
};

typedef struct ui_input ui_input;
struct ui_input {
    vec2 mouse_pos;
    ui_mouse_buttons mouse_buttons;
    ui_events events;
    rect client_rect;
};

typedef struct ui_frame ui_frame;
struct ui_frame {
    arena *arena;
    ui_map map;

    ui_id           hot_id;
    ui_interactions hot_id_interactions;

    ui_id           active_id;
    ui_interactions active_id_interactions;
    bool            active_id_touched;

    ui_id           focused_id;

    ui_input input;

    ui_mouse_buttons is_down_mouse_buttons;
    ui_mouse_buttons went_down_mouse_buttons;
    ui_mouse_buttons went_up_mouse_buttons;
};

#define T rect
#include "ui_stack_template.h"
#define T ui_face
#include "ui_stack_template.h"
#define T u32
#include "ui_stack_template.h"
#define T b8
#include "ui_stack_template.h"
#define T ui_pushed_layout
#include "ui_stack_template.h"
#define T vec2
#include "ui_stack_template.h"
#define T ui_box_ptr
#include "ui_stack_template.h"

typedef struct ui_ctx ui_ctx;
struct ui_ctx {
    u64 frame_idx;
    ui_frame this_frame;
    ui_frame prev_frame;
    f32 deltatime;

    ui_cmds cmds;

    buf op_def_buf; // Array of ui_op_def structs.
    buf op_data_buf; // Buffer for ui_op_glyph, ui_op_rect, ... structs.
    u32 op_scissor_offset; // Offset into op_data_buf of current scissor rect
    f32 current_lineheight; // Caches lineheight between calls to ui_set_face().
    f32 current_advance_ascii[128]; // Caches advance of ascii codepoints between calls to ui_set_face().

    ui_font_backend font_backend;

    struct {
        ui_stack(ui_face) face_stack;
        ui_stack(u32) foreground_stack;
        ui_stack(u32) background_stack;
        ui_stack(b8) pixel_snap_stack;
        ui_stack(ui_pushed_layout) layout_stack;
        ui_stack(rect) scissor_stack;
        ui_stack(vec2) transform_stack;
        ui_stack(ui_box_ptr) box_parent_stack;
    };

    ui_box root_box;
    ui_box *selected_box;

    ui_box *current_focus_group;

    bool invalidate_next_frame;
};

////////////////////////////////////////////////////////////////
// rune: Context

static void ui_free_ctx(ui_ctx ctx);
static void ui_select_ctx(ui_ctx *ctx);
static void ui_next_frame(ui_input *input);

static f32 ui_dt(void); // NOTE(rune): Frame delta time

////////////////////////////////////////////////////////////////
// rune: Id

#define ui_hash_val(val) (_Generic((val),    \
        i8                  : ui_hash_i8,               \
        i16                 : ui_hash_i16,              \
        i64                 : ui_hash_i64,              \
        i32                 : ui_hash_i32,              \
        u8                  : ui_hash_u8,               \
        u16                 : ui_hash_u16,              \
        u32                 : ui_hash_u32,              \
        u64                 : ui_hash_u64,              \
        f32                 : ui_hash_f32,              \
        f64                 : ui_hash_f64,              \
        bool                : ui_hash_bool,             \
        str                 : ui_hash_str,              \
        char *              : ui_hash_cstr,             \
        default             : ui_hash_ptr               \
))((val))

#define ui_hash_(...)      VA_WRAP(ui_hash_val, __VA_ARGS__)
#define ui_hash(...)       VA_AGGREGATE(ui_id_combine, ui_hash_(__VA_ARGS__))

#define ui_id(...) ui_hash(__VA_ARGS__)
static ui_id ui_hash_str(str s);
static ui_id ui_hash_args(args args);

////////////////////////////////////////////////////////////////
// rune: Memory

static arena *ui_arena(void); // NOTE(rune): Is reset in ui_next_frame().

////////////////////////////////////////////////////////////////
// rune: Layout

static rect ui_align_rect(rect avail, vec2 pref_dim, ui_align align);
static rect ui_place_rect(vec2 pref_dim);
static rect ui_place_border(ui_border border, vec2 content_pref_dim);
static rect ui_place_spacing(f32 amount);

static ui_layout ui_make_layout_stack(rect area, ui_align align);
static ui_layout ui_make_layout_x(rect area, ui_align align, f32 spacing);
static ui_layout ui_make_layout_y(rect area, ui_align align, f32 spacing);

////////////////////////////////////////////////////////////////
// rune: Units

static f32 ui_px(f32 v); // Device independent pixels.
static f32 ui_dpx(f32 v); // Device pixels.
static f32 ui_em(f32 v);

////////////////////////////////////////////////////////////////
// rune: Rect cut

static rect ui_cut_left(rect *r, f32 a);
static rect ui_cut_right(rect *r, f32 a);
static rect ui_cut_top(rect *r, f32 a);
static rect ui_cut_bot(rect *r, f32 a);
static rect ui_cut_dir(rect *r, f32 a, dir2 dir);
static void ui_cut_all(rect *r, f32 a);
static void ui_cut_x(rect *r, f32 a);
static void ui_cut_y(rect *r, f32 a);
static void ui_cut_xy(rect *r, f32 x, f32 y);
static void ui_cut_vec2(rect *r, vec2 a);
static void ui_cut_a(rect *r, f32 a, axis2 axis);
static void ui_cut_sides(rect *r, f32 left, f32 right, f32 top, f32 bot);

static void ui_cut_centered_x(rect *r, f32 pref_dim_x);
static void ui_cut_centered_y(rect *r, f32 pref_dim_y);
static void ui_cut_centered_a(rect *r, f32 pref_dim_a, axis2 axis);
static void ui_cut_centered_xy(rect *r, f32 pref_dim_x, f32 pref_dim_y);
static void ui_cut_centered(rect *r, vec2 pref_dim);
static void ui_cut_aligned(rect *r, vec2 pref_dim, ui_align align);
static void ui_cut_border(rect *r, ui_border border);

static void ui_add_left(rect *r, f32 a);
static void ui_add_right(rect *r, f32 a);
static void ui_add_top(rect *r, f32 a);
static void ui_add_bot(rect *r, f32 a);
static void ui_add_dir(rect *r, f32 a, dir2 dir);
static void ui_add_all(rect *r, f32 a);
static void ui_add_x(rect *r, f32 a);
static void ui_add_y(rect *r, f32 a);
static void ui_add_axis(rect *r, f32 a, axis2 axis);
static void ui_add_sides(rect *r, f32 left, f32 right, f32 top, f32 bot);

static void ui_max_left(rect *r, f32 a);
static void ui_max_right(rect *r, f32 a);
static void ui_max_top(rect *r, f32 a);
static void ui_max_bot(rect *r, f32 a);
static void ui_max_dir(rect *r, f32 a, dir2 dir);

static rect ui_get_left(rect *r, f32 a);
static rect ui_get_right(rect *r, f32 a);
static rect ui_get_top(rect *r, f32 a);
static rect ui_get_bot(rect *r, f32 a);
static rect ui_get_dir(rect *r, f32 a, dir2 dir);

////////////////////////////////////////////////////////////////
// rune: Stacks

static ui_face ui_get_face(void);
static void ui_set_face(ui_face v);
static void ui_push_face(void);
static void ui_pop_face(void);
#define ui_face_scope(v) defer((ui_push_face(), ui_set_face(v)), ui_pop_face())

static bool ui_get_pixel_snap(void);
static void ui_set_pixel_snap(bool v);
static void ui_push_pixel_snap(void);
static void ui_pop_pixel_snap(void);
#define ui_pixel_snap_scope(v) defer((ui_push_pixel_snap(), ui_set_pixel_snap(v)), ui_pop_pixel_snap())

static ui_layout ui_get_layout(void);
static void ui_set_layout(ui_layout v);
static void ui_push_layout(void);
static void ui_pop_layout(void);
#define ui_layout_scope(v) defer((ui_push_layout(), ui_set_layout(v)), ui_pop_layout())

static rect ui_get_scissor(void);
static void ui_set_scissor(rect v, ui_scissor_flags flags);
static void ui_push_scissor(void);
static void ui_pop_scissor(void);
#define ui_scissor_scope(v, flags) defer((ui_push_scissor(), ui_set_scissor(v, flags)), ui_pop_scissor())

static vec2 ui_get_transform(void);
static ui_op_transform *ui_set_transform(vec2 v, ui_transform_flags flags);
static void ui_push_transform(void);
static void ui_pop_transform(void);
#define ui_transform_scope(v, flags) defer((ui_push_transform(), ui_set_transform(v, flags)), ui_pop_transform())

////////////////////////////////////////////////////////////////
// rune: Draw

static void ui_draw_rect(rect r, u32 color);
static void ui_draw_gradient(rect dst, u32 color0, u32 color1, u32 color2, u32 color3);
static void ui_draw_soft_rect(rect r, u32 color, f32 softness);
static void ui_draw_rounded_rect(rect r, u32 color, f32 roundness);
static void ui_draw_textured(rect r, u32 color, rect uv);

static void ui_draw_shadow(rect r, f32 shadow_radius, f32 shadow_softness);
static void ui_draw_rounded_shadow(rect r, f32 shadow_radius, f32 roundness, f32 shadow_softness);
static void ui_draw_rect_with_shadow(rect r, u32 color, f32 shadow_radius, f32 shadow_softness);
static void ui_draw_rounded_rect_with_shadow(rect r, u32 color, f32 roundness, f32 shadow_radius, f32 shadow_softness);

static void ui_draw_glyph(vec2 p, u32 codepoint, u32 color);
static void ui_draw_glyph_r(u32 codepoint, rect dst, u32 color); // @Cleanup rect should be first param.
static void ui_draw_text(vec2 p, str s, u32 color);
static void ui_draw_text_r(rect dst, str s, u32 color);
static vec2 ui_draw_text_centered_r(rect dst, str s, u32 color);
static void ui_draw_caret(vec2 p, f32 thickness, u32 color);

static void ui_draw_border(rect r, ui_border border);

////////////////////////////////////////////////////////////////
// rune: Measure

static f32 ui_measure_glyph(u32 codepoint);
static vec2 ui_measure_text(str s);
static vec2 ui_measure_border(ui_border border, vec2 content_dim);

