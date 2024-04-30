////////////////////////////////////////////////////////////////
// rune: Configuration

#if EDITH_DOC_DEBUG_OUTPUT && !defined(RUN_TESTS)
static void edith_doc_debug_print_state_(gapbuffer *gb);
#define edith_doc_debug_print(...)                print(__VA_ARGS__)
#define edith_doc_debug_println(...)              println(__VA_ARGS__)
#define edith_doc_debug_print_state(...)          doc_debug_print_state_(__VA_ARGS__)
#else
#define edith_doc_debug_print(...)                unused(__VA_ARGS__)
#define edith_doc_debug_println(...)              unused(__VA_ARGS__)
#define edith_doc_debug_print_state(...)          unused(__VA_ARGS__)
#endif

////////////////////////////////////////////////////////////////
// rune: Helpers

static i64 edith_gapbuffer_len(edith_gapbuffer *gb) {
    i64 ret = gb->buf_cap - range_len(gb->gap);
    return ret;
}

static i64 edith_pos_to_raw(edith_gapbuffer *gb, i64 pos) {
    i64_range gap = gb->gap;
    if (pos >= gap.min) {
        return pos + range_len(gap);
    } else {
        return pos;
    }
}

static i64 edith_raw_to_pos(edith_gapbuffer *gb, i64 raw) {
    i64_range gap = gb->gap;
    assert(!range_contains(gap, raw));

    if (raw >= gap.min) {
        return raw - range_len(gap);
    } else {
        return raw;
    }
}

static u8 *edith_doc_get_raw_ptr(edith_gapbuffer *gb, i64 pos) {
    i64 rawidx = edith_pos_to_raw(gb, pos);
    u8 *ret;
    if (rawidx >= 0 && rawidx < gb->buf_cap) {
        ret = &gb->buf[rawidx];
    } else if (rawidx < 0) {
        ret = &gb->buf[0];
    } else {
        ret = &gb->buf[gb->buf_cap];
    }
    return ret;
}

static u8 edith_doc_get_byte_at(edith_gapbuffer *gb, i64 pos) {
    u8 ret = *edith_doc_get_raw_ptr(gb, pos);
    return ret;
}

static u32 edith_doc_get_codepoint_at(edith_gapbuffer *gb, i64 pos) {
    clamp_assign(&pos, 0, edith_gapbuffer_len(gb));
    i64 rawidx = edith_pos_to_raw(gb, pos);
    str s = substr_idx(str_make(gb->buf, gb->buf_cap), rawidx);
    unicode_codepoint unicode = decode_single_utf8_codepoint(s);
    return unicode.codepoint;
}

////////////////////////////////////////////////////////////////
// rune: Gap buffer

static void edith_gapbuffer_create(edith_gapbuffer *gb, i64 initial_size, str name) {
    zero_struct(gb);
    gb->buf = edith_heap_alloc(initial_size, name);
}

static void edith_gapbuffer_destroy(edith_gapbuffer *gb) {
    edith_heap_free(gb->buf);
}

static void edith_gapbuffer_reset(edith_gapbuffer *gb) {
    gb->gap.min = 0;
    gb->gap.max = gb->buf_cap;
}

static void edith_gapbuffer_shift_gap(edith_gapbuffer *gb, i64 to) {
    YO_PROFILE_BEGIN(edith_gapbuffer_shift_gap);
    i64_range gap = gb->gap;
    i64 gap_len = range_len(gap);
    clamp_assign(&to, 0, gb->buf_cap - gap_len);

    if (gap.min < to) {
        i64 delta = to - gap.min;
        memmove(gb->buf + gap.min,
                gb->buf + gap.max,
                delta);

        gb->gap.min += delta;
        gb->gap.max += delta;
    }

    if (gap.min > to) {
        i64 delta = gb->gap.min - to;
        memcpy(gb->buf + gb->gap.max - delta,
               gb->buf + gb->gap.min - delta,
               delta);

        gb->gap.min -= delta;
        gb->gap.max -= delta;
    }
    YO_PROFILE_END(edith_gapbuffer_shift_gap);
}

static void edith_gapbuffer_reserve_gap(edith_gapbuffer *gb, i64 reserve_len) {
    i64 gap_len = range_len(gb->gap);
    if (gap_len < reserve_len) {
        edith_gapbuffer_shift_gap(gb, gb->buf_cap - gap_len);

        i64   new_buf_cap = clamp_bot(i64_round_up_to_pow2((reserve_len + gb->buf_cap - gap_len) * 2), 16);
        void *new_buf     = edith_heap_realloc(gb->buf, new_buf_cap);

        gb->buf     = new_buf;
        gb->buf_cap = new_buf_cap;
        gb->gap.max = new_buf_cap;
    }
}

