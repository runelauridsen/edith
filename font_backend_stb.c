static bool font_backend_stb_startup(font_backend_stb *backend, atlas *atlas) {
    // @Todo Error handling.
    map_create(&backend->glyph_map, countof(backend->glyph_storage));
    backend->atlas = atlas;
    return true;
}

static u64 font_backend_stb_make_glyph_key(ui_face face, u32 codepoint) {
    u64 ret = pack_u32x2(face.u32, codepoint);
    return ret;
}

static f32 font_backend_stb_get_scale(font_backend_stb_slot *slot, u16 fontsize) {
    f32 scale = f32(fontsize) / f32(slot->ascent);
    return scale;
}

static font_backend_stb_slot *font_backend_stb_get_slot(font_backend_stb *backend, ui_font font) {
    font_backend_stb_slot *ret = null;
    if (font.u16 >= 1 && font.u16 <= countof(backend->slots)) {
        ret = &backend->slots[font.u16 - 1];
    }
    return ret;
}

static f32 font_backend_stb_get_advance(void *userdata, ui_face face, u32 codepoint) {
    f32 ret = 1;
    font_backend_stb *backend = (font_backend_stb *)userdata;
    font_backend_stb_slot *slot = font_backend_stb_get_slot(backend, face.font);
    if (slot) {
        f32 scale = font_backend_stb_get_scale(slot, face.fontsize);
        if (codepoint < countof(slot->advance_cache_ascii)) {
            ret = slot->advance_cache_ascii[codepoint] * scale;
        } else {
            // TODO(rune): get_advance() for non-ascii codepoints.");
            ret = slot->advance_cache_ascii[' '] * scale;
        }
    }
    return ret;
}

static f32 font_backend_stb_get_lineheight(void *userdata, ui_face face) {
    f32 ret = 1;
    font_backend_stb *backend = (font_backend_stb *)userdata;
    font_backend_stb_slot *slot = font_backend_stb_get_slot(backend, face.font);
    if (slot) {
        f32 scale = font_backend_stb_get_scale(slot, face.fontsize);
        ret = slot->lineheight * scale;
    }
    return ret;
}

static font_metrics font_backend_stb_get_font_metrics(font_backend_stb *backend, ui_face face) {
    font_metrics ret = { 0 };
    font_backend_stb_slot *slot = font_backend_stb_get_slot(backend, face.font);
    if (slot) {
        f32 scale = font_backend_stb_get_scale(slot, face.fontsize);
        ret.ascent = slot->ascent * scale;
        ret.descent = slot->descent * scale;
        ret.linegap = slot->linegap * scale;
        ret.lineheight = slot->lineheight * scale;
    }

    return ret;
}

static glyph font_backend_stb_get_glyph(font_backend_stb *backend, ui_face face, u32 codepoint) {
    glyph ret = { 0 };
    u64 key = font_backend_stb_make_glyph_key(face, codepoint);
    u64 glyph_idx;
    if (map_get(&backend->glyph_map, key, &glyph_idx)) {
        assert_bounds(glyph_idx, countof(backend->glyph_storage));
        ret = backend->glyph_storage[glyph_idx];
    } else {
        font_backend_stb_slot *slot = font_backend_stb_get_slot(backend, face.font);
        if (slot) {
            f32 scale = font_backend_stb_get_scale(slot, face.fontsize);

            i32 unscaled_bearing_x, unscaled_advance;
            stbtt_GetCodepointHMetrics(&slot->stb_info, codepoint, &unscaled_advance, &unscaled_bearing_x);

            irect rect = { 0 };
            stbtt_GetCodepointBitmapBox(&slot->stb_info, codepoint, scale, scale,
                                        &rect.x0, &rect.y0, &rect.x1, &rect.y1);

            ret.advance   = (f32)unscaled_advance * scale;
            ret.bearing.x = (f32)rect.x0;
            ret.bearing.y = (f32)rect.y0;

            atlas *atlas = backend->atlas;
            ivec2 dim = irect_dim(rect);
            atlas_node *node = atlas_new_node(atlas, key, dim);
            if (node) {
                i32 stride = atlas->dim.x;
                u8 *pixel = &atlas->pixels[node->rect.x0 + node->rect.y0 * stride];
                i32 w = irect_dim_x(node->rect);
                i32 h = irect_dim_y(node->rect);
                stbtt_MakeCodepointBitmap(&slot->stb_info, pixel, w, h, stride, scale, scale, codepoint);
                atlas->dirty = true;
                ret.atlas_idx = u32(node - atlas->node_storage);
            }

            if (backend->glyph_storage_count < countof(backend->glyph_storage)) {
                glyph_idx = backend->glyph_storage_count;
                glyph *cached = &backend->glyph_storage[backend->glyph_storage_count++];
                *cached = ret;
                map_put(&backend->glyph_map, key, glyph_idx);
            }
        }
    }

    return ret;
}

static ui_font font_backend_stb_init_font(font_backend_stb *backend, void *data, u64 data_size) {
    unused(data_size);

    ui_font ret = { 0 };

    if (backend->slot_count < countof(backend->slots)) {
        font_backend_stb_slot *slot = &backend->slots[backend->slot_count++];
        ret.u16 = backend->slot_count;

        stbtt_InitFont(&slot->stb_info, data, 0); // @Todo Error handling, don't increment backend->slot_count in case of invalid data.

        int ascent, descent, linegap;
        stbtt_GetFontVMetrics(&slot->stb_info, &ascent, &descent, &linegap);
        slot->ascent = (f32)ascent;
        slot->descent = (f32)descent;
        slot->linegap = (f32)linegap;
        slot->lineheight = slot->linegap + slot->ascent - slot->descent;

        for_n (i32, i, countof(slot->advance_cache_ascii)) {
            i32 c = i;
            if (!u32_is_printable) {
                c = ' ';
            }

            i32 advance = 0;
            stbtt_GetCodepointHMetrics(&slot->stb_info, i, &advance, null);
            slot->advance_cache_ascii[i] = (f32)advance;
        }
    }

    return ret;
}
