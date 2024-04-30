////////////////////////////////////////////////////////////////
// rune: Textbuf query

static edith_search_result_array edith_submit_textbuf_query(u128 key, str needle, edith_textbuf *textbuf, bool *completed) {
    edith_search_result_array results = { 0 };
    edith_query_cache *cache = &edith_global_query_cache;
    *completed = false;

    if (needle.len > 0 && !u128_is_zero(key)) {
        bool already_requested = false;
        bool found_slot = false;
        os_mutex_scope(cache->mutex) {
            // rune: Look for cached response
            for_list (edith_query_slot, slot, cache->requests) {
                if (str_eq(needle, slot->needle) && textbuf == slot->textbuf) {
                    slot->accessed_frame_count = edith_global_frame_counter;
                    results                    = slot->results;
                    *completed                 = slot->completed;
                    already_requested          = true;
                    break;
                }
            }

            // rune: Request new results
            if (already_requested == false) {
                edith_query_slot *slot     = edith_query_slot_alloc(cache);
                slot->needle               = arena_copy_str(cache->arena, needle);
                slot->textbuf              = textbuf;
                slot->key                  = key;
                slot->accessed_frame_count = edith_global_frame_counter;
                edith_query_slot_list_push(&cache->requests, slot);

                os_cond_signal(cache->cond);
            }
        }
    }

    return results;
}

static edith_search_result_array edith_search_results_from_key(u128 key, bool *completed, u32 timeout_ms) {
    edith_search_result_array ret = { 0 };
    edith_query_cache *cache = &edith_global_query_cache;
    if (!u128_is_zero(key)) {
        os_mutex_scope(cache->mutex) {
            edith_query_slot *found = null;

            for_list (edith_query_slot, slot, cache->requests) {
                if (u128_eq(slot->key, key)) {
                    found = slot;
                    break;
                }
            }

            if (found && found->completed == false) {
                os_cond_wait(cache->cond, cache->mutex, 1);
            }

            for_list (edith_query_slot, slot, cache->requests) {
                if (u128_eq(slot->key, key)) {
                    found = slot;
                    break;
                }
            }

            if (found) {
                found->accessed_frame_count = edith_global_frame_counter;
                ret = found->results;
                if (completed) {
                    *completed = found->completed;
                }
            }
        }
    }
    return ret;
}

static i64_range edith_search_result_idx_from_pos(edith_search_result_array a, i64 pos) {
    i64_range ret = { -1, -1 };

    i64 min = 0;
    i64 max = a.count - 1;
    while (min <= max) {
        i64 mid = min + (max - min) / 2;
        if (a.v[mid].range.min <= pos) {
            ret.min = mid;
            min = mid + 1;
        } else {
            ret.max = mid;
            max = mid - 1;
        }
    }

    return ret;
}

////////////////////////////////////////////////////////////////
// rune: Query slot list

static void edith_query_slot_list_push(edith_query_slot_list *list, edith_query_slot *node) {
    dlist_push(list, node);
    list->count += 1;
}

static edith_query_slot *edith_query_slot_list_pop(edith_query_slot_list *list) {
    edith_query_slot *node = list->last;
    edith_query_slot_list_remove(list, node);
    return node;
}

static void edith_query_slot_list_remove(edith_query_slot_list *list, edith_query_slot *node) {
    if (node) {
        list->count -= 1;
        dlist_remove(list, node);
    }
}

////////////////////////////////////////////////////////////////
// rune: Query slot allocation

static edith_query_slot *edith_query_slot_alloc(edith_query_cache *cache) {
    edith_query_slot *slot = edith_query_slot_list_pop(&cache->free_slots);
    if (slot) {
        memset(slot, 0, sizeof(edith_query_slot));
    } else {
        slot = arena_push_struct(cache->arena, edith_query_slot);
    }
    return slot;
}

static void edith_query_slot_free(edith_query_cache *cache, edith_query_slot *slot) {
    if (slot) {
        if (slot->arena) edith_arena_destroy(slot->arena);
        slot->arena = null;
        edith_query_slot_list_push(&cache->free_slots, slot);
    }
}

////////////////////////////////////////////////////////////////
// rune: Query thread

static void edith_query_thread_entry_point(void *param) {
    edith_thread_local_arena = edith_arena_create_default(str("query_thread/thread_local_arena"));

    edith_query_cache *cache = &edith_global_query_cache;

    cache->mutex= os_mutex_create();
    cache->cond  = os_cond_create();
    cache->arena = edith_arena_create_default(str("query_cache"));

    i64 last_gc_frame_count = 0;

    while (1) {
        edith_query_slot *handle_slot = null;
        while (handle_slot == null) {
            os_mutex_scope(cache->mutex) {
                // rune: Look for incoming requests
                for_list (edith_query_slot, it, cache->requests) {
                    if (it->completed == false) {
                        handle_slot = it;
                        break;
                    }
                }

                // rune: Garbage collect
                if (handle_slot == null) {
                    if (last_gc_frame_count != edith_global_frame_counter) {
                        last_gc_frame_count = edith_global_frame_counter;

                        for_list (edith_query_slot, it, cache->requests) {
                            if (edith_global_frame_counter - it->accessed_frame_count > 1) {
                                edith_query_slot_list_remove(&cache->requests, it);
                                edith_query_slot_free(cache, it);
                            }
                        }
                    }

                    os_cond_wait(cache->cond, cache->mutex, 1000);
                }
            }
        }

        if (handle_slot) {
            u64 t_begin = os_get_performance_timestamp();

            edith_query_slot *slot = handle_slot;

            arena *perm = edith_arena_create_default(edith_tprint("query_slot %", slot->needle));
            arena *temp = edith_temp();
            arena_scope_begin(temp);

            edith_textbuf *textbuf = slot->textbuf;
            edith_gapbuffer *gb = &textbuf->gb;

            // rune: Search through text buffer are push results to list
            edith_search_result_list list = { 0 };
            if (slot->needle.len > 0) {
                i64 pos = 0;
                while (true) {
                    i64_range search_range = { pos, I64_MAX };
                    i64 occurence = edith_next_occurence_in_gapbuffer(gb, search_range, slot->needle, DIR_FORWARD, false, temp);
                    if (occurence != -1) {
                        i64_range occurence_range = { occurence, occurence + slot->needle.len };

                        edith_search_result_node *node = arena_push_struct(temp, edith_search_result_node);
                        node->result.range = occurence_range;

                        slist_push(&list, node);
                        list.count += 1;

                        pos = occurence + 1;
                    } else {
                        break;
                    }
                }
            }

            // rune: Pack list into array
            edith_search_result_array array = { 0 };
            array.v = arena_push_array(perm, edith_search_result, list.count);
            array.count = list.count;
            i64 i = 0;
            for_list (edith_search_result_node, node, list) {
                array.v[i++] = node->result;
            }

            arena_scope_end(temp);

            // rune: Mark slot as completed
            os_mutex_scope(cache->mutex) {
                slot->arena = perm;
                slot->results = array;
                slot->accessed_frame_count = edith_global_frame_counter;
                slot->completed = true;

                os_cond_signal_all(cache->cond);
            }
        }
    }
}
