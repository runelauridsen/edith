static atlas_shelf *atlas_alloc_shelf(atlas *atlas) {
    atlas_shelf *ret = atlas->shelf_freelist.first;
    slist_pop_front(&atlas->shelf_freelist);

    if (ret) {
        zero_struct(ret);
    } else {
        ret = arena_push_struct(atlas->arena, atlas_shelf);
    }

    return ret;
}

static atlas_slot *atlas_alloc_slot(atlas *atlas) {
    atlas_slot *ret = atlas->slot_freelist.first;
    slist_pop_front(&atlas->slot_freelist);

    if (ret) {
        ret->gen++;
        ret->next = null;
    }

    return ret;
}

static void atlas_init(atlas *atlas, uvec2 dim, u32 max_slots, arena *arena) {
    zero_struct(atlas);

    atlas->arena      = arena;
    atlas->dim        = dim;
    atlas->slot_count = max_slots;
    atlas->slots      = arena_push_array(arena, atlas_slot, max_slots);
    atlas->pixels     = arena_push_array(arena, u8, dim.x * dim.y);

    atlas_shelf *root_shelf = atlas_alloc_shelf(atlas);
    root_shelf->dim_y = atlas->dim.y;
    dlist_push(&atlas->shelves, root_shelf);

    for_n (u32, i, atlas->slot_count) {
        slist_push(&atlas->slot_freelist, &atlas->slots[i]);
    }
}

static void atlas_next_timestamp(atlas *atlas) {
    atlas->curr_timestamp++;
}

static bool atlas_get_slot(atlas *atlas, atlas_slot_key key, urect *rect) {
    bool ret = false;

    assert(key.idx < atlas->slot_count);
    atlas_slot *slot = &atlas->slots[key.idx];
    if (slot->gen == key.gen) {
        *rect = slot->rect;
        slot->timestamp = atlas->curr_timestamp;
        ret = true;
    }
    return ret;
}

static rect atlas_get_uv(atlas *atlas, urect r) {
    rect uv = { 0 };

    uv.p0.x = (f32)(r.x0) / (f32)(atlas->dim.x);
    uv.p0.y = (f32)(r.y0) / (f32)(atlas->dim.y);
    uv.p1.x = (f32)(r.x1) / (f32)(atlas->dim.x);
    uv.p1.y = (f32)(r.y1) / (f32)(atlas->dim.y);

    return uv;
}

static inline bool atlas_shelf_can_fit(atlas *atlas, atlas_shelf *shelf, uvec2 dim) {
    bool ret = ((dim.y <= shelf->dim_y) &&
                (dim.x <= atlas->dim.x - shelf->used_x));
    return ret;
}

static atlas_shelf *atlas_shelf_merge(atlas *atlas, atlas_shelf *a, atlas_shelf *b) {
    assert(a != b);
    assert(a->used_x == 0);
    assert(b->used_x == 0);
    assert(a->slots.first == null);
    assert(a->slots.last  == null);
    assert(b->slots.first == null);
    assert(b->slots.last  == null);
    assert(a->base_y != b->base_y);

    // NOTE(rune): Always dealloc topmost shelf.
    atlas_shelf *bot = a->base_y < b->base_y ? a : b;
    atlas_shelf *top = a->base_y < b->base_y ? b : a;

    bot->dim_y += top->dim_y;

    dlist_remove(&atlas->shelves, top);
    slist_push(&atlas->shelf_freelist, top);

    return bot;
}

static atlas_shelf *atlas_shelf_split(atlas *atlas, atlas_shelf *split, u32 y) {
    atlas_shelf *ret = null;

    if (split->dim_y > y) {
        ret = atlas_alloc_shelf(atlas);

        ret->dim_y = y;
        ret->base_y = split->base_y;

        split->base_y += y;
        split->dim_y -= y;

        dlist_insert_before(&atlas->shelves, ret, split);
    } else if (split->dim_y == y) {
        ret = split; // NOTE(rune): Exact match -> no need to split.
    } else {
        assert(!"Not enough atlas space.");
    }

    return ret;
}

