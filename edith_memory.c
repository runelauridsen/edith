////////////////////////////////////////////////////////////////
// rune: Temporary storage

static arena *edith_temp(void) {
    return edith_thread_local_arena;
}

static str edith_tprint_(args args) {
    str s = arena_print_args(edith_thread_local_arena, args);
    return s;
}

////////////////////////////////////////////////////////////////
// rune: Allocation tracking

static void edith_mem_init(void) {
    edith_thread_local_arena      = arena_create_default();
    edith_g_mem_track_arena  = arena_create_default();
    edith_g_mem_track_mutex  = os_mutex_create();

    edith_mem_track(edith_g_mem_track_arena, EDITH_MEM_TRACK_KIND_ARENA, EDITH_MEM_TRACK_ACTION_ALLOC)->name = str("mem track");
    edith_mem_track(edith_thread_local_arena, EDITH_MEM_TRACK_KIND_ARENA, EDITH_MEM_TRACK_ACTION_ALLOC)->name = str("thread local");
}

static edith_mem_track_node *edith_mem_track(void *ptr, edith_mem_track_kind kind, edith_mem_track_action action) {
    edith_mem_track_node *node = null;

    os_mutex_scope(edith_g_mem_track_mutex) {
        switch (action) {
            case EDITH_MEM_TRACK_ACTION_ALLOC: {
                node = arena_push_struct(edith_g_mem_track_arena, edith_mem_track_node);
                node->kind = kind;
                node->ptr = ptr;

                dlist_push(&edith_g_mem_track_list, node);
            } break;

            case EDITH_MEM_TRACK_ACTION_REALLOC:
            case EDITH_MEM_TRACK_ACTION_FREE: {
                for_list (edith_mem_track_node, it, edith_g_mem_track_list) {
                    if (it->kind == kind && it->ptr == ptr) {
                        node = it;
                        break;
                    }
                }

                if (action == EDITH_MEM_TRACK_ACTION_FREE) {
                    dlist_remove(&edith_g_mem_track_list, node);
                }
            } break;
        }
    }

    return node;
}

static void edith_mem_track_print(void) {
    os_mutex_scope(edith_g_mem_track_mutex) {
        print("================================================================\n");
        for_list (edith_mem_track_node, node, edith_g_mem_track_list) {
            switch (node->kind) {
                case EDITH_MEM_TRACK_KIND_HEAP: {
                    print("heap \t ");
                    printf("name: \t %-30s \t", node->name.v);
                    print("size: \t %(size) \t ", node->size);
                    print("\n");
                } break;

                case EDITH_MEM_TRACK_KIND_ARENA: {
                    struct arena *arena = node->ptr;
                    i64 pos = 0;
                    i64 cap = arena->cap;
                    struct arena *it = arena;
                    while (it) {
                        if (it == arena->curr) {
                            pos += it->pos;
                        } else {
                            pos += it->cap;
                        }

                        it = it->next;
                    }

                    print("arena \t ");
                    printf("name: \t %-30s \t", node->name.v);
                    print("size: \t %(size)   \t / %(size) \t ", pos, cap);
                    print("\n");
                } break;
            }
        }
    }
}

static void edith_mem_track_dump_csv(str file_name) {
    arena *temp = edith_thread_local_arena;
    str_list list = { 0 };
    os_mutex_scope(edith_g_mem_track_mutex) {
        arena_scope(temp) {
            str_list_push_fmt(&list, temp, "%,%,%,%,%", "name", "kind", "pos", "cap", "realloc");

            for_list (edith_mem_track_node, node, edith_g_mem_track_list) {
                str name = { 0 };
                str kind = { 0 };
                i64 pos     = 0;
                i64 cap     = 0;
                i64 realloc = 0;

                switch (node->kind) {
                    case EDITH_MEM_TRACK_KIND_HEAP: {
                        kind    = str("heap");
                        name    = node->name;
                        pos     = node->size;
                        cap     = node->size;
                        realloc = node->realloc_count;
                    } break;

                    case EDITH_MEM_TRACK_KIND_ARENA: {
                        kind = str("arena");
                        name = node->name;
                        cap  = ((arena *)node->ptr)->cap;

                        struct arena *arena = node->ptr;
                        struct arena *it = arena;
                        while (it) {
                            if (it == arena->curr) {
                                pos += it->pos;
                            } else {
                                pos += it->cap;
                            }

                            it = it->next;
                        }
                    } break;
                }

                str_list_push_fmt(&list, temp, "%,%,%,%,%", name, kind, pos, cap, realloc);
            }

            str file_data = str_list_concat_sep(&list, temp, str("\n"));
            os_write_entire_file(file_name, file_data, temp, 0);
        }
    }
}

static void *edith_heap_alloc(i64 size, str name) {
    void *ptr = heap_alloc(size);

    edith_mem_track_node *node = edith_mem_track(ptr, EDITH_MEM_TRACK_KIND_HEAP, EDITH_MEM_TRACK_ACTION_ALLOC);
    node->name = name;
    node->size = size;

    return ptr;
}

static void *edith_heap_realloc(void *ptr, i64 size) {
    edith_mem_track_node *node = edith_mem_track(ptr, EDITH_MEM_TRACK_KIND_HEAP, EDITH_MEM_TRACK_ACTION_REALLOC);
    if (node == null) {
        assert(false && "Bad heap realloc");
    }

    void *new_ptr = heap_realloc(ptr, size);
    if (new_ptr == null) {
        assert(false && "Out of memory"); // TODO(rune): OOM handler
    }

    node->ptr            = new_ptr;
    node->size           = size;
    node->realloc_count += 1;

    return new_ptr;
}

static void edith_heap_free(void *ptr) {
    edith_mem_track_node *node = edith_mem_track(ptr, EDITH_MEM_TRACK_KIND_HEAP, EDITH_MEM_TRACK_ACTION_FREE);
    if (node == null) {
        assert(false && "Bad heap free");
    }
}

static arena *edith_arena_create(i64 cap, arena_kind kind, str name) {
    arena *arena = arena_create(cap, kind);

    edith_mem_track_node *node = edith_mem_track(arena, EDITH_MEM_TRACK_KIND_ARENA, EDITH_MEM_TRACK_ACTION_ALLOC);
    node->name = arena_copy_str(edith_g_mem_track_arena, name);

    return arena;
}

static arena *edith_arena_create_default(str name) {
    return edith_arena_create(kilobytes(64), ARENA_KIND_LINEAR, name);
}

static void edith_arena_destroy(arena *arena) {
    edith_mem_track_node *node = edith_mem_track(arena, EDITH_MEM_TRACK_KIND_ARENA, EDITH_MEM_TRACK_ACTION_FREE);
    if (node == null) {
        assert(false && "Bad arena destroy");
    }

    arena_destroy(arena);
}

#define heap_alloc(...)           (edith_heap_alloc(__VA_ARGS__, str("unamed")))
#define heap_realloc(...)         (edith_heap_realloc(__VA_ARGS__))
#define heap_free(...)            (edith_heap_free(__VA_ARGS__))
#define arena_create(...)         (edith_arena_create(__VA_ARGS__, str("unamed")))
#define arena_create_default()    (edith_arena_create_default(str("unamed")))
#define arena_destroy(...)        (edith_arena_destroy(__VA_ARGS__))
