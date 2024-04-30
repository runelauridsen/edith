////////////////////////////////////////////////////////////////
// rune: Linemarks

static i64 edith_pos_from_linenumber(edith_linemarks *lms, i64 linenumber) {
    YO_PROFILE_BEGIN(edith_pos_from_linenumber);
    i64 line_count = edith_gb64_count(&lms->pos_buf);
    i64 ret = 0;
    if (linenumber < 0) {
        ret = 0;
    } else if (linenumber > line_count) {
        ret = *edith_gb64_get(&lms->pos_buf, line_count - 1);
    } else {
        ret = *edith_gb64_get(&lms->pos_buf, linenumber);
    }
    YO_PROFILE_END(edith_pos_from_linenumber);
    return ret;
}

static i64 edith_linenumber_from_pos(edith_linemarks *lms, i64 pos) {
    YO_PROFILE_BEGIN(edith_linenumber_from_pos);
    i64 ret = edith_gb64_binary_search_bot(&lms->pos_buf, pos);
    YO_PROFILE_END(edith_linenumber_from_pos);
    return ret;
}

static void edith_insert_linemarker_at_idx(edith_linemarks *lms, i64 idx, i64 pos) {
    edith_gb64_put(&lms->pos_buf, idx, pos);
}

static void edith_delete_linemarkers_at_idx(edith_linemarks *lms, i64 idx, i64 count) {
    if (count > 0) {
        edith_gb64_delete(&lms->pos_buf, idx, count);
    }
}

static i64 edith_insert_linemarkers_after_pos(edith_linemarks *lms, i64 first_lm, i64 after_pos, str data) {
    i64 num_added_lines = 0;
    for_n (i64, i, data.len) {
        if (data.v[i] == '\n') {
            i64 insert_idx = first_lm + num_added_lines;
            i64 insert_pos = after_pos + i + 1;
            edith_insert_linemarker_at_idx(lms, insert_idx, insert_pos);

            num_added_lines++;
        }
    }

    return num_added_lines;
}

static i64 edith_linemarks_delete_until_pos(edith_linemarks *lms, i64 first_lm, i64 until_pos) {
    i64 num = edith_gb64_count(&lms->pos_buf);
    i64 last_lm = first_lm;
    for (; last_lm < num; last_lm++) {
        if (i64(*edith_gb64_get(&lms->pos_buf, last_lm)) > until_pos) {
            break;
        }
    }

    i64 num_deleted = last_lm - first_lm;
    edith_delete_linemarkers_at_idx(lms, first_lm, num_deleted);
    return num_deleted;
}

static void edith_linemarks_add_or_remove(edith_linemarks *lms, edith_edit_array edits, edith_edit_kind kind) {
    if (edits.count > 0) {
        switch (kind) {
            case EDITH_EDIT_KIND_INSERT: {
                i64 shift = 0;
                i64 at = 0;
                i64 k = 0;
                for (i64 i = 0; i < edits.count; i++) {
                    at = edits.v[i].pos + shift;

                    while (k < edith_gb64_count(&lms->pos_buf) && *edith_gb64_get(&lms->pos_buf, k) <= at) {
                        k++;
                    }

                    str data = edits.v[i].data;
                    for (i64 j = 0; j < data.len; j++) {
                        if (data.v[j] == '\n') {
                            edith_insert_linemarker_at_idx(lms, k, at + j + 1);
                            k++;
                        }
                    }

                    shift += data.len;
                }
            } break;

            case EDITH_EDIT_KIND_DELETE: {
                i64 shift = 0;
                i64 at = 0;
                i64 k = 0;
                for (i64 i = 0; i < edits.count; i++) {
                    at = edits.v[i].pos + shift;

                    while (k < edith_gb64_count(&lms->pos_buf) && *edith_gb64_get(&lms->pos_buf, k) <= at) {
                        k++;
                    }

                    str data = edits.v[i].data;
                    edith_linemarks_delete_until_pos(lms, k, at + data.len);

                    //shift -= data.len;
                }
            } break;

            default: {
                assert(false);
            } break;
        }
    }
}