static void edith_gapbuffer_insert(edith_gapbuffer *gb, i64 pos, str to_insert) {
    if (pos <= edith_gapbuffer_len(gb)) {
        edith_gapbuffer_reserve_gap(gb, to_insert.len);
        edith_gapbuffer_shift_gap(gb, pos);
        memcpy(gb->buf + gb->gap.min, to_insert.v, to_insert.len);
        gb->gap.min += to_insert.len;
    } else {
        //assert(false);
    }
}

static void edith_gapbuffer_delete(edith_gapbuffer *gb, i64 pos, i64 len) {
    edith_gapbuffer_shift_gap(gb, pos);
    gb->gap.max += len;
    clamp_assign(&gb->gap.max, 0, gb->buf_cap);
}

static void edith_gapbuffer_apply_edits(edith_gapbuffer *gb, edith_edit_array edits) {
    i64 offset = 0;
    for_array (edith_edit, edit, edits) {
        switch (edits.kind) {
            case EDITH_EDIT_KIND_INSERT: {
                edith_gapbuffer_insert(gb, edit->pos + offset, edit->data);
                offset += edit->data.len;
            } break;

            case EDITH_EDIT_KIND_DELETE: {
                edith_gapbuffer_delete(gb, edit->pos + offset, edit->data.len);
                offset -= edit->data.len;
            } break;

            default: {
                assert(false);
            } break;
        }
    }
}

static str edith_str_from_gapbuffer_range(edith_gapbuffer *gb, i64_range range, arena *arena) {
    YO_PROFILE_BEGIN(edith_str_from_gapbuffer_range);
    i64_range gap = gb->gap;

    i64 raw_min = edith_pos_to_raw(gb, range.min);
    i64 raw_max = edith_pos_to_raw(gb, range.max);

    i64_range before_gap = { 0 };
    i64_range after_gap = { 0 };

    before_gap.min = raw_min;
    before_gap.max = clamp(gap.min, raw_min, raw_max);

    after_gap.min = clamp(gap.max, raw_min, raw_max);
    after_gap.max = raw_max;

    i64 before_gap_len = range_len(before_gap);
    i64 after_gap_len = range_len(after_gap);

    i64 ret_len = before_gap_len + after_gap_len;
    str ret = {
        .v = arena_push_array(arena, u8, ret_len),
        .len = ret_len,
    };

    memcpy(ret.v,
           gb->buf + before_gap.min,
           before_gap_len);

    memcpy(ret.v   + before_gap_len,
           gb->buf + after_gap.min,
           after_gap_len);

    YO_PROFILE_END(edith_str_from_gapbuffer_range);
    return ret;
}

static str edith_str_from_gapbuffer(edith_gapbuffer *gb, arena *arena) {
    i64_range full_range = { 0, edith_gapbuffer_len(gb) };
    str s = edith_str_from_gapbuffer_range(gb, full_range, arena);
    return s;
}

static void edith_gapbuffer_copy_range(edith_gapbuffer *gb, i64 min, i64 max, u8 *out) {
    i64_range gap = gb->gap;

    i64 rawidx_min = edith_pos_to_raw(gb, min);
    i64 rawidx_max = edith_pos_to_raw(gb, max);

    i64_range before_gap = { 0 };
    i64_range after_gap = { 0 };

    before_gap.min = rawidx_min;
    before_gap.max = clamp(gap.min, rawidx_min, rawidx_max);

    after_gap.min = clamp(gap.max, rawidx_min, rawidx_max);
    after_gap.max = rawidx_max;

    i64 before_gap_len = range_len(before_gap);
    i64 after_gap_len = range_len(after_gap);

    memcpy(out,
           gb->buf + before_gap.min,
           before_gap_len);

    memcpy(out     + before_gap_len,
           gb->buf + after_gap.min,
           after_gap_len);
}

static str edith_gapbuffer_copy_len(edith_gapbuffer *gb, i64 idx, i64 len, arena *arena) {
    str ret = edith_str_from_gapbuffer_range(gb, i64_range(idx, idx + len), arena);
    return ret;
}

////////////////////////////////////////////////////////////////
// rune: Iterator

static i64 edith_global_doc_iter_counter;

static void edith_doc_iter_jump(edith_doc_iter *iter, edith_gapbuffer *gb, i64 pos) {
    pos = min(pos, edith_gapbuffer_len(gb));

    iter->gb = gb;
    iter->pos = pos;
    iter->codepoint = edith_doc_get_codepoint_at(iter->gb, iter->pos);
    if (iter->codepoint == '\r') iter->codepoint = '\n';
    iter->valid = iter->pos >= 0 && iter->pos < edith_gapbuffer_len(gb);
}

