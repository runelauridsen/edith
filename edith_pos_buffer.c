typedef struct edith_pos_item edith_pos_item;
struct edith_pos_item {
    str  data;
    u64  pos_idx;
};

typedef struct edith_pos_buffer edith_pos_buffer;
struct edith_pos_buffer {
    edith_gapbuffer64 item_array;

    struct {
        u64 *base;
        u64  capacity;
        u64  count;

        u64 first_free;
    } pos_array;

    u64 pos_idx_member_offset;
};

static void edith_pos_buffer_init(edith_pos_buffer *pb, u64 pos_idx_member_offset) {
    pb->pos_idx_member_offset = pos_idx_member_offset;
}

static u64 edith_pos_buffer_pos_from_item(edith_pos_buffer *pb, void *item) {
    u64 *pos_idx_member = ptr_add(item, pb->pos_idx_member_offset);
    u64 pos_idx = *pos_idx_member;
    assert_bounds(pos_idx, pb->pos_array.count);
    u64 pos = pb->pos_array.base[pos_idx];
    return pos;
}

static void *edith_pos_buffer_item_from_idx(edith_pos_buffer *pb, u64 idx) {
    void *item = ptr_from_u64(*edith_gb64_get(&pb->item_array, idx));
    return item;
}

static u64 edith_pos_buffer_search_bot(edith_pos_buffer *pb, u64 needle) {
    u64 bot_idx = 0;
    u64 top_idx = edith_gb64_count(&pb->item_array);
    while (top_idx > bot_idx + 1) {
        u64   mid_idx  = bot_idx + (top_idx - bot_idx) / 2;
        void *mid_item = edith_pos_buffer_item_from_idx(pb, mid_idx);
        u64   mid_pos  = edith_pos_buffer_pos_from_item(pb, mid_item);
        if (needle < mid_pos) {
            top_idx = mid_idx;
        } else {
            bot_idx = mid_idx;
        }
    }
    return bot_idx;
}

static u64 edith_pos_buffer_alloc_pos_idx(edith_pos_buffer *pb) {
    u64 ret = 0;

    // Does the free list currently contain any entries?
    if (pb->pos_array.first_free & (u64(1) << 63)) {
        // Pop from free list
        ret = pb->pos_array.first_free & ~(u64(1) << 63);
        pb->pos_array.first_free = pb->pos_array.base[pb->pos_array.first_free];
    } else {
        // Push to array and resize if neccesary
        if (pb->pos_array.count + 1 >= pb->pos_array.capacity) {
            u64 new_capacity = pb->pos_array.capacity * 2;
            new_capacity = clamp_bot(new_capacity, 1024);

            pb->pos_array.base     = edith_heap_realloc(pb->pos_array.base, sizeof(u64) * new_capacity);
            pb->pos_array.capacity = new_capacity;
        }

        ret = pb->pos_array.count++;
    }

    return ret;
}

static void edith_pos_buffer_free_pos_idx(edith_pos_buffer *pb, u64 pos_idx) {
    assert_bounds(pos_idx, pb->pos_array.count);
    assert((pb->pos_array.base[pos_idx] & (u64(1) << 63)) == 0);

    // Does the free list currently contain any entries?
    if (pb->pos_array.first_free & (u64(1) << 63)) {
        // Point newly free entry to head of free list.
        pb->pos_array.base[pos_idx] = pb->pos_array.first_free;
    } else {
        // Mark entry as tail of free list.
        pb->pos_array.base[pos_idx] = 0;
    }

    // Set head of free list to newly freed entry.
    pb->pos_array.first_free = pos_idx | (u64(1) << 63);
}

static u64 edith_pos_buffer_search_top(edith_pos_buffer *pb, u64 needle) {
    u64 bot_idx = 0;
    u64 top_idx = edith_gb64_count(&pb->item_array);
    while (top_idx > bot_idx) {
        u64   mid_idx  = bot_idx + (top_idx - bot_idx) / 2;
        void *mid_item = edith_pos_buffer_item_from_idx(pb, mid_idx);
        u64   mid_pos  = edith_pos_buffer_pos_from_item(pb, mid_item);
        if (needle <= mid_pos) {
            top_idx = mid_idx;
        } else {
            bot_idx = mid_idx + 1;
        }
    }
    return top_idx;
}

static u64 edith_pos_buffer_idx_from_item(edith_pos_buffer *pb, void *item) {
    u64 pos = edith_pos_buffer_pos_from_item(pb, item);
    u64 item_idx = edith_pos_buffer_search_bot(pb, pos);

#ifndef NDEBUG
    void *found_item = edith_pos_buffer_item_from_idx(pb, item_idx);
    assert(found_item == item);
#endif

    return item_idx;
}

static void edith_pos_buffer_insert(edith_pos_buffer *pb, void *item, u64 pos) {
    // rune: Add sorted entry in item array
    u64 item_idx = edith_pos_buffer_search_top(pb, pos);
    edith_gb64_put(&pb->item_array, item_idx, u64_from_ptr(item));

    // rune: Add entry in pos array
    u64 pos_idx = edith_pos_buffer_alloc_pos_idx(pb);
    pb->pos_array.base[pos_idx] = pos;

    // rune: Store position
    u64 *pos_idx_member = ptr_add(item, pb->pos_idx_member_offset);
    *pos_idx_member = pos_idx;
}

static void edith_pos_buffer_remove(edith_pos_buffer *pb, void *item) {
    u64 item_idx = edith_pos_buffer_idx_from_item(pb, item);

    // rune: Push pos slot to free list
    u64 *pos_idx_member = ptr_add(item, pb->pos_idx_member_offset);
    u64 pos_idx         = *pos_idx_member;
    edith_pos_buffer_free_pos_idx(pb, pos_idx);

    // rune: Remove item ptr from item array
    edith_gb64_delete(&pb->item_array, item_idx, 1);
}

////////////////////////////////////////////////////////////////
// rune: Debug print

static void edith_pos_buffer_print(edith_pos_buffer *pb) {
    print(ANSI_FG_GRAY);
    print("== item array =================================\n");
    print(ANSI_FG_DEFAULT);

    u64 count = edith_gb64_count(&pb->item_array);
    for_n (u64, i, count) {
        edith_pos_item *item = edith_pos_buffer_item_from_idx(pb, i); // TODO(rune): Hardcoded item type
        u64 pos              = edith_pos_buffer_pos_from_item(pb, item);
        print("pos:\t%\t", pos);
        print("data:\t%\t", item->data);
        print("\n");
    }
    print("\n");

    print(ANSI_FG_GRAY);
    print("== pos array ==================================\n");
    print(ANSI_FG_DEFAULT);

    if (pb->pos_array.first_free & (u64(1) << 63)) {
        print("first free: %\n", pb->pos_array.first_free & ~(u64(1) << 63));
    } else {
        print("first free: none\n");
    }

    for_n (u64, i, pb->pos_array.count) {
        u64 pos = pb->pos_array.base[i];
        if (pos & (u64(1) << 63)) {
            pos &= ~(u64(1) << 63);
            print(ANSI_FG_MAGENTA);
        }

        if (i == (pb->pos_array.first_free & ~(u64(1) << 63))) {
            print(ANSI_FG_BLUE);
        }

        print("[%]\t= %\n", i, pos);
        print(ANSI_FG_DEFAULT);
    }
    print("\n");

}