////////////////////////////////////////////////////////////////
// rune: Ring buffer

static i64 edith_ring_avail_for_write(edith_ring *ring) {
    i64 ret = ring->size - ring->write_pos + ring->read_pos;
    return ret;
}

static i64 edith_ring_avail(edith_ring *ring) {
    i64 ret = ring->write_pos - ring->read_pos;
    return ret;
}

static i64 edith_ring_write(edith_ring *ring, void *src, i64 write_size) {
    i64 avail_size = edith_ring_avail_for_write(ring);
    if (write_size <= avail_size) {

        i64 a_off = ring->write_pos % ring->size;
        i64 a_len = write_size;
        u8 *a_src = src;

        i64 b_off = 0;
        i64 b_len = 0;
        u8 *b_src = 0;

        if (a_off + write_size > ring->size) {
            a_len = ring->size - a_off;
            b_len = write_size - a_len;
            b_src = (u8 *)src  + a_len;
        }

        if (a_len > 0) memcpy(ring->base + a_off, a_src, a_len);
        if (b_len > 0) memcpy(ring->base + b_off, b_src, b_len);

        ring->write_pos += write_size;
    } else {
        write_size = 0;
    }

    return write_size;
}

static i64 edith_ring_peek(edith_ring *ring, void *dst, i64 peek_off, i64 peek_size) {
    i64 read_size = 0;
    i64 avail_size = edith_ring_avail(ring);
    if (peek_off + peek_size <= avail_size) {
        if (dst) {
            i64 a_off = (ring->read_pos + peek_off) % ring->size;
            i64 a_len = peek_size;
            u8 *a_dst = dst;

            i64 b_off = 0;
            i64 b_len = 0;
            u8 *b_dst = 0;

            if (a_off + peek_size > ring->size) {
                a_len = ring->size - a_off;
                b_len = peek_size  - a_len;
                b_dst = (u8 *)dst  + a_len;
            }

            if (a_len > 0) memcpy(a_dst, ring->base + a_off, a_len);
            if (b_len > 0) memcpy(b_dst, ring->base + b_off, b_len);
        }

        read_size = peek_size;
    }

    return read_size;
}

static i64 edith_ring_skip(edith_ring *ring, void *dst, i64 read_size) {
    i64 actual_size_read = edith_ring_peek(ring, dst, 0, read_size);
    ring->read_pos += actual_size_read;
    return actual_size_read;
}

static bool edith_ring_write_str(edith_ring *ring, str src) {
    return edith_ring_write(ring, src.v, src.len);
}

static str edith_ring_consume_str(edith_ring *ring, i64 size, arena *arena) {
    arena_mark mark = arena_mark_get(arena);

    str s = arena_push_str(arena, size, 0);
    bool b = edith_ring_skip(ring, s.v, s.len);
    if (b == false) {
        s.len = 0;
        s.v = 0;
        arena_mark_set(arena, mark);
    }

    return s;
}

////////////////////////////////////////////////////////////////
// rune: Command ring

static i64 edith_cmd_ring_write(edith_cmd_ring *queue, edith_cmd *cmd) {
    bool written_size = 0;
    i64 avail = edith_ring_avail_for_write(&queue->ring);
    i64 need_size = sizeof(edith_cmd_packet) + cmd->body.size;

    if (avail >= need_size) {
        edith_cmd_packet head = { 0 };
        head.magic     = EDITH_CMD_PACKET_MAGIC;
        head.kind      = cmd->kind;
        head.args[0]   = cmd->args[0];
        head.args[1]   = cmd->args[1];
        head.args[2]   = cmd->args[2];
        head.args[3]   = cmd->args[3];
        head.body_size = cmd->body.size;
        edith_ring_write(&queue->ring, &head, sizeof(head));
        edith_ring_write(&queue->ring, cmd->body.v, cmd->body.size);

        written_size = need_size;
    }

    return written_size;
}

static i64 edith_cmd_ring_peek(edith_cmd_ring *queue, edith_cmd *cmd, u64 off, arena *arena) {
    edith_cmd_packet head = { 0 };
    str body = { 0 };

    // rune: Peek head
    i64 head_size_peeked = edith_ring_peek(&queue->ring, &head, off, sizeof(head));

    // rune: Peek body
    i64 body_size_peeked = 0;
    if (head_size_peeked == sizeof(head) && head.body_size > 0) {
        u8 *body_dst   = arena_push_size_nozero(arena, head.body_size, 1);
        i64 body_off   = off + head_size_peeked;
        i64 body_size  = head.body_size;
        body_size_peeked = edith_ring_peek(&queue->ring, body_dst, body_off, body_size);

        body.v    = body_dst;
        body.size = body_size;
    }

    // rune: Unpack command if we go the expected size
    i64 ret = 0;
    if ((head_size_peeked == sizeof(head)) &&
        (body_size_peeked == head.body_size)) {
        ret = head_size_peeked + body_size_peeked;

        assert(head.magic == EDITH_CMD_PACKET_MAGIC);

        cmd->kind      = head.kind;
        cmd->args[0]   = head.args[0];
        cmd->args[1]   = head.args[1];
        cmd->args[2]   = head.args[2];
        cmd->args[3]   = head.args[3];
        cmd->body      = body;
    }

    return ret;
}

static void edith_cmd_ring_skip(edith_cmd_ring *queue, i64 cmd_size) {
    edith_ring_skip(&queue->ring, 0, cmd_size);
}


////////////////////////////////////////////////////////////////
// rune: Misc

static edith_cmd_spec *edith_cmd_spec_from_kind(edith_cmd_kind kind) {
    assert_bounds(kind, countof(edith_cmd_specs));
    edith_cmd_spec *spec =  &edith_cmd_specs[kind];
    return spec;
}

static void edith_cmd_print(edith_cmd *cmd) {
    // TODO(rune): Implement
#if 0
    edith_cmd_spec *spec = edith_cmd_spec_from_kind(cmd->kind);

    print(spec->name);
    print(" ");
    for (i64 i = 0; i < countof(spec->args); i++) {
        if (spec->args[i].kind != EDITH_CMD_ARG_KIND_NONE) {
            if (cmd->args.v[i].kind == EDITH_CMD_ARG_KIND_STR)    print("%(lit)", cmd->args.v[i].str);
            if (cmd->args.v[i].kind == EDITH_CMD_ARG_KIND_TAB)    print("%(lit)", cmd->args.v[i].tab->file_name);
            if (cmd->args.v[i].kind == EDITH_CMD_ARG_KIND_I64)    print("%(lit)", cmd->args.v[i].i64);
        }

        print(" ");
    }
    print("\n");
#endif
}
