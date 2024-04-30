////////////////////////////////////////////////////////////////
// rune: References

// Casey Muratori: Immediate-mode Graphical User Interfaces
// https://caseymuratori.com/blog_0001

// Ryan Fluery: UI Series
// https://www.rfleury.com/p/ui-series-table-of-contents

// Omar: Dear ImGUI
// https://github.com/ocornut/imgui

// Martin Cohent: RectCut for dead simple UI layouts
// https://halt.software/dead-simple-layouts/

////////////////////////////////////////////////////////////////
// rune: Fonts

// NOTE(rune): We limit the number of fonts and fontsize to u16,
// so they can fit together with a u32 codepoint in a single u64.

typedef struct yo_font yo_font;
struct yo_font {
    u16 u16;
};

typedef union yo_face yo_face;
union yo_face {
    struct { yo_font font; u16 fontsize; };
    struct { u16 u16[2]; };
    struct { u32 u32; };
};

typedef f32 yo_get_advance_fn(void *userdata, yo_face, u32 glyph);
typedef f32 yo_get_lineheight_fn(void *userdata, yo_face);

typedef struct yo_font_backend yo_font_backend;
struct yo_font_backend {
    void *userdata;
    yo_get_advance_fn *get_advance;
    yo_get_lineheight_fn *get_lineheight;
};

////////////////////////////////////////////////////////////////
// rune: Ops

typedef enum yo_axis {
    YO_AXIS_X,
    YO_AXIS_Y,
} yo_axis;

typedef enum yo_op_kind {
    YO_OP_KIND_NONE,
    YO_OP_KIND_GLYPH,
    YO_OP_KIND_TEXT,
    YO_OP_KIND_AABB,
    YO_OP_KIND_AABB_EX,
    YO_OP_KIND_QUAD,
} yo_op_kind;

typedef struct yo_op yo_op;
struct yo_op {
    yo_op_kind type;
    u32 _unused;
    yo_op *next;
};

typedef struct yo_op_list yo_op_list;
struct yo_op_list {
    yo_op *first;
    yo_op *last;
};

typedef struct yo_op_glyph yo_op_glyph;
struct yo_op_glyph {
    yo_op base;
    u32 color;
    yo_face face;
    u32 codepoint;
    vec2 p;
};

typedef struct yo_op_text yo_op_text;
struct yo_op_text {
    yo_op base;
    u32 color;
    yo_face face;
    vec2 p;
    str s;
};

typedef struct yo_op_aabb yo_op_aabb;
struct yo_op_aabb {
    yo_op base;
    u32 color;
    rect rect;
};

typedef struct yo_op_aabb_ex yo_op_aabb_ex;
struct yo_op_aabb_ex {
    yo_op base;
    u32 color[4];
    f32 corner_radius[4];

    rect dst_rect;
    rect tex_rect;

    f32  softness;
    f32  tex_weight;
};

typedef struct yo_op_quad yo_op_quad;
struct yo_op_quad {
    yo_op base;
    vec4 p[4];
    u32 color[4];
};

////////////////////////////////////////////////////////////////
// rune: Events

typedef enum yo_event_kind {
    YO_EVENT_KIND_NONE,
    YO_EVENT_KIND_MOUSE_PRESS,
    YO_EVENT_KIND_MOUSE_RELEASE,
    YO_EVENT_KIND_KEY_PRESS,
    YO_EVENT_KIND_KEY_RELEASE,
    YO_EVENT_KIND_CODEPOINT,
    YO_EVENT_KIND_SCROLL,
    YO_EVENT_KIND_MOUSE_MOVE,
    YO_EVENT_KIND_RESIZE,
    YO_EVENT_KIND_COUNT,
} yo_event_kind;

typedef enum yo_modifiers {
    YO_MODIFIER_NONE  = 0x0,
    YO_MODIFIER_SHIFT = 0x1,
    YO_MODIFIER_CTRL  = 0x2,
    YO_MODIFIER_ALT   = 0x4,
} yo_modifiers;

