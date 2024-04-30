////////////////////////////////////////////////////////////////
// rune: General

static draw_ctx *global_draw_ctx;

static void draw_create_ctx(draw_ctx *ctx, r_state *renderer) {
    zero_struct(ctx);
    ctx->renderer = renderer;
    ctx->atlas_tex = r_create_texture(ctx->renderer, ivec2(512, 512), R_TEX_FORMAT_R8);
    ctx->scissor = rect_make(VEC2_MIN, VEC2_MAX);

    ctx->arena = arena_create_default();
}

static void draw_destroy_ctx(draw_ctx *ctx) {
    arena_destroy(ctx->arena);
    r_destroy_texture(ctx->renderer, ctx->atlas_tex);
}

static void draw_select_ctx(draw_ctx *ctx) {
    global_draw_ctx = ctx;
}

static void draw_begin(void) {
    draw_ctx *ctx = global_draw_ctx;
    arena_reset(ctx->arena);
    ctx->submitted_passes.first = null;
    ctx->submitted_passes.last  = null;
}

static r_pass_list draw_end(void) {
    draw_ctx *ctx = global_draw_ctx;
    r_pass_list ret = ctx->submitted_passes;
    return ret;
}

static void draw_submit_current_pass(void) {
    draw_ctx *ctx = global_draw_ctx;
    r_pass *submit = arena_push_struct(ctx->arena, r_pass);
    *submit = ctx->current_pass;
    slist_push(&ctx->submitted_passes, submit);

    zero_struct(&ctx->current_pass);
}

////////////////////////////////////////////////////////////////
// rune: Rect pass

static void draw_begin_rect_pass(rect viewport, darray(r_rect_instance) *storage, atlas *atlas) {
    draw_ctx *ctx = global_draw_ctx;
    ctx->current_pass.type                       = R_PASS_TYPE_RECT;
    ctx->current_pass.type_rects.viewport        = viewport;
    ctx->current_pass.type_rects.tex             = ctx->atlas_tex;
    ctx->rect_storage = storage;
    ctx->atlas = atlas;
}

static r_pass draw_end_rect_pass(void) {
    draw_ctx *ctx = global_draw_ctx;
    if (ctx->atlas->dirty) {

        r_update_texture(ctx->renderer,
                         ctx->atlas_tex,
                         ctx->atlas->pixels,
                         ctx->atlas->dim.x * ctx->atlas->dim.y);

        ctx->atlas->dirty = false;
    }

    ctx->current_pass.type_rects.instances.v     = ctx->rect_storage->v;
    ctx->current_pass.type_rects.instances.count = ctx->rect_storage->count;
    return ctx->current_pass;
}

static void draw_set_scissor(rect r) {
    draw_ctx *ctx = global_draw_ctx;
    ctx->scissor = r;
}

static rect draw_get_scissor(void) {
    draw_ctx *ctx = global_draw_ctx;
    return ctx->scissor;
}

static void clip_adjust_uv(rect clip, rect *dst, rect *uv) {
    clip = rect_intersect(clip, draw_get_scissor());

    vec2 dim   = rect_dim(*dst);
    vec2 uvdim = rect_dim(*uv);

    if (dst->x0 < clip.x0) {
        uv->x0 -= uvdim.x * ((dst->x0 - clip.x0) / dim.x);
        dst->x0 = clip.x0;
    }

    if (dst->y0 < clip.y0) {
        uv->y0 -= uvdim.y * ((dst->y0 - clip.y0) / dim.y);
        dst->y0 = clip.y0;
    }

    if (dst->x1 > clip.x1) {
        uv->x1 -= uvdim.x * ((dst->x1 - clip.x1) / dim.x);
        dst->x1 = clip.x1;
    }

    if (dst->y1 > clip.y1) {
        uv->y1 -= uvdim.y * ((dst->y1 - clip.y1) / dim.y);
        dst->y1 = clip.y1;
    }
}

static void draw_rect_instance(r_rect_instance rect) {
    draw_ctx *ctx = global_draw_ctx;
    clip_adjust_uv(draw_get_scissor(), &rect.dst_rect, &rect.tex_rect);
    if (rect_dim_x(rect.dst_rect) > 0 &&
        rect_dim_y(rect.dst_rect) > 0) {
        r_rect_instance *inst = darray_add(ctx->rect_storage, 1);
        *inst = rect;
        ctx->current_pass.type_rects.instances.count++;
    }
}

static f32 draw_glyph(u32 codepoint, vec2 p, u32 color, ui_face face, font_backend_stb *backend) {
    draw_ctx *ctx = global_draw_ctx;
    atlas *atlas = ctx->atlas;

    assert(ctx->atlas == backend->atlas);

    // @Speed Don't do this for every glyph.
    font_metrics metrics = font_backend_stb_get_font_metrics(backend, face);

    f32 ret = 0.0f;
    if (codepoint) {
        p.y += metrics.lineheight;
        p.y += metrics.descent;

        glyph glyph = font_backend_stb_get_glyph(backend, face, codepoint);

        assert_bounds(glyph.atlas_idx, atlas->node_storage_count);
        atlas_node *node = &atlas->node_storage[glyph.atlas_idx];
        rect uv = atlas_get_node_uv(atlas, node);

        vec2 dim = vec2_from_ivec2(irect_dim(node->rect));

        vec2 p0 = vec2_add(p, glyph.bearing);
        vec2 p1 = vec2_add(p0, dim);
        rect dst = rect_round(rect_make(p0, p1));
        r_rect_instance inst = {
            .color = { color, color, color, color },
            .dst_rect = dst,
            .tex_rect = uv,
            .tex_weight = 1
        };
        draw_rect_instance(inst);

        ret = glyph.advance;
    } else {
        ret = font_backend_stb_get_advance(backend, face, codepoint);
    }

    return ret;
}

static f32 draw_text(str s, vec2 p, u32 color, ui_face face, font_backend_stb *backend) {
    draw_ctx *ctx = global_draw_ctx;

    assert(ctx->atlas == backend->atlas);
    assert(is_well_formed_utf8(s));

    f32 advance_sum = 0.0f;
    unicode_codepoint decoded = { 0 };
    rect clip = draw_get_scissor();
    while (advance_single_utf8_codepoint(&s, &decoded)) {
        if (p.x > clip.x1) {
            break;
        }

        f32 advance = draw_glyph(decoded.codepoint, p, color, face, backend);
        advance_sum += advance;
        p.x += advance;
    }

    return advance_sum;
}
