////////////////////////////////////////////////////////////////
// rune: Configuration

#define EDITH_TEXTVIEW_DEBUG_PRINT 0

////////////////////////////////////////////////////////////////
// rune: Textview

typedef struct edith_cursor edith_cursor;
struct edith_cursor {
    i64 caret;
    i64 mark;
    edith_textbuf_coord want;
    vec2 animated_p;
    bool is_primary;
};

typedef struct edith_saved_cursor edith_saved_cursor;
struct edith_saved_cursor {
    i64 caret;
    i64 mark;
    edith_textbuf_coord want;
    side side;
    bool is_primary;
};

typedef_darray(edith_cursor);

typedef struct edith_textview edith_textview;
struct edith_textview {
    edith_textbuf tb; // TODO(rune): Should be non-owning.

    irect glyph_viewport;
    i64   cells_per_page;
    ivec2 cell_dim;

    struct {
        darray(edith_cursor) cursors; // TODO(rune): Just make this a fixed sized array.
    };

    struct {
        // NOTE(rune): animated_scroll is in pixel coordidates, but targe_scroll is in cell coordinates.
        vec2 animated_scroll;
        edith_textbuf_coord target_scroll;

        f32  anim_linenumber_width;
        f32  anim_linenumber_width_vel;
        bool anim_linenumber_width_started;
        bool anim_linenumber_width_finished;
    };

    u128 search_result_key_requested;
    u128 search_result_key_visible;

    arena *local;
};

////////////////////////////////////////////////////////////////
// rune: Lifetime

static void edith_textview_create(edith_textview *tv);
static void edith_textview_destroy(edith_textview *tv);

////////////////////////////////////////////////////////////////
// rune: Submission

// TODO(rune): Explanation comment for submission

static void edith_textview_submit_barrier(edith_textview *tv);

static void edith_textview_submit_u8(edith_textview *tv, u8 c);
static void edith_textview_submit_u32(edith_textview *tv, u32 c);
static void edith_textview_submit_str(edith_textview *tv, str s);
static void edith_textview_submit_str_list(edith_textview *tv, str_list strings);

static void edith_textview_submit_delete(edith_textview *tv, move_by by, dir dir);
static void edith_textview_submit_delete_selected(edith_textview *tv);

static void edith_textview_submit_move(edith_textview *tv, move_by by, dir dir, bool expand_selection);
static void edith_textview_submit_move_cursor(edith_textview *tv, move_by by, dir dir, bool expand_selection, edith_cursor *it);

////////////////////////////////////////////////////////////////
// rune: History

static void edith_textview_unredo(edith_textview *tv, edith_unredo unredo); // TODO(rune): Move to textbuf

////////////////////////////////////////////////////////////////
// rune: Cursor movement

static void edith_textview_move(edith_textview *tv, move_by by, dir dir, bool expand_selection);
static void edith_textview_move_to_pos(edith_textview *tv, i64 pos, bool expand_selection);
static void edith_textview_move_to_coord(edith_textview *tv, edith_textbuf_coord pos, bool expand_selection);
static void edith_textview_move_to_pixel(edith_textview *tv, vec2 pixel, bool expand_selection);

////////////////////////////////////////////////////////////////
// rune: Cursor placement

static void edith_textview_add_cursor_at_pos(edith_textview *tv, i64 caret, i64 mark, bool make_primary, edith_cursor *parent);
static void edith_textview_add_cursor_at_coord(edith_textview *tv, edith_textbuf_coord coord, bool make_primary, edith_cursor *parent);
static void edith_textview_add_cursor_at_pixel(edith_textview *tv, vec2 pixel, bool make_primary);
static void edith_textview_add_cursor_at_row_offset(edith_textview *tv, i64 row_offset); // NOTE(rune): Add cursor above/below primary cursor

////////////////////////////////////////////////////////////////
// rune: Cursor managment

static edith_cursor *edith_textview_cursors_get_primary(edith_textview *tv);
static void          edith_textview_cursors_clear_secondary(edith_textview *tv);

static void          edith_textview_cursors_push(edith_textview *tv, side side);
static void          edith_textview_cursors_restore(edith_textview *tv, edith_saved_cursor_array saved_cursors);
static void          edith_textview_cursors_dedup(edith_textview *tv);

static edith_cursor  edith_cursor_make(i64 caret, i64 mark, edith_textbuf_coord want);
static i64_range     edith_cursor_range(edith_cursor cursor);

////////////////////////////////////////////////////////////////
// rune: Selection

static void edith_editor_clear_selections(edith_textview *tv);

////////////////////////////////////////////////////////////////
// rune: Scroll

typedef enum edith_textview_scroll_target {
    EDITH_TEXTVIEW_SCROLL_TARGET_DEFAULT,
    EDITH_TEXTVIEW_SCROLL_TARGET_CENTER,
} edith_textview_scroll_target;

static void edith_textview_scroll_to_pos(edith_textview *tv, i64 pos, edith_textview_scroll_target target);
static void edith_textview_scroll_to_coord(edith_textview *tv, edith_textbuf_coord coord, edith_textview_scroll_target target);
static void edith_textview_scroll_to_cursor(edith_textview *tv, edith_cursor cursor, edith_textview_scroll_target target);
static void edith_textview_scroll_clamp(edith_textview *tv);
static void edith_textview_scroll_to_primary_cursor(edith_textview *tv, edith_textview_scroll_target target);

////////////////////////////////////////////////////////////////
// rune: Pixel <> pos translation

static i64  edith_textview_pos_from_pixel(edith_textview *tv, vec2 pixel);

////////////////////////////////////////////////////////////////
// rune: Cell size

static void  edith_textview_set_face(edith_textview *tv, yo_face face);

////////////////////////////////////////////////////////////////
// rune: Debugging

static void edith_debug_print_saved_cursors(edith_textview *tv);
static void edith_assert_that_cursors_are_sorted(edith_textview *tv);
static void edith_assert_that_selections_to_not_overlap(edith_textview *tv);