typedef enum yo_mouse_button {
    YO_MOUSE_BUTTON_NONE,
    YO_MOUSE_BUTTON_LEFT,
    YO_MOUSE_BUTTON_RIGHT,
    YO_MOUSE_BUTTON_MIDDLE,

    YO_MOUSE_BUTTON_COUNT,
} yo_mouse_button;

typedef enum yo_key {
    YO_KEY_NONE = 0,

    YO_KEY_A = 'A',
    YO_KEY_B = 'B',
    YO_KEY_C = 'C',
    YO_KEY_D = 'D',
    YO_KEY_E = 'E',
    YO_KEY_F = 'F',
    YO_KEY_G = 'G',
    YO_KEY_H = 'H',
    YO_KEY_I = 'I',
    YO_KEY_J = 'J',
    YO_KEY_K = 'K',
    YO_KEY_L = 'L',
    YO_KEY_M = 'M',
    YO_KEY_N = 'N',
    YO_KEY_O = 'O',
    YO_KEY_P = 'P',
    YO_KEY_Q = 'Q',
    YO_KEY_R = 'R',
    YO_KEY_S = 'S',
    YO_KEY_T = 'T',
    YO_KEY_U = 'U',
    YO_KEY_V = 'V',
    YO_KEY_W = 'W',
    YO_KEY_X = 'X',
    YO_KEY_Y = 'Y',
    YO_KEY_Z = 'Z',

    YO_KEY_SPACE = ' ',
    YO_KEY_TAB = '\t',
    YO_KEY_ENTER = '\n',

    YO_KEY_PERIOD = '.',
    YO_KEY_COMMA = ';',
    YO_KEY_MINUS = '-',
    YO_KEY_PLUS = '=',

    YO_KEY_SHIFT = 256,
    YO_KEY_CTRL,
    YO_KEY_ALT,
    YO_KEY_LEFT,
    YO_KEY_RIGHT,
    YO_KEY_UP,
    YO_KEY_DOWN,
    YO_KEY_PAGE_UP,
    YO_KEY_PAGE_DOWN,
    YO_KEY_HOME,
    YO_KEY_END,
    YO_KEY_INSERT,
    YO_KEY_DELETE,
    YO_KEY_BACKSPACE,
    YO_KEY_ESCAPE,

    YO_KEY_F1,
    YO_KEY_F2,
    YO_KEY_F3,
    YO_KEY_F4,
    YO_KEY_F5,
    YO_KEY_F6,
    YO_KEY_F7,
    YO_KEY_F8,
    YO_KEY_F9,
    YO_KEY_F10,
    YO_KEY_F11,
    YO_KEY_F12,

    YO_KEY_COUNT,
} yo_key;

typedef struct yo_event yo_event;
struct yo_event {
    yo_event_kind kind;
    yo_modifiers mods;
    yo_mouse_button mouse_button;
    yo_key key;
    vec2 pos;
    vec2 scroll;
    u32 codepoint;
    bool eaten;

    yo_event *next;
    yo_event *prev;
};

typedef struct yo_event_list yo_event_list;
struct yo_event_list {
    yo_event *first;
    yo_event *last;
    i64 count;
};

////////////////////////////////////////////////////////////////
// rune: Nodes

typedef enum yo_node_flags {
    YO_NODE_FLAG_OFFSET_NOT_RELATIVE = 0x1,

    YO_NODE_FLAG_MOUSE_CLICKABLE = 0x2,
    YO_NODE_FLAG_CLIP = 0x4
} yo_node_flags;

typedef struct yo_node      yo_node;
typedef struct yo_node_list yo_node_list;
struct yo_node_list {
    yo_node *first;
    yo_node *last;
};

typedef u64 yo_id;

typedef yo_node *yo_node_ptr;
typedef struct yo_node yo_node;
struct yo_node {
    yo_id id;
    char *tag;

