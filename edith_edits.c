////////////////////////////////////////////////////////////////
// rune: Edits

static edith_edit_array edith_edit_array_reserve(arena *arena, i64 count, edith_edit_kind kind) {
    edith_edit_array ret = {
        .v     = arena_push_array_nozero(arena, edith_edit, count),
        .count = count,
        .kind  = kind,
    };
    return ret;
}

static edith_edit *edith_edit_list_push(edith_edit_list *list, arena *arena) {
    edith_edit_node *node = arena_push_struct(arena, edith_edit_node);
    slist_push(list, node);
    list->count++;
    return &node->edit;
}

static edith_edit *edith_edit_array_pop(edith_edit_array *edits) {
    edith_edit *ret = null;
    if (edits->count > 0) {
        ret = edits->v;
        edits->v++;
        edits->count--;
    }
    return ret;
}

static edith_edit_array edith_edit_array_copy(arena *arena, edith_edit_array src, bool deep) {
    edith_edit_array dst = edith_edit_array_reserve(arena, src.count, src.kind);
    memcpy(dst.v, src.v, src.count * sizeof(edith_edit));
    if (deep) {
        for_array (edith_edit, e, dst) {
            e->data = arena_copy_str(arena, e->data);
        }
    }
    return dst;
}

static edith_edit_array edith_edit_array_from_list(edith_edit_list list, edith_edit_kind kind, arena *arena) {
    edith_edit_array array = edith_edit_array_reserve(arena, list.count, kind);
    i64 i = 0;
    for_list (edith_edit_node, node, list) {
        array.v[i++] = node->edit;
    }
    return array;
}

static edith_edit_array edith_edit_array_invert(edith_edit_array src, arena *arena) {
    edith_edit_array dst = edith_edit_array_reserve(arena, src.count, 0);
    switch (src.kind) {
        case EDITH_EDIT_KIND_INSERT: {
            dst.kind = EDITH_EDIT_KIND_DELETE;
            i64 offset = 0;
            for_n (i64, i, src.count) {
                dst.v[i].pos  = src.v[i].pos + offset;
                dst.v[i].data = src.v[i].data;
                offset       += src.v[i].data.len;
            }
        } break;

        case EDITH_EDIT_KIND_DELETE: {
            dst.kind = EDITH_EDIT_KIND_INSERT;
            i64 offset = 0;
            for_n (i64, i, src.count) {
                dst.v[i].pos  = src.v[i].pos + offset;
                dst.v[i].data = src.v[i].data;
                offset       -= src.v[i].data.len;
            }
        } break;
    }

    return dst;
}

static str edith_str_from_edit_kind(edith_edit_kind kind) {
    switch (kind) {
        case EDITH_EDIT_KIND_NONE:      return str("EDITH_EDIT_KIND_NONE");
        case EDITH_EDIT_KIND_INSERT:    return str("EDITH_EDIT_KIND_INSERT");
        case EDITH_EDIT_KIND_DELETE:    return str("EDITH_EDIT_KIND_DELETE");
        default:                        return str("INVALID");
    }
}

static str edith_ansi_color_from_edit_kind(edith_edit_kind kind) {
    switch (kind) {
        case EDITH_EDIT_KIND_NONE:      return str(ANSI_FG_GRAY);
        case EDITH_EDIT_KIND_INSERT:    return str(ANSI_FG_GREEN);
        case EDITH_EDIT_KIND_DELETE:    return str(ANSI_FG_YELLOW);
        default:                        return str(ANSI_FG_BRIGHT_RED);
    }
}

////////////////////////////////////////////////////////////////
// rune: Edit batch

static void edith_edit_batch_push(edith_edit_batch *b, edith_edit_array a, arena *arena) {
    edith_edit_batch_node *node = arena_push_struct(arena, edith_edit_batch_node);
    node->edits = a;
    dlist_push(b, node);
}

typedef struct edith_edit_chain edith_edit_chain;
struct edith_edit_chain {
    i64 pos;
    str_list parts;
    edith_edit_chain *next;
    edith_edit_chain *prev;
};

typedef struct edith_edit_chain_list edith_edit_chain_list;
struct edith_edit_chain_list {
    edith_edit_chain *first;
    edith_edit_chain *last;
    i64 count;
};

static void edith_print_edit_array(edith_edit_array edits) {
    i64 i = 0;
    print("%%\n", edith_ansi_color_from_edit_kind(edits.kind), edith_str_from_edit_kind(edits.kind));
    for_array (edith_edit, edit, edits) {
        print("[%]\tpos:\t%\tlen:\t%\tdata:\t%(literal)\n", i, edit->pos, edit->data.len, edit->data);
        i++;
    }
    print("\n");
    print(ANSI_RESET);
}

