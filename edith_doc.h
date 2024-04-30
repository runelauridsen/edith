////////////////////////////////////////////////////////////////
// rune: Macros

#define EDITH_DOC_DEBUG_OUTPUT 0

////////////////////////////////////////////////////////////////
// rune: Gap buffer

typedef struct edith_gapbuffer edith_gapbuffer;
struct edith_gapbuffer {
    u8 *buf;
    i64 buf_cap;
    i64_range gap;
};

static void edith_gapbuffer_create(edith_gapbuffer *gb, i64 initial_size, str name);
static void edith_gapbuffer_destroy(edith_gapbuffer *gb);
static void edith_gapbuffer_reset(edith_gapbuffer *gb);
static void edith_gapbuffer_insert(edith_gapbuffer *gb, i64 at, str insert);
static void edith_gapbuffer_delete(edith_gapbuffer *gb, i64 at, i64 len);

////////////////////////////////////////////////////////////////
// rune: Iterator

typedef struct edith_doc_iter edith_doc_iter;
struct edith_doc_iter {
    edith_gapbuffer *gb;
    i64 pos;
    u32 codepoint;
    bool valid;
};

static void edith_doc_iter_jump(edith_doc_iter *iter, edith_gapbuffer *gb, i64 pos);
static void edith_doc_iter_next(edith_doc_iter *iter, dir direction);

static void edith_doc_iter_to_line_start(edith_doc_iter *iter);
static void edith_doc_iter_to_line_end(edith_doc_iter *iter);
static void edith_doc_iter_to_line_side(edith_doc_iter *iter, side side);
static void edith_doc_iter_by_cols(edith_doc_iter *iter, i64 x);

////////////////////////////////////////////////////////////////
// rune: Utility

static str edith_doc_concat_fulltext(edith_gapbuffer *gb, arena *arena);