    yo_node_flags flags;
    vec2 rel_offset;
    rect rel_rect;

    vec2 abs_offset;
    rect abs_rect;

    // rune: Per frame drawing instructions
    yo_op_list ops;

    // rune: Per frame tree links
    yo_node *next;
    yo_node *parent;
    yo_node_list children;

    // rune: Hash chain links
    yo_node *hash_next;
    yo_node *hash_prev;

    // rune: Lifetime
    i64 last_frame_touched;
    i64 first_frame_touched;

    // rune: Animation
    f32 hot_t;
    f32 focus_t;
    f32 active_t;
};

////////////////////////////////////////////////////////////////
// rune: Stacks

#define T vec2
#include "yo_stack.h"
#define T yo_node_ptr
#include "yo_stack.h"
#define T yo_id
#include "yo_stack.h"

////////////////////////////////////////////////////////////////
// rune: Context

typedef struct yo_input yo_input;
struct yo_input {
    rect client_rect;
    yo_event_list events;
    vec2 mouse_pos;
    yo_mouse_button mouse_buttons;
    f32 delta_time;
};

typedef struct yo_state yo_state;
struct yo_state {
    yo_node *root;
    yo_font_backend font_backend;

    bool invalidate_next_frame;

    arena *perm_arena;
    arena *frame_arena;

    yo_node *node_free_list;

    yo_stack(yo_node_ptr) node_stack;
    yo_stack(yo_id) id_stack;

    // rune: Hash table by id
    struct {
        yo_node **slots;
        i64       slots_count;
    } hash;

    // rune: Font
    yo_face curr_face;
    f32 curr_adv_ascii[128];
    f32 curr_lineheight;

    // rune: Hot/active ids
    yo_id hot_id;
    bool  hot_id_found_this_frame;
    yo_id active_id;
    yo_id focus_id;
    vec2 mouse_drag_start;

    // rune: Time
    i64 frame_counter;
    f32 delta_time;

    // rune: Input
    vec2 client_dim;
    vec2 mouse_pos;
    yo_event_list events;

    // rune: Context menu
    yo_id ctx_menu_id;
    bool  ctx_menu_touched_this_frame;
};

////////////////////////////////////////////////////////////////
// rune: Color constants