static void edith_print_edit_batch(edith_edit_batch batch) {
    print("batch\n");
    for_list (edith_edit_batch_node, node, batch) {
        edith_print_edit_array(node->edits);
    }
}

static void edith_print_edit_chain_list(edith_edit_chain_list chains) {
    for_list (edith_edit_chain, chain, chains) {
        print("chain\tpos:\t%\ttotal_len:\t%\tparts: ", chain->pos, chain->parts.total_len);
        for_list (str_node, part, chain->parts) {
            print("%(literal)", part->v);
            if (part == chain->parts.last) {
                print("\n");
            } else {
                print(" → ");
            }
        }
    }
    print("\n");
}

static edith_edit_chain *edith_edit_chain_list_push(edith_edit_chain_list *list, edith_edit_chain *insert_before, arena *arena) {
    edith_edit_chain *node = arena_push_struct(arena, edith_edit_chain);

    if (insert_before) {
        dlist_insert_before(list, node, insert_before);
    } else {
        dlist_push(list, node);
    }

    list->count += 1;
    return node;
}

static edith_edit_batch edith_edit_batch_consolidate(edith_edit_batch batch, arena *arena) {
    // TODO(rune): Explanation
    edith_edit_batch result = { 0 };

    struct arena *temp = edith_thread_local_arena;
    arena_scope_begin(temp);

    // rune: Edit batch nodes -> edit chains
    edith_edit_kind curr_kind = 0;
    edith_edit_chain_list chains = { 0 };
    {
        for_list (edith_edit_batch_node, node, batch) {
            if (curr_kind == 0) {
                curr_kind = node->edits.kind;
            }

            // rune: Add edits existing chains or create new chain.
            switch (curr_kind) {
                case EDITH_EDIT_KIND_INSERT: {
                    edith_edit_chain *curr_chain = chains.first;
                    for_array (edith_edit, e, node->edits) {
                        while (curr_chain && curr_chain->pos + curr_chain->parts.total_len < e->pos) {
                            curr_chain = curr_chain->next;
                        }

                        if (curr_chain && curr_chain->pos + curr_chain->parts.total_len == e->pos) {
                            str_list_push(&curr_chain->parts, temp, e->data);
                        } else if (curr_chain && curr_chain->pos == e->pos) {
                            str_list_push_front(&curr_chain->parts, temp, e->data);
                        } else if (!curr_chain || curr_chain->pos + curr_chain->parts.total_len > e->pos) {
                            edith_edit_chain *new_chain = edith_edit_chain_list_push(&chains, curr_chain, temp);
                            new_chain->pos = e->pos;
                            str_list_push(&new_chain->parts, temp, e->data);
                            curr_chain = new_chain->next;
                        }
                    }
                } break;

                case EDITH_EDIT_KIND_DELETE: {
                    edith_edit_chain *curr_chain = chains.first;
                    for_array (edith_edit, e, node->edits) {
                        while (curr_chain && curr_chain->pos < e->pos) {
                            curr_chain = curr_chain->next;
                        }

                        if (curr_chain && curr_chain->pos == e->pos) {
                            str_list_push(&curr_chain->parts, temp, e->data);
                        } else if (curr_chain && curr_chain->pos == e->pos + e->data.len) {
                            str_list_push_front(&curr_chain->parts, temp, e->data);
                        } else if (!curr_chain || curr_chain->pos > e->pos) {
                            edith_edit_chain *new_chain = edith_edit_chain_list_push(&chains, curr_chain, temp);
                            new_chain->pos = e->pos;
                            str_list_push(&new_chain->parts, temp, e->data);
                            curr_chain = new_chain->next;
                        }
                    }
                } break;
            }

            // rune: Shift chains' starting posistions
            {
#if EDITH_DEBUG_PRINT_EDIT_BATCH_CONSOLIDATE
                print(ANSI_FG_MAGENTA "before shift\n" ANSI_RESET);
                edith_print_edit_chain_list(chains);
#endif

                i64 shift = 0;
                edith_edit_chain *curr_chain = chains.first;
                for_array (edith_edit, e, node->edits) {
                    while (curr_chain && curr_chain->pos <= e->pos) {
                        curr_chain->pos += shift;
                        curr_chain = curr_chain->next;
                    }

                    if (curr_kind == EDITH_EDIT_KIND_INSERT) shift += e->data.len;
                    if (curr_kind == EDITH_EDIT_KIND_DELETE) shift -= e->data.len;
                }

                while (curr_chain) { // TODO(rune): Cleanup
                    curr_chain->pos += shift;
                    curr_chain = curr_chain->next;
                }

#if EDITH_DEBUG_PRINT_EDIT_BATCH_CONSOLIDATE
                print(ANSI_FG_MAGENTA "after shift\n" ANSI_RESET);
                edith_print_edit_chain_list(chains);
#endif
            }

            // rune: Collapse chains into a final edit array and push them to the output arena
            if (node->next == null || node->next->edits.kind != curr_kind) {
                edith_edit_array dst = edith_edit_array_reserve(arena, chains.count, curr_kind);
                {
                    i64 shift = 0;
                    i64 i = 0;
                    for_list (edith_edit_chain, chain, chains) {
                        str s = str_list_concat(&chain->parts, arena);
                        dst.v[i].pos  = chain->pos + shift;
                        dst.v[i].data = s;
                        i += 1;

                        if (curr_kind == EDITH_EDIT_KIND_INSERT) shift -= chain->parts.total_len;
                        if (curr_kind == EDITH_EDIT_KIND_DELETE) shift += chain->parts.total_len;
                    }
                }
                edith_edit_batch_push(&result, dst, arena);

#if EDITH_DEBUG_PRINT_EDIT_BATCH_CONSOLIDATE
                edith_print_edit_array(dst);
#endif

                chains.first = 0;
                chains.last  = 0;
                chains.count = 0;
                curr_kind    = 0;
            }
        }
    }

    arena_scope_end(temp);

    return result;
}

