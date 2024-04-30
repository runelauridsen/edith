////////////////////////////////////////////////////////////////
// rune: Box

#define UI_LEN_TEXT    F32_MIN
#define UI_LEN_NOT_REL F32_MIN

typedef struct ui_len ui_len;
struct ui_len {
    f32 min;
    f32 max;
    f32 rel;
};

typedef struct ui_box_draw ui_box_draw;
struct ui_box_draw {
    u32 color[4];
    f32 corner_radius[4];
    rect uv;
    f32 softness;
    f32 tex_weight;

    ui_sides_f32 padding;
    ui_border border;

    bool has_border;
    bool has_background;
};

typedef struct ui_callback_param ui_callback_param;
struct ui_callback_param {
    rect screen_rect;
    rect padded_screen_rect;
    struct ui_box *root;
};

typedef void ui_callback_fn(ui_callback_param *param, void *userdata);

typedef struct ui_box ui_box;
struct ui_box {
    ui_id id;

    // NOTE(rune): Builder params.
    struct {
        u32 text_color;
        str text;
        ui_text_align text_align;
        ui_face face;

        union {
            struct { ui_len dim[2]; };
            struct { ui_len dim_x, dim_y; };
        };

        ui_axis_align align_x : 2;
        ui_axis_align align_y : 2;
        u8 child_axis : 1;
        u8 use_child_axis : 1;
        u8 allow_overflow : 1;
        u8 child_clip : 1;

        ui_sides_f32 padding;
        f32 shadow;

        ui_callback_fn *callback;
        void *callback_userdata;
    };

    // NOTE(rune): Box hierarchy links.
    struct {
        ui_box *parent;
        ui_box *next;
        ui_box *prev;
        list(ui_box) children;
    };

    // NOTE(rune): Calculated by auto-layout.
    struct {
        vec2 calc_child_sum;
        vec2 calc_dim;
        vec2 calc_pos;

        // TODO(rune): Only needed for widget interaction check, should be moved somewhere else.
        rect screen_rect;
        rect clipped_screen_rect;
    };

    struct {
        ui_box_draw draw;
    };

    // NOTE(rune): Persistent.
    struct {
        void *user_data;
        u64   user_data_size;

        vec2 scroll;
        ui_animated_vec2 animated_scroll;
        f32 hot_t;
        f32 active_t;
        f32 focused_t;
    };

    ui_box *focus_parent;
    ui_box *focus_next;
    ui_box *focus_prev;
    list(ui_box) focus_children;

    ui_box *next_hash;
    ui_box *prev_hash;
};

typedef ui_box  *ui_box_ptr;