// NOTE(rune): Source https://gist.githubusercontent.com/curran/b236990081a24761f7000567094914e0/raw/acd2b8cecfe51c520622fbaf407ee88b8796bfc6/cssNamedColors.csv
#define YO_COLOR_BLACK                (0xff000000)
#define YO_COLOR_SILVER               (0xffc0c0c0)
#define YO_COLOR_GRAY                 (0xff808080)
#define YO_COLOR_WHITE                (0xffffffff)
#define YO_COLOR_MAROON               (0xff000080)
#define YO_COLOR_RED                  (0xff0000ff)
#define YO_COLOR_PURPLE               (0xff800080)
#define YO_COLOR_FUCHSIA              (0xffff00ff)
#define YO_COLOR_GREEN                (0xff008000)
#define YO_COLOR_LIME                 (0xff00ff00)
#define YO_COLOR_OLIVE                (0xff008080)
#define YO_COLOR_YELLOW               (0xff00ffff)
#define YO_COLOR_NAVY                 (0xff800000)
#define YO_COLOR_BLUE                 (0xffff0000)
#define YO_COLOR_TEAL                 (0xff808000)
#define YO_COLOR_AQUA                 (0xffffff00)
#define YO_COLOR_ORANGE               (0xff00a5ff)
#define YO_COLOR_ALICEBLUE            (0xfffff8f0)
#define YO_COLOR_ANTIQUEWHITE         (0xffd7ebfa)
#define YO_COLOR_AQUAMARINE           (0xffd4ff7f)
#define YO_COLOR_AZURE                (0xfffffff0)
#define YO_COLOR_BEIGE                (0xffdcf5f5)
#define YO_COLOR_BISQUE               (0xffc4e4ff)
#define YO_COLOR_BLANCHEDALMOND       (0xffcdebff)
#define YO_COLOR_BLUEVIOLET           (0xffe22b8a)
#define YO_COLOR_BROWN                (0xff2a2aa5)
#define YO_COLOR_BURLYWOOD            (0xff87b8de)
#define YO_COLOR_CADETBLUE            (0xffa09e5f)
#define YO_COLOR_CHARTREUSE           (0xff00ff7f)
#define YO_COLOR_CHOCOLATE            (0xff1e69d2)
#define YO_COLOR_CORAL                (0xff507fff)
#define YO_COLOR_CORNFLOWERBLUE       (0xffed9564)
#define YO_COLOR_CORNSILK             (0xffdcf8ff)
#define YO_COLOR_CRIMSON              (0xff3c14dc)
#define YO_COLOR_CYAN                 (0xffffff00)
#define YO_COLOR_AQUA                 (0xffffff00)
#define YO_COLOR_DARKBLUE             (0xff8b0000)
#define YO_COLOR_DARKCYAN             (0xff8b8b00)
#define YO_COLOR_DARKGOLDENROD        (0xff0b86b8)
#define YO_COLOR_DARKGRAY             (0xffa9a9a9)
#define YO_COLOR_DARKGREEN            (0xff006400)
#define YO_COLOR_DARKGREY             (0xffa9a9a9)
#define YO_COLOR_DARKKHAKI            (0xff6bb7bd)
#define YO_COLOR_DARKMAGENTA          (0xff8b008b)
#define YO_COLOR_DARKOLIVEGREEN       (0xff2f6b55)
#define YO_COLOR_DARKORANGE           (0xff008cff)
#define YO_COLOR_DARKORCHID           (0xffcc3299)
#define YO_COLOR_DARKRED              (0xff00008b)
#define YO_COLOR_DARKSALMON           (0xff7a96e9)
#define YO_COLOR_DARKSEAGREEN         (0xff8fbc8f)
#define YO_COLOR_DARKSLATEBLUE        (0xff8b3d48)
#define YO_COLOR_DARKSLATEGRAY        (0xff4f4f2f)
#define YO_COLOR_DARKSLATEGREY        (0xff4f4f2f)
#define YO_COLOR_DARKTURQUOISE        (0xffd1ce00)
#define YO_COLOR_DARKVIOLET           (0xffd30094)
#define YO_COLOR_DEEPPINK             (0xff9314ff)
#define YO_COLOR_DEEPSKYBLUE          (0xffffbf00)
#define YO_COLOR_DIMGRAY              (0xff696969)
#define YO_COLOR_DIMGREY              (0xff696969)
#define YO_COLOR_DODGERBLUE           (0xffff901e)
#define YO_COLOR_FIREBRICK            (0xff2222b2)
#define YO_COLOR_FLORALWHITE          (0xfff0faff)
#define YO_COLOR_FORESTGREEN          (0xff228b22)
#define YO_COLOR_GAINSBORO            (0xffdcdcdc)
#define YO_COLOR_GHOSTWHITE           (0xfffff8f8)
#define YO_COLOR_GOLD                 (0xff00d7ff)
#define YO_COLOR_GOLDENROD            (0xff20a5da)
#define YO_COLOR_GREENYELLOW          (0xff2fffad)
#define YO_COLOR_GREY                 (0xff808080)
#define YO_COLOR_HONEYDEW             (0xfff0fff0)
#define YO_COLOR_HOTPINK              (0xffb469ff)
#define YO_COLOR_INDIANRED            (0xff5c5ccd)
#define YO_COLOR_INDIGO               (0xff82004b)
#define YO_COLOR_IVORY                (0xfff0ffff)
#define YO_COLOR_KHAKI                (0xff8ce6f0)
#define YO_COLOR_LAVENDER             (0xfffae6e6)
#define YO_COLOR_LAVENDERBLUSH        (0xfff5f0ff)
#define YO_COLOR_LAWNGREEN            (0xff00fc7c)
#define YO_COLOR_LEMONCHIFFON         (0xffcdfaff)
#define YO_COLOR_LIGHTBLUE            (0xffe6d8ad)
#define YO_COLOR_LIGHTCORAL           (0xff8080f0)
#define YO_COLOR_LIGHTCYAN            (0xffffffe0)
#define YO_COLOR_LIGHTGOLDENRODYELLOW (0xffd2fafa)
#define YO_COLOR_LIGHTGRAY            (0xffd3d3d3)
#define YO_COLOR_LIGHTGREEN           (0xff90ee90)
#define YO_COLOR_LIGHTGREY            (0xffd3d3d3)
#define YO_COLOR_LIGHTPINK            (0xffc1b6ff)
#define YO_COLOR_LIGHTSALMON          (0xff7aa0ff)
#define YO_COLOR_LIGHTSEAGREEN        (0xffaab220)
#define YO_COLOR_LIGHTSKYBLUE         (0xffface87)
#define YO_COLOR_LIGHTSLATEGRAY       (0xff998877)
#define YO_COLOR_LIGHTSLATEGREY       (0xff998877)
#define YO_COLOR_LIGHTSTEELBLUE       (0xffdec4b0)
#define YO_COLOR_LIGHTYELLOW          (0xffe0ffff)
#define YO_COLOR_LIMEGREEN            (0xff32cd32)
#define YO_COLOR_LINEN                (0xffe6f0fa)
#define YO_COLOR_MAGENTA              (0xffff00ff)
#define YO_COLOR_FUCHSIA              (0xffff00ff)
#define YO_COLOR_MEDIUMAQUAMARINE     (0xffaacd66)
#define YO_COLOR_MEDIUMBLUE           (0xffcd0000)
#define YO_COLOR_MEDIUMORCHID         (0xffd355ba)
#define YO_COLOR_MEDIUMPURPLE         (0xffdb7093)
#define YO_COLOR_MEDIUMSEAGREEN       (0xff71b33c)
#define YO_COLOR_MEDIUMSLATEBLUE      (0xffee687b)
#define YO_COLOR_MEDIUMSPRINGGREEN    (0xff9afa00)
#define YO_COLOR_MEDIUMTURQUOISE      (0xffccd148)
#define YO_COLOR_MEDIUMVIOLETRED      (0xff8515c7)
#define YO_COLOR_MIDNIGHTBLUE         (0xff701919)
#define YO_COLOR_MINTCREAM            (0xfffafff5)
#define YO_COLOR_MISTYROSE            (0xffe1e4ff)
#define YO_COLOR_MOCCASIN             (0xffb5e4ff)
#define YO_COLOR_NAVAJOWHITE          (0xffaddeff)
#define YO_COLOR_OLDLACE              (0xffe6f5fd)
#define YO_COLOR_OLIVEDRAB            (0xff238e6b)
#define YO_COLOR_ORANGERED            (0xff0045ff)
#define YO_COLOR_ORCHID               (0xffd670da)
#define YO_COLOR_PALEGOLDENROD        (0xffaae8ee)
#define YO_COLOR_PALEGREEN            (0xff98fb98)
#define YO_COLOR_PALETURQUOISE        (0xffeeeeaf)
#define YO_COLOR_PALEVIOLETRED        (0xff9370db)
#define YO_COLOR_PAPAYAWHIP           (0xffd5efff)
#define YO_COLOR_PEACHPUFF            (0xffb9daff)
#define YO_COLOR_PERU                 (0xff3f85cd)
#define YO_COLOR_PINK                 (0xffcbc0ff)
#define YO_COLOR_PLUM                 (0xffdda0dd)
#define YO_COLOR_POWDERBLUE           (0xffe6e0b0)
#define YO_COLOR_ROSYBROWN            (0xff8f8fbc)
#define YO_COLOR_ROYALBLUE            (0xffe16941)
#define YO_COLOR_SADDLEBROWN          (0xff13458b)
#define YO_COLOR_SALMON               (0xff7280fa)
#define YO_COLOR_SANDYBROWN           (0xff60a4f4)
#define YO_COLOR_SEAGREEN             (0xff578b2e)
#define YO_COLOR_SEASHELL             (0xffeef5ff)
#define YO_COLOR_SIENNA               (0xff2d52a0)
#define YO_COLOR_SKYBLUE              (0xffebce87)
#define YO_COLOR_SLATEBLUE            (0xffcd5a6a)
#define YO_COLOR_SLATEGRAY            (0xff908070)
#define YO_COLOR_SLATEGREY            (0xff908070)
#define YO_COLOR_SNOW                 (0xfffafaff)
#define YO_COLOR_SPRINGGREEN          (0xff7fff00)
#define YO_COLOR_STEELBLUE            (0xffb48246)
#define YO_COLOR_TAN                  (0xff8cb4d2)
#define YO_COLOR_THISTLE              (0xffd8bfd8)
#define YO_COLOR_TOMATO               (0xff4763ff)
#define YO_COLOR_TURQUOISE            (0xffd0e040)
#define YO_COLOR_VIOLET               (0xffee82ee)
#define YO_COLOR_WHEAT                (0xffb3def5)
#define YO_COLOR_WHITESMOKE           (0xfff5f5f5)
#define YO_COLOR_YELLOWGREEN          (0xff32cd9a)
#define YO_COLOR_REBECCAPURPLE        (0xff993366)

