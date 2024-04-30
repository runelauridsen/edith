////////////////////////////////////////////////////////////////
// rune: Draw

typedef_darray(r_rect_instance);

typedef struct draw_ctx draw_ctx;
struct draw_ctx {
    arena *arena;
    r_pass current_pass;
    r_pass_list submitted_passes;

    atlas *atlas;
    r_tex  atlas_tex;

    rect viewport;

    r_state *renderer;

    darray(r_rect_instance) *rect_storage;

    rect scissor;
};

////////////////////////////////////////////////////////////////
// rune: General

static void        draw_create_ctx(draw_ctx *ctx, r_state *renderer);
static void        draw_destroy_ctx(draw_ctx *ctx);
static void        draw_select_ctx(draw_ctx *ctx);
static void        draw_begin(void);
static r_pass_list draw_end(void);

////////////////////////////////////////////////////////////////
// rune: Rect pass

static void     draw_begin_rect_pass(rect viewport, darray(r_rect_instance) *storage, atlas *atlas);
static r_pass   draw_end_rect_pass(void);
static void     draw_rect_instance(r_rect_instance rect);