static void edith_doc_iter_next(edith_doc_iter *iter, dir direction) {
    edith_global_doc_iter_counter++;

    edith_gapbuffer *gb = iter->gb;
    iter->valid = false;

    switch (direction) {
        case DIR_FORWARD: {
            while (iter->pos < edith_gapbuffer_len(gb)) {
                iter->pos++;
                if (iter->pos < edith_gapbuffer_len(gb)) {
                    i64 rawidx = edith_pos_to_raw(gb, iter->pos);
                    assert_bounds(rawidx, gb->buf_cap);
                    u8 byte = gb->buf[rawidx];
                    if (!utf8_is_continuation(byte)) {
                        break;
                    }
                } else {
                    break;
                }
            }

            iter->valid = iter->pos < edith_gapbuffer_len(gb);
        } break;

        case DIR_BACKWARD: {
            while (iter->pos > 0) {
                iter->pos--;
                i64 rawidx = edith_pos_to_raw(gb, iter->pos);
                assert_bounds(rawidx, gb->buf_cap);
                u8 byte = gb->buf[rawidx];
                if (!utf8_is_continuation(byte)) {
                    iter->valid = true;
                    break;
                }
            }
        } break;
    }

    iter->codepoint = edith_doc_get_codepoint_at(gb, iter->pos);

    // LF-normalize: If we encounter a CR we just it treat as a LF and skip the following LF.

    if (iter->codepoint == '\n' && iter->valid) {
        u8 prev_byte = edith_doc_get_byte_at(gb, iter->pos - 1);
        if (prev_byte == '\r') {
            edith_doc_iter_next(iter, direction);
        }
    }

    if (iter->codepoint == '\r') {
        iter->codepoint = '\n';
    }
}

static void edith_doc_iter_to_line_start(edith_doc_iter *iter) {
    while (1) {
        edith_doc_iter next = *iter;
        edith_doc_iter_next(&next, DIR_BACKWARD);
        if (iter->valid == false || next.codepoint == '\n') {
            break;
        }
        *iter = next;
    }
}

static void edith_doc_iter_to_line_end(edith_doc_iter *iter) {
    while (1) {
        if (iter->valid == false || iter->codepoint == '\n') {
            break;
        }
        edith_doc_iter_next(iter, DIR_FORWARD);
    }
}

static void edith_doc_iter_to_line_side(edith_doc_iter *iter, side side) {
    switch (side) {
        case SIDE_MIN: edith_doc_iter_to_line_start(iter); break;
        case SIDE_MAX: edith_doc_iter_to_line_end(iter); break;
    }
}

static void edith_doc_iter_by_cols(edith_doc_iter *iter, i64 cols) {
    i64 at = 0;
    while (iter->valid) {
        if (at >= cols) {
            break;
        }

        if (iter->codepoint == '\n') {
            break;
        }

        edith_doc_iter_next(iter, DIR_FORWARD);
        at += 1;
    }
}

////////////////////////////////////////////////////////////////
// rune: Utility

// TODO(rune): Remove edith_doc_concat_fulltext and use edith_str_from_gapbuffer() instead.
static str edith_doc_concat_fulltext(edith_gapbuffer *gb, arena *arena) {
    str before_gap = str_make(gb->buf, gb->gap.min);
    str after_gap  = str_make(gb->buf + gb->gap.max, gb->buf_cap - gb->gap.max);

    i64   temp_size = gb->buf_cap;
    void *temp = edith_heap_alloc(temp_size, str("edith_doc_concat_fulltext"));
    buf buf = make_buf(temp, temp_size, 0);
    buf_append_str(&buf, before_gap);
    buf_append_str(&buf, after_gap);

    str ret = arena_copy_str(arena, buf_as_str(buf));
    edith_heap_free(temp);

    return ret;
}

static void edith_gapbuffer_print(edith_gapbuffer *gb) {
    str s = str_make(gb->buf, gb->buf_cap);
    str_x3 x3 = str_split_x3(s, gb->gap.min, gb->gap.max);
    str s0 = x3.v[0];
    str s1 = x3.v[1];
    str s2 = x3.v[2];

    for (i64 i = 0; i < s0.len; i++) {
        printf("%c", s0.v[i]);
    }

    printf(ANSI_FG_GRAY);
    for (i64 i = 0; i < s1.len; i++) {
        printf(".");
    }
    printf(ANSI_RESET);

    for (i64 i = 0; i < s2.len; i++) {
        printf("%c", s2.v[i]);
    }

    printf("\n");
}