////////////////////////////////////////////////////////////////
// rune: Global state allocation and selection

static yo_state *yo_g_state;

static yo_state *yo_state_create(void);
static void      yo_state_destroy(yo_state *state);
static void      yo_state_select(yo_state *state);
static yo_state *yo_state_get(void);

////////////////////////////////////////////////////////////////
// rune: Frame boundaries

static void    yo_frame_begin(yo_input *input);
static void    yo_frame_end(void);

////////////////////////////////////////////////////////////////
// rune: Node allocation

static yo_node *yo_node_begin(yo_id id);
static yo_node *yo_node_end(void);
static yo_node *yo_node_get(void);
#define         yo_node(id) defer(yo_node_begin(id), yo_node_end())

static yo_node *yo_node_alloc(void);
static void     yo_node_free(yo_node *node);

static yo_node_list yo_children(void);

////////////////////////////////////////////////////////////////
// rune: Tree traversel

static yo_node *yo_node_iter_pre_order(yo_node *curr);
static yo_node *yo_node_iter_post_order(yo_node *curr); // TODO(rune): Implement

////////////////////////////////////////////////////////////////
// rune: Op list contruction

static void *           yo_op_push(yo_op_kind type, u64 size);
static yo_op_glyph *    yo_op_push_glyph(void);
static yo_op_text *     yo_op_push_text(void);
static yo_op_aabb *     yo_op_push_aabb(void);
static yo_op_aabb_ex *  yo_op_push_aabb_ex(void);
static yo_op_quad *     yo_op_push_quad(void);

