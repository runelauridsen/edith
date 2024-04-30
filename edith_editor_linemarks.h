////////////////////////////////////////////////////////////////
// rune: Acceleration structure for mapping linenumber <> pos.

typedef struct edith_linemarks edith_linemarks;
struct edith_linemarks {
    edith_gapbuffer64 pos_buf;
};

static void edith_linemarks_apply_edits(edith_linemarks *lms, edith_edit_array edits);
static i64  edith_pos_from_linenumber(edith_linemarks *lms, i64 linenumber);
static i64  edith_linenumber_from_pos(edith_linemarks *lms, i64 pos);