static void edith_debug_print_linemarks(edith_linemarks *lms) {
    i64 num = edith_gb64_count(&lms->pos_buf);
    println("======================================");
    for_range (i64, i, 0, num) {
        println("i = % \t pos = % \t linenumber = %", i, *edith_gb64_get(&lms->pos_buf, i), i + 1);
    }
    edith_gb64_print(&lms->pos_buf);
}

static void edith_linemarks_apply_edits(edith_linemarks *lms, edith_edit_array edits) {
    YO_PROFILE_BEGIN(edith_linemarks_apply_edits);

    if (edith_gb64_count(&lms->pos_buf) == 0) {
        edith_edit *initial_edit = edith_edit_array_pop(&edits);
        assert(edits.kind == EDITH_EDIT_KIND_INSERT);
        assert(initial_edit->pos == 0);
        edith_insert_linemarker_at_idx(lms, 0, 0);
        edith_insert_linemarkers_after_pos(lms, 1, 0, initial_edit->data);
    }

    if (edits.kind == EDITH_EDIT_KIND_DELETE) {
        edith_linemarks_add_or_remove(lms, edits, EDITH_EDIT_KIND_DELETE);
    }

    edith_shift_after_edits_gb64(&lms->pos_buf, edits);
    if (edits.kind == EDITH_EDIT_KIND_INSERT) {
        edith_linemarks_add_or_remove(lms, edits, EDITH_EDIT_KIND_INSERT);
    }

    YO_PROFILE_END(edith_linemarks_apply_edits);
}

static bool edith_linemarks_sanity_check(edith_linemarks *lms, edith_gapbuffer *gb) {
#ifdef NDEBUG
    return false;
#endif

    typedef struct landmark landmark;
    typedef_darray(landmark);
    struct landmark {
        i64 pos;
        i64 linenumber;
    };

    darray(landmark) marks = { 0 };
    darray_reset(&marks);
    darray_put(&marks, ((landmark) { 0, 0 }));

    i64 linenumber = 0;
    edith_doc_iter iter = { 0 };
    edith_doc_iter_jump(&iter, gb, 0);

    while (iter.valid) {
        bool was_newline = false;
        if (iter.codepoint == '\n') {
            was_newline = true;
            linenumber++;
        }

        edith_doc_iter_next(&iter, DIR_FORWARD);
        if (was_newline) {
            darray_put(&marks, ((landmark) { iter.pos, linenumber }));
        }
    }

    i64 num = edith_gb64_count(&lms->pos_buf);
#ifndef RUN_TESTS
#if 0
    println("======================================");
    for_range (i64, i, 0, num) {
        println("i = % \t pos = % \t linenumber = %", i, *gb64_get(&lms->pos_buf, i), i + 1);
    }
    gb64_print(&lms->pos_buf);
#endif
#endif

#ifdef RUN_TESTS
    bool ret = num == marks.count;
    if (ret) {
        for (i64 i = 0; i < num; i++) {
            i64 lms_pos = *edith_gb64_get(&lms->pos_buf, i);
            i64 mark_pos = marks.v[i].pos;
            ret &= (lms_pos == mark_pos);
            //ret &= (marks.v[i].linenumber == lms->linenumber_buf[i]);
        }
    }

    return ret;
#else
    assert(num == marks.count);
    for (i64 i = 0; i < num; i++) {
        i64 lms_pos = *edith_gb64_get(&lms->pos_buf, i);
        i64 mark_pos = marks.v[i].pos;
        assert(lms_pos == mark_pos);
        // assert(marks.v[i].linenumber == lms->linenumber_buf[i]);
    }

    darray_destroy(&marks);
    return true;
#endif
}