////////////////////////////////////////////////////////////////
// rune: Hashing

static u64 yo_murmur_mix(u64 a);

#define yo_hash_val(val) (_Generic((val),   \
        i8      : yo_hash_i8,               \
        i16     : yo_hash_i16,              \
        i64     : yo_hash_i64,              \
        i32     : yo_hash_i32,              \
        u8      : yo_hash_u8,               \
        u16     : yo_hash_u16,              \
        u32     : yo_hash_u32,              \
        u64     : yo_hash_u64,              \
        f32     : yo_hash_f32,              \
        f64     : yo_hash_f64,              \
        bool    : yo_hash_bool,             \
        str     : yo_hash_str,              \
        char *  : yo_hash_cstr,             \
        default : yo_hash_ptr               \
))((val))

#define yo_hash(...)       VA_AGGREGATE(yo_id_combine, VA_WRAP(yo_hash_val, __VA_ARGS__))

static yo_id yo_hash_i8(i8 a);
static yo_id yo_hash_i16(i16 a);
static yo_id yo_hash_i32(i32 a);
static yo_id yo_hash_i64(i64 a);
static yo_id yo_hash_u8(u8 a);
static yo_id yo_hash_u16(u16 a);
static yo_id yo_hash_u32(u32 a);
static yo_id yo_hash_u64(u64 a);
static yo_id yo_hash_f32(f32 a);
static yo_id yo_hash_f64(f64 a);
static yo_id yo_hash_bool(bool a);
static yo_id yo_hash_str(str s);
static yo_id yo_hash_cstr(char *s);
static yo_id yo_hash_ptr(void *a);