////////////////////////////////////////////////////////////////
// rune: Gapbuffer for i64

static void edith_gb64_create(edith_gapbuffer64 *gb, str name) {
    zero_struct(gb);
    gb->buf = edith_heap_alloc(1024, name);
}

static void edith_gb64_destroy(edith_gapbuffer64 *gb) {
    edith_heap_free(gb->buf);
}

static void edith_gb64_reset(edith_gapbuffer64 *gb) {
    gb->gap_min = 0;
    gb->gap_max = gb->buflen;
}

static i64 edith_gb64_gaplen(edith_gapbuffer64 *gb) {
    i64 gaplen = gb->gap_max - gb->gap_min;
    return gaplen;
}

static i64 edith_gb64_count(edith_gapbuffer64 *gb) {
    i64 count = gb->buflen - edith_gb64_gaplen(gb);
    return count;
}

static i64 edith_gb64_rawidx_from_idx(edith_gapbuffer64 *gb, i64 idx) {
    assert(idx <= edith_gb64_count(gb));
    i64 ret;
    if (idx >= gb->gap_min) {
        ret = idx + edith_gb64_gaplen(gb);
    } else {
        ret = idx;
    }
    return ret;
}

static i64 *edith_gb64_get(edith_gapbuffer64 *gb, i64 idx) {
    i64 rawidx = edith_gb64_rawidx_from_idx(gb, idx);
    i64 *ptr = &gb->buf[rawidx];
    return ptr;
}

static void edith_gb64_shift_gap(edith_gapbuffer64 *gb, i64 to) {
    i64 gaplen = edith_gb64_gaplen(gb);

    clamp_assign(&to, 0, gb->buflen - gaplen);

    if (gb->gap_min < to) {
        i64 delta = to - gb->gap_min;
        memmove(gb->buf + gb->gap_min,
                gb->buf + gb->gap_max,
                delta * sizeof(i64));

        gb->gap_min += delta;
        gb->gap_max += delta;
    }

    if (gb->gap_min > to) {
        i64 delta = gb->gap_min - to;
        memcpy(gb->buf + gb->gap_max - delta,
               gb->buf + gb->gap_min - delta,
               delta * sizeof(i64));

        gb->gap_min -= delta;
        gb->gap_max -= delta;
    }
}

static void edith_gb64_reserve_gap(edith_gapbuffer64 *gb) {
    i64 gaplen = edith_gb64_gaplen(gb);
    if (gaplen == 0) {
        i64 old_buflen = gb->buflen;
        i64 new_buflen = max(gb->buflen * 2, 8);
        gb->buf = edith_heap_realloc(gb->buf, new_buflen * isizeof(i64));
        gb->gap_min = old_buflen;
        gb->gap_max = new_buflen;
        gb->buflen = new_buflen;
    }
}

static void edith_gb64_put(edith_gapbuffer64 *gb, i64 idx, i64 val) {
    assert(idx <= edith_gb64_count(gb));

    edith_gb64_reserve_gap(gb);
    edith_gb64_shift_gap(gb, idx);

    assert(edith_gb64_gaplen(gb) > 0);
    gb->buf[gb->gap_min] = val;
    gb->gap_min++;
}

static void edith_gb64_delete(edith_gapbuffer64 *gb, i64 idx, i64 count) {
    assert(idx < edith_gb64_count(gb));

    edith_gb64_shift_gap(gb, idx);
    gb->gap_max += count;
    clamp_assign(&gb->gap_max, 0, gb->buflen);
}