static atlas_shelf *atlas_shelf_reset_and_merge(atlas *atlas, atlas_shelf *reset) {
    reset->used_x = 0;

    if (reset->slots.first) {
        for_list (atlas_slot, slot, reset->slots) {
            slot->gen += 69; // NOTE(rune): Invalidate all atlas_slot_keys that refer to this slot.
        }

        slist_join(&atlas->slot_freelist, &reset->slots);
    }

    reset->slots.first = null;
    reset->slots.last  = null;

    if (reset->next && reset->next->used_x == 0) reset = atlas_shelf_merge(atlas, reset, reset->next);
    if (reset->prev && reset->prev->used_x == 0) reset = atlas_shelf_merge(atlas, reset, reset->prev);

    return reset;
}

static atlas_shelf *atlas_prune(atlas *atlas, u32 until_y) {
    atlas_shelf *ret = null;
    atlas_shelf *stale = null;
    u64 min_timestamp_found = 0;
    do {
        // rune: Find shelf with lowest last accessed generation.
        stale = null;
        min_timestamp_found = atlas->curr_timestamp;
        for_list (atlas_shelf, it, atlas->shelves) {
            // rune: Find timestamp of most recently accessed slot in this shelf.
            u32 it_timestamp = 0;
            for_list (atlas_slot, slot, it->slots) {
                max_assign(&it_timestamp, slot->timestamp);
            }

            // rune: We assume that caller has already checked empty shelves.
            if (it->used_x > 0 && it_timestamp < min_timestamp_found) {
                stale = it;
                min_timestamp_found = it_timestamp;
            }
        }

        // rune: Reset shelf with lowest last accessed generation.
        if (stale) {
            stale = atlas_shelf_reset_and_merge(atlas, stale);

            if (stale->dim_y >= until_y) {
                ret = atlas_shelf_split(atlas, stale, until_y);
            }
        }

        // rune: Stop when there are no more stale shelves, or when we found/merged a shelf that fits until_y.
    } while (stale && !ret);

    return ret;
}

static atlas_slot_key atlas_new_slot(atlas *atlas, uvec2 dim) {
    atlas_slot_key key         = { 0 };
    atlas_slot  *slot          = null;
    atlas_shelf *best_nonempty = null;
    atlas_shelf *best_empty    = null;
    uvec2 rounded_dim          = uvec2(dim.x, dim.y - (dim.y - 1) % 8 + (8 - 1));

    // NOTE(rune): Find the shelves where we waste the least amount of y-space.
    for_list (atlas_shelf, shelf, atlas->shelves) {

        bool fits = ((dim.y <= shelf->dim_y) && (dim.x <= atlas->dim.x - shelf->used_x));
        if (fits) {
            u32 wasted = shelf->dim_y - rounded_dim.y;

            if (shelf->used_x != 0) {
                if (best_nonempty == null || wasted < best_nonempty->dim_y - rounded_dim.y) {
                    best_nonempty = shelf;
                }
            } else {
                if (best_empty == null || wasted < best_empty->dim_y - rounded_dim.y) {
                    best_empty = shelf;
                }
            }
        }
    }

    atlas_shelf *best = null;

    // NOTE(rune): If it fits in an existing shelf that is currently nonempty, just continue to use that shelf,
    // otherwise we begin a new shelf, by splitting the shelf with least wasted y space.

    if (best_nonempty) {
        best = best_nonempty;
    } else if (best_empty) {
        best = atlas_shelf_split(atlas, best_empty, rounded_dim.y);
    } else {
        best = atlas_prune(atlas, rounded_dim.y);
    }

    if (best) {
        slot = atlas_alloc_slot(atlas);
        if (slot) {
            slist_push(&best->slots, slot);
            slot->rect.x0 = best->used_x;
            slot->rect.y0 = best->base_y;
            slot->rect.x1 = best->used_x + dim.x;
            slot->rect.y1 = best->base_y + dim.y;

            slot->timestamp = atlas->curr_timestamp;

            best->used_x += dim.x;

            key.idx = (u32)(slot - atlas->slots);
            key.gen = slot->gen;
            assert(key.gen != 0);
        }
    }

    return key;
}

static void atlas_reset(atlas *atlas) {
    for_list (atlas_shelf, it, atlas->shelves) {
        if (it->slots.first)  slist_join(&atlas->slot_freelist, &it->slots);
    }

    slist_join(&atlas->shelf_freelist, &atlas->shelves);

    atlas->shelves.first = null;
    atlas->shelves.last  = null;

    atlas_shelf *root_shelf = atlas_alloc_shelf(atlas);
    root_shelf->dim_y = atlas->dim.y;
    dlist_push(&atlas->shelves, root_shelf);
}