////////////////////////////////////////////////////////////////
// rune: Ids

#define yo_id(...)   yo_id_combine_with_parent(yo_hash(__VA_ARGS__))
#define yo_auto_id() yo_id_combine_with_parent(yo_hash(__COUNTER__))

static yo_id yo_id_root(void);
static yo_id yo_id_combine(yo_id a, yo_id b);
static yo_id yo_id_combine_with_parent(yo_id child_id);

static yo_id yo_active_id(void);
static yo_id yo_hot_id(void);

////////////////////////////////////////////////////////////////
// rune: Events

static yo_event_list yo_events(void);
static yo_event *    yo_event_list_push(yo_event_list *list, arena *arena);
static void          yo_event_list_remove(yo_event_list *list, yo_event *e);

static bool yo_event_is_key_press(yo_event *e, yo_key key, yo_modifiers mods);
static bool yo_event_is_key_release(yo_event *e, yo_key key, yo_modifiers mods);
static bool yo_event_is_mouse_press(yo_event *e, yo_mouse_button button, yo_modifiers mods);

static bool yo_event_eat(yo_event *e);
static bool yo_event_eat_key_press(yo_key key, yo_modifiers mods);
static bool yo_event_eat_key_release(yo_key key, yo_modifiers mods);

////////////////////////////////////////////////////////////////
// rune: Mouse

static vec2 yo_mouse_pos(void);
static vec2 yo_mouse_drag_delta(void);

////////////////////////////////////////////////////////////////
// rune: Context menus

static void yo_ctx_menu_open(yo_id id);
static void yo_ctx_menu_close(void);
static bool yo_ctx_menu_begin(yo_id id);
static void yo_ctx_menu_end(void);
#define     yo_ctx_menu(id) defer_if(yo_ctx_menu_begin(id), yo_ctx_menu_end())

////////////////////////////////////////////////////////////////
// rune: Draw

static void yo_draw_rect(rect dst, u32 color);
static void yo_draw_gradient(rect dst, u32 color0, u32 color1, u32 color2, u32 color3);
static void yo_draw_soft_rect(rect dst, u32 color, f32 softness);
static void yo_draw_rounded_rect(rect dst, u32 color, f32 roundness);
static void yo_draw_textured(rect dst, u32 color, rect uv);

static void yo_draw_shadow(rect dst, f32 shadow_radius, f32 shadow_softness);
static void yo_draw_rounded_shadow(rect dst, f32 shadow_radius, f32 roundness, f32 shadow_softness);
static void yo_draw_rect_with_shadow(rect dst, u32 color, f32 shadow_radius, f32 shadow_softness);
static void yo_draw_rounded_rect_with_shadow(rect dst, u32 color, f32 roundness, f32 shadow_radius, f32 shadow_softness);

static void yo_draw_glyph(vec2 dst, u32 codepoint, u32 color);
static void yo_draw_text(vec2 dst, str s, u32 color);
static void yo_draw_caret(vec2 dst, f32 thickness, u32 color);