static i64 edith_gb64_binary_search_bot(edith_gapbuffer64 *gb, i64 needle) {
    i64 bot = 0;
    i64 top = edith_gb64_count(gb);
    while (top > bot + 1) {
        i64 mid = bot + (top - bot) / 2;
        if (needle < *edith_gb64_get(gb, mid)) {
            top = mid;
        } else {
            bot = mid;
        }
    }
    return bot;
}

static i64 edith_gb64_binary_search_top(edith_gapbuffer64 *gb, i64 needle) {
    i64 bot = 0;
    i64 top = edith_gb64_count(gb);
    while (top > bot) {
        i64 mid = bot + (top - bot) / 2;
        if (needle <= *edith_gb64_get(gb, mid)) {
            top = mid;
        } else {
            bot = mid + 1;
        }
    }
    return top;
}

static void edith_gb64_print(edith_gapbuffer64 *gb) {
    for_n (i64, i, gb->buflen) {
        print(ANSI_FG_GRAY "|" ANSI_RESET);
        if (i >= gb->gap_min && i < gb->gap_max) {
            print(ANSI_FG_GRAY "-" ANSI_RESET);
        } else {
            print(gb->buf[i]);
        }
    }

    print(ANSI_FG_GRAY "|" ANSI_RESET);
    print("\n");
}

////////////////////////////////////////////////////////////////
// rune: Shifter

#define EDITH_SHIFT_AFTER_EDITS_TEMPLATE                                            \
    if (edits.count > 0) {                                                          \
        switch (edits.kind) {                                                       \
            case EDITH_EDIT_KIND_INSERT: {                                          \
                i64 min = 0;                                                        \
                i64 max = 0;                                                        \
                i64 k = 0;                                                          \
                i64 shift = 0;                                                      \
                for (i64 i = 0; i < edits.count; i++) {                             \
                    min = edits.v[i].pos;                                           \
                    max = i + 1 < edits.count ? edits.v[i + 1].pos : I64_MAX;       \
                    shift += edits.v[i].data.len;                                   \
                                                                                    \
                    while (k < num && *SHIFT_AFTER_EDITS_PTR(k) <= min) {           \
                        k++;                                                        \
                    }                                                               \
                                                                                    \
                    while (k < num && *SHIFT_AFTER_EDITS_PTR(k) <= max) {           \
                        *SHIFT_AFTER_EDITS_PTR(k) += shift;                         \
                        k++;                                                        \
                    }                                                               \
                }                                                                   \
            } break;                                                                \
                                                                                    \
            case EDITH_EDIT_KIND_DELETE: {                                          \
                i64 min = 0;                                                        \
                i64 max = 0;                                                        \
                i64 k = 0;                                                          \
                i64 shift = 0;                                                      \
                for (i64 i = 0; i < edits.count; i++) {                             \
                    min = edits.v[i].pos;                                           \
                    max = i + 1 < edits.count ? edits.v[i + 1].pos : I64_MAX;       \
                    shift -= edits.v[i].data.len;                                   \
                                                                                    \
                    while (k < num && *SHIFT_AFTER_EDITS_PTR(k) <= min) {           \
                        k++;                                                        \
                    }                                                               \
                                                                                    \
                    while (k < num && *SHIFT_AFTER_EDITS_PTR(k) <= max) {           \
                        *SHIFT_AFTER_EDITS_PTR(k) += shift;                         \
                        k++;                                                        \
                    }                                                               \
                }                                                                   \
            } break;                                                                \
                                                                                    \
            default: {                                                              \
                assert(false);                                                      \
            } break;                                                                \
        }                                                                           \
    }

static void edith_shift_after_edits_flat(i64 *flat, i64 num, edith_edit_array edits) {
#define SHIFT_AFTER_EDITS_PTR(k) (&flat[k])
    EDITH_SHIFT_AFTER_EDITS_TEMPLATE
#undef SHIFT_AFTER_EDITS_PTR
}

static void edith_shift_after_edits_gb64(edith_gapbuffer64 *gb, edith_edit_array edits) {
    i64 num = edith_gb64_count(gb);
#define SHIFT_AFTER_EDITS_PTR(k) (edith_gb64_get(gb, k))
    EDITH_SHIFT_AFTER_EDITS_TEMPLATE
#undef SHIFT_AFTER_EDITS_PTR
}

static void edith_shift_after_edits_struct(void *structs, i64 num, i64 stride, i64 member_offset, edith_edit_array edits) {
#define SHIFT_AFTER_EDITS_PTR(k) ((i64 *)(((u8 *)structs) + (k * stride) + member_offset))
    EDITH_SHIFT_AFTER_EDITS_TEMPLATE
#undef SHIFT_AFTER_EDITS_PTR
}
