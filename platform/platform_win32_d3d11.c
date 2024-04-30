static r_rect_inst *r_d3d11_rect_inst_push(r_state *state) {
    return null;
}

static r_rect_inst *r_d3d11_rect_chunk_list_push(r_d3d11_rect_chunk_list *chunks, arena *arena) {
    r_d3d11_rect_chunk *chunk = chunks->last;
    if (chunk == null || chunk->count >= chunk->cap) {
        i64 inst_per_chunk = 64;

        chunk      = arena_push_struct(arena, r_d3d11_rect_chunk);
        chunk->v   = arena_push_array(arena, r_rect_inst, inst_per_chunk);
        chunk->cap = inst_per_chunk;

        slist_push(chunks, chunk);
        chunks->count += 1;
    }

    chunks->total_inst_count += 1;
    r_rect_inst *inst = &chunk->v[chunk->count++];
    return inst;
}

extern_c r_d3d11_rect_batch_list r_d3d11_rect_batches_from_nodes(yo_node *root, arena *arena, r_d3d11_render_data *data) {

    r_d3d11_rect_batch_list batches = { 0 };
    r_d3d11_rect_batch *batch = null;

    r_d3d11_rect_params params = { 0 };
    params.tex = data->atlas_tex;
    params.clip = rect_make(vec2(0, 0), vec2(1000, 1000));

    for (yo_node *node = root; node != null; node = yo_node_iter_pre_order(node)) {
        bool params_changed = false;

#if 1
        if (node->flags & YO_NODE_FLAG_CLIP) {
            params_changed = true;
            params.clip = node->abs_rect;
        }
#endif

        if (params_changed || batch == null ) {
            params_changed = false;

            batch = arena_push_struct(arena, r_d3d11_rect_batch);
            batch->params = params;
            slist_push(&batches, batch);
            batches.count += 1;
        }

        vec2 transform = vec2_sub(node->abs_offset, vec2(0, 0));
        for_list (yo_op, op_node, node->ops) {
            switch (op_node->type) {
                case YO_OP_KIND_AABB: {
                    yo_op_aabb *op = (yo_op_aabb *)op_node;
                    r_rect_inst *inst = r_d3d11_rect_chunk_list_push(&batch->chunks, arena);
                    inst->color[0] = op->color;
                    inst->color[1] = op->color;
                    inst->color[2] = op->color;
                    inst->color[3] = op->color;
                    inst->dst_rect = rect_round(rect_offset(op->rect, transform));
                } break;

                case YO_OP_KIND_AABB_EX: {
                    yo_op_aabb_ex *op = (yo_op_aabb_ex *)op_node;
                    r_rect_inst *inst = r_d3d11_rect_chunk_list_push(&batch->chunks, arena);
                    inst->dst_rect   = rect_round(rect_offset(op->dst_rect, transform));
                    inst->tex_rect   = op->tex_rect;
                    inst->tex_weight = op->tex_weight;
                    inst->softness   = op->softness;
                    inst->roundness  = op->corner_radius[0]; // @Todo Different radius per. corner.
                    inst->color[0]   = op->color[0];
                    inst->color[1]   = op->color[1];
                    inst->color[2]   = op->color[2];
                    inst->color[3]   = op->color[3];
                } break;

                case YO_OP_KIND_GLYPH: {
                    yo_op_glyph *op = (yo_op_glyph *)op_node;
                    vec2 p = op->p;

                    font_metrics metrics = font_backend_get_font_metrics(data->font_backend, op->face);
                    glyph glyph          = font_backend_get_glyph(data->font_backend, op->face, op->codepoint);

                    urect atlas_rect = { 0 };
                    if (atlas_get_slot(data->atlas, glyph.atlas_key, &atlas_rect)) {

                        vec2 glyph_p = p;
                        vec2_add_assign(&glyph_p, transform);
                        vec2_add_assign(&glyph_p, glyph.bearing);
                        vec2_add_assign(&glyph_p, vec2(0.0f, metrics.ascent));
                        vec2_round_assign(&glyph_p);
                        vec2 glyph_dim = vec2((f32)(atlas_rect.x1 - atlas_rect.x0), (f32)(atlas_rect.y1 - atlas_rect.y0));
                        rect glyph_rect = rect_make_dim(glyph_p, glyph_dim);

                        r_rect_inst *inst = r_d3d11_rect_chunk_list_push(&batch->chunks, arena);
                        inst->color[0] = op->color;
                        inst->color[1] = op->color;
                        inst->color[2] = op->color;
                        inst->color[3] = op->color;
                        inst->dst_rect = glyph_rect;
                        inst->tex_weight = 1.0f;
                        inst->tex_rect = atlas_get_uv(data->atlas, atlas_rect);
                    }
                } break;

                case YO_OP_KIND_TEXT: {
                    yo_op_text *op = (yo_op_text *)op_node;
                    font_metrics metrics = font_backend_get_font_metrics(data->font_backend, op->face);
                    vec2 p = op->p;
                    str s = op->s;
                    unicode_codepoint decoded = { 0 };
                    while (advance_single_utf8_codepoint(&s, &decoded)) {
                        glyph glyph = font_backend_get_glyph(data->font_backend, op->face, decoded.codepoint);

                        urect atlas_rect = { 0 };
                        if (atlas_get_slot(data->atlas, glyph.atlas_key, &atlas_rect)) {
                            vec2 glyph_p = p;
                            vec2_add_assign(&glyph_p, transform);
                            vec2_add_assign(&glyph_p, glyph.bearing);
                            vec2_add_assign(&glyph_p, vec2(0.0f, metrics.ascent));
                            vec2_round_assign(&glyph_p);
                            vec2 glyph_dim = vec2((f32)(atlas_rect.x1 - atlas_rect.x0), (f32)(atlas_rect.y1 - atlas_rect.y0));
                            rect glyph_rect = rect_make_dim(glyph_p, glyph_dim);

                            r_rect_inst *inst = r_d3d11_rect_chunk_list_push(&batch->chunks, arena);
                            inst->color[0] = op->color;
                            inst->color[1] = op->color;
                            inst->color[2] = op->color;
                            inst->color[3] = op->color;
                            inst->dst_rect = glyph_rect;
                            inst->tex_weight = 1.0f;
                            inst->tex_rect = atlas_get_uv(data->atlas, atlas_rect);
                        }

                        p.x += glyph.advance;
                    }
                } break;

                default: {
                    assert(!"ui op type not implemeted");
                } break;
            }
        }
    }

    return batches;
}

extern_c r_d3d11_rect_drawcall_list r_d3d11_rect_drawcalls_from_batches(r_d3d11_rect_batch_list batches, arena *arena, r_d3d11_render_data *data) {
    r_d3d11_rect_drawcall_list drawcalls = { 0 };

    for_list (r_d3d11_rect_batch, batch, batches) {
        i64          inst_count = batch->chunks.total_inst_count;
        r_rect_inst *insts = arena_push_array(arena, r_rect_inst, inst_count);

        i64 i = 0;
        for_list (r_d3d11_rect_chunk, chunk, batch->chunks) {
            i64 j = 0;
            while (j < chunk->count) {
                insts[i++] = chunk->v[j++];
            }
        }

        r_d3d11_rect_drawcall *drawcall = arena_push_struct(arena, r_d3d11_rect_drawcall);
        drawcall->insts.v     = insts;
        drawcall->insts.count = inst_count;
        drawcall->params      = batch->params;

        slist_push(&drawcalls, drawcall);
        drawcalls.count += 1;
    }

    return drawcalls;
}
