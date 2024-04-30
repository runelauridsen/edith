static atlas_shelf *atlas_alloc_shelf(atlas *atlas) {
    atlas_shelf *ret = atlas->shelf_freelist.first;
    slist_pop_front(&atlas->shelf_freelist);

    if (ret) {
        zero_struct(ret);
    } else if (atlas->shelf_storage_used < atlas->shelf_storage_count) {
        ret = &atlas->shelf_storage[atlas->shelf_storage_used++];
        zero_struct(ret);
    }

    return ret;
}

static atlas_node *atlas_alloc_node(atlas *atlas) {
    atlas_node *ret = atlas->node_freelist.first;
    slist_pop_front(&atlas->node_freelist);

    if (ret) {
        zero_struct(ret);
    } else if (atlas->node_storage_used < atlas->node_storage_count) {
        ret = &atlas->node_storage[atlas->node_storage_used++];
        zero_struct(ret);
    }

    return ret;
}

static bool atlas_create(atlas *atlas, ivec2 dim, i32 max_nodes) {
    zero_struct(atlas);

    atlas->shelf_storage       = heap_alloc(sizeof(atlas_shelf) * 256);
    atlas->shelf_storage_count = 256;

    atlas->node_storage       = heap_alloc(sizeof(atlas_node) * max_nodes);
    atlas->node_storage_count = max_nodes;

    i64 pixel_size       = dim.x * dim.y;
    atlas->pixels        = heap_alloc(pixel_size);
    atlas->pixel_size    = pixel_size;
    atlas->dim           = dim;

    bool ret = (atlas->shelf_storage &&
                atlas->node_storage &&
                atlas->pixels);

    if (ret) {
        atlas_shelf *root_shelf = atlas_alloc_shelf(atlas);
        root_shelf->dim_y = atlas->dim.y;
        dlist_push(&atlas->shelves, root_shelf);
    } else {
        atlas_destroy(atlas);
    }

    return ret;
}

static void atlas_destroy(atlas *atlas) {
    heap_free(atlas->pixels);
    heap_free(atlas->shelf_storage);
    heap_free(atlas->node_storage);
    zero_struct(atlas);
}

static atlas_node *atlas_get_node(atlas *atlas, u64 key) {
    // TODO(rune): Profile! Hashtable lookup?

    atlas_node *ret = null;

    for_list (atlas_shelf, shelf, atlas->shelves) {
        for_list (atlas_node, node, shelf->nodes) {
            if (node->key == key) {
                node->last_accessed  = atlas->current_generation;
                shelf->last_accessed = atlas->current_generation;

                ret = node;
            }
        }
    }

    return ret;
}

static rect atlas_get_node_uv(atlas *atlas, atlas_node *node) {
    rect ret = { 0 };

    if (node) {
        ret.p0.x = (f32)(node->rect.x0) / (f32)(atlas->dim.x);
        ret.p0.y = (f32)(node->rect.y0) / (f32)(atlas->dim.y);
        ret.p1.x = (f32)(node->rect.x1) / (f32)(atlas->dim.x);
        ret.p1.y = (f32)(node->rect.y1) / (f32)(atlas->dim.y);
    }

    return ret;
}

static inline bool atlas_shelf_can_fit(atlas *atlas, atlas_shelf *shelf, ivec2 dim) {
    bool ret = ((dim.y <= shelf->dim_y) &&
                (dim.x <= atlas->dim.x - shelf->used_x));
    return ret;
}

static atlas_shelf *atlas_shelf_merge(atlas *atlas, atlas_shelf *a, atlas_shelf *b) {
    assert(a != b);
    assert(a->used_x == 0);
    assert(b->used_x == 0);
    assert(a->nodes.first == null);
    assert(b->nodes.first == null);
    assert(a->nodes.last  == null);
    assert(b->nodes.last  == null);
    assert(a->base_y != b->base_y);

    // NOTE(rune): Always dealloc topmost shelf.
    atlas_shelf *bot = a->base_y < b->base_y ? a : b;
    atlas_shelf *top = a->base_y < b->base_y ? b : a;

    bot->dim_y += top->dim_y;

    dlist_remove(&atlas->shelves, top);
    slist_push(&atlas->shelf_freelist, top);

    return bot;
}

static atlas_shelf *atlas_shelf_split(atlas *atlas, atlas_shelf *split, i32 y) {
    atlas_shelf *ret = null;

    if (split->dim_y > y) {
        ret = atlas_alloc_shelf(atlas);

        ret->dim_y = y;
        ret->base_y = split->base_y;

        split->base_y += y;
        split->dim_y -= y;

        dlist_insert_before(&atlas->shelves, ret, split);
    } else if (split->dim_y == y) {
        ret = split; // NOTE(rune): No need to split.
    } else {
        assert(!"Not enough atlas space.");
    }

    return ret;
}