////////////////////////////////////////////////////////////////
// rune: Font measuring

static f32  yo_adv_from_glyph(u32 codepoint);
static f32  yo_adv_from_text(str s);
static vec2 yo_dim_from_glyph(u32 codepoint);
static vec2 yo_dim_from_text(str s);

////////////////////////////////////////////////////////////////
// rune: Rect cut

static void yo_rect_cut(rect *r, f32 left, f32 right, f32 top, f32 bot);
static rect yo_rect_cut_left(rect *r, f32 a);
static rect yo_rect_cut_right(rect *r, f32 a);
static rect yo_rect_cut_top(rect *r, f32 a);
static rect yo_rect_cut_bot(rect *r, f32 a);
static rect yo_rect_cut_dir(rect *r, f32 a, dir2 dir);
static void yo_rect_cut_all(rect *r, f32 a);
static void yo_rect_cut_x(rect *r, f32 a);
static void yo_rect_cut_y(rect *r, f32 a);
static void yo_rect_cut_xy(rect *r, f32 x, f32 y);
static void yo_rect_cut_vec2(rect *r, vec2 a);
static void yo_rect_cut_a(rect *r, f32 a, yo_axis axis);
static void yo_rect_cut_sides(rect *r, f32 left, f32 right, f32 top, f32 bot);

static void yo_rect_cut_centered_x(rect *r, f32 pref_dim_x);
static void yo_rect_cut_centered_y(rect *r, f32 pref_dim_y);
static void yo_rect_cut_centered_a(rect *r, f32 pref_dim_a, yo_axis axis);
static void yo_rect_cut_centered_xy(rect *r, f32 pref_dim_x, f32 pref_dim_y);
static void yo_rect_cut_centered(rect *r, vec2 pref_dim);
//static void yo_cut_aligned(rect *r, vec2 pref_dim, yo_align align);
//static void yo_cut_border(rect *r, yo_border border);

static void yo_rect_add_left(rect *r, f32 a);
static void yo_rect_add_right(rect *r, f32 a);
static void yo_rect_add_top(rect *r, f32 a);
static void yo_rect_add_bot(rect *r, f32 a);
static void yo_rect_add_dir(rect *r, f32 a, dir2 dir);
static void yo_rect_add_all(rect *r, f32 a);
static void yo_rect_add_x(rect *r, f32 a);
static void yo_rect_add_y(rect *r, f32 a);
static void yo_rect_add_axis(rect *r, f32 a, yo_axis axis);
static void yo_rect_add(rect *r, f32 left, f32 right, f32 top, f32 bot);

static void yo_rect_max_left(rect *r, f32 a);
static void yo_rect_max_right(rect *r, f32 a);
static void yo_rect_max_top(rect *r, f32 a);
static void yo_rect_max_bot(rect *r, f32 a);
static void yo_rect_max_dir(rect *r, f32 a, dir2 dir);

static rect yo_rect_get_left(rect *r, f32 a);
static rect yo_rect_get_right(rect *r, f32 a);
static rect yo_rect_get_top(rect *r, f32 a);
static rect yo_rect_get_bot(rect *r, f32 a);
static rect yo_rect_get_dir(rect *r, f32 a, dir2 dir);

////////////////////////////////////////////////////////////////
// rune: Animation

static f32 yo_delta_time(void);

static void yo_anim_base(f32 *value, f32 target, f32 rate);
static f32  yo_anim_f32(f32 *pos, f32 target, f32 rate);
static vec2 yo_anim_vec2(vec2 *pos, vec2 target, f32 rate);
static vec3 yo_anim_vec3(vec3 *pos, vec3 target, f32 rate);
static vec4 yo_anim_vec4(vec4 *pos, vec4 target, f32 rate);
static rect yo_anim_rect(rect *pos, rect target, f32 rate);

////////////////////////////////////////////////////////////////
// rune: String conversions

static str yo_str_from_op_kind(yo_op_kind type);