static atlas_shelf *atlas_shelf_reset_and_merge(atlas *atlas, atlas_shelf *reset) {
    reset->used_x = 0;
    reset->last_accessed = 0;

    if (reset->nodes.first) {
        slist_join(&atlas->node_freelist, &reset->nodes);
    }
    reset->nodes.first = null;
    reset->nodes.last  = null;

    if (reset->next && reset->next->used_x == 0) reset = atlas_shelf_merge(atlas, reset, reset->next);
    if (reset->prev && reset->prev->used_x == 0) reset = atlas_shelf_merge(atlas, reset, reset->prev);

    return reset;
}

static atlas_shelf *atlas_prune(atlas *atlas, i32 until_y) {
    atlas_shelf *ret = null;
    atlas_shelf *stale = null;
    i64 stale_last_accessed = 0;
    do {
        // NOTE(rune): Find shelf with lowest last accessed generation.
        stale = null;
        stale_last_accessed = atlas->current_generation;
        for_list (atlas_shelf, it, atlas->shelves) {
            // NOTE(rune): We assume that caller has already checked empty shelves.
            if (it->used_x > 0 && it->last_accessed < stale_last_accessed) {
                stale = it;
                stale_last_accessed = it->last_accessed;
            }
        }

        // NOTE(rune): Reset shelf with lowest last accessed generation.
        if (stale) {
            stale = atlas_shelf_reset_and_merge(atlas, stale);

            if (stale->dim_y >= until_y) {
                ret = atlas_shelf_split(atlas, stale, until_y);
            }
        }

        // NOTE(rune): Stop when there are no more stale shelves,
        // or when we found/merged a shelf that fits until_y.
    } while (stale && !ret);

    return ret;
}

static atlas_node *atlas_new_node(atlas *atlas, u64 key, ivec2 dim) {
    atlas_node  *ret           = null;
    atlas_shelf *best_nonempty = null;
    atlas_shelf *best_empty    = null;
    ivec2 rounded_dim          = ivec2(dim.x, dim.y - (dim.y - 1) % 8 + (8 - 1));

    // NOTE(rune): Find the shelves where we waste the least amount of y-space.
    for_list (atlas_shelf, shelf, atlas->shelves) {
        i32 wasted = shelf->dim_y - rounded_dim.y;
        bool fits = atlas_shelf_can_fit(atlas, shelf, rounded_dim);

        if (fits && shelf->used_x) {
            if (best_nonempty == null || wasted < best_nonempty->dim_y - rounded_dim.y) {
                best_nonempty = shelf;
            }
        }

        if (fits && !shelf->used_x) {
            if (best_empty == null || wasted < best_empty->dim_y - rounded_dim.y) {
                best_empty = shelf;
            }
        }
    }

    atlas_shelf *best = null;

    // NOTE(rune): If it fits in an existing shelf that is currently nonempty, just continue to use that shelf,
    // otherwise we begin a new shelf, by splitting the shelf with least wasted y space.
    // TODO(rune): This approach could lead to a lot of short shelves. Think of another strategy to pick shelf to split?

    if (best_nonempty) {
        best = best_nonempty;
    } else if (best_empty) {
        best = atlas_shelf_split(atlas, best_empty, rounded_dim.y);
    } else {
        best = atlas_prune(atlas, rounded_dim.y);
    }

    if (best) {
        ret = atlas_alloc_node(atlas);
        if (ret) {
            slist_push(&best->nodes, ret);
            ret->rect.x0 = best->used_x;
            ret->rect.y0 = best->base_y;
            ret->rect.x1 = best->used_x + dim.x;
            ret->rect.y1 = best->base_y + dim.y;
            ret->key = key;

            ret->last_accessed = atlas->current_generation;
            best->last_accessed = atlas->current_generation;

            best->used_x += dim.x;
        }
    }

    return ret;
}

static void atlas_reset(atlas *atlas) {
    for_list (atlas_shelf, it, atlas->shelves) {
        if (it->nodes.first)  slist_join(&atlas->node_freelist, &it->nodes);
    }

    slist_join(&atlas->shelf_freelist, &atlas->shelves);

    atlas->shelves.first = null;
    atlas->shelves.last  = null;

    atlas_shelf *root_shelf = atlas_alloc_shelf(atlas);
    root_shelf->dim_y = atlas->dim.y;
    dlist_push(&atlas->shelves, root_shelf);
}
