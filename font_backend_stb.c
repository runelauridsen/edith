static bool font_backend_startup(font_backend *backend, atlas *atlas) {
    // @Todo Error handling.
    map_create(&backend->glyph_map, countof(backend->glyph_storage));
    backend->atlas = atlas;
    return true;
}

static u64 font_backend_make_glyph_key(yo_face face, u32 codepoint) {
    u64 ret = pack_u32x2(face.u32, codepoint);
    return ret;
}

static f32 font_backend_get_scale(font_backend_slot *slot, u16 fontsize) {
    f32 scale = f32(fontsize) / f32(slot->ascent);
    return scale;
}

static font_backend_slot *font_backend_get_slot(font_backend *backend, yo_font font) {
    font_backend_slot *ret = null;
    if (font.u16 >= 1 && font.u16 <= countof(backend->slots)) {
        ret = &backend->slots[font.u16 - 1];
    }
    return ret;
}

static f32 font_backend_get_advance(void *userdata, yo_face face, u32 codepoint) {
    f32 ret = 1;
    font_backend *backend = (font_backend *)userdata;
    font_backend_slot *slot = font_backend_get_slot(backend, face.font);
    if (slot) {
        f32 scale = font_backend_get_scale(slot, face.fontsize);
        if (codepoint < countof(slot->advance_cache_ascii)) {
            ret = slot->advance_cache_ascii[codepoint] * scale;
        } else {
            // @todo get_advance() for non-ascii codepoints.");
            ret = slot->advance_cache_ascii[' '] * scale;
        }
    }
    return ret;
}

static f32 font_backend_get_lineheight(void *userdata, yo_face face) {
    f32 ret = 1;
    font_backend *backend = (font_backend *)userdata;
    font_backend_slot *slot = font_backend_get_slot(backend, face.font);
    if (slot) {
        f32 scale = font_backend_get_scale(slot, face.fontsize);
        ret = slot->lineheight * scale;
    }
    return ret;
}

static font_metrics font_backend_get_font_metrics(font_backend *backend, yo_face face) {
    font_metrics ret = { 0 };
    font_backend_slot *slot = font_backend_get_slot(backend, face.font);
    if (slot) {
        f32 scale = font_backend_get_scale(slot, face.fontsize);
        ret.ascent = slot->ascent * scale;
        ret.descent = slot->descent * scale;
        ret.linegap = slot->linegap * scale;
        ret.lineheight = slot->lineheight * scale;
    }

    return ret;
}

static glyph font_backend_get_glyph(font_backend *backend, yo_face face, u32 codepoint) {
    glyph ret = { 0 };

    bool found_cached = 0;
    u64 key = font_backend_make_glyph_key(face, codepoint);
    u64 glyph_idx;
    if (map_get(&backend->glyph_map, key, &glyph_idx)) {
        assert_bounds(glyph_idx, countof(backend->glyph_storage));
        ret = backend->glyph_storage[glyph_idx];

        urect rect = { 0 };
        if (atlas_get_slot(backend->atlas, ret.atlas_key, &rect)) {
            found_cached = true;
        }
    }

    if (found_cached == false) {
        font_backend_slot *slot = font_backend_get_slot(backend, face.font);
        if (slot) {
            f32 scale = font_backend_get_scale(slot, face.fontsize);

            i32 unscaled_bearing_x, unscaled_advance;
            stbtt_GetCodepointHMetrics(&slot->stb_info, codepoint, &unscaled_advance, &unscaled_bearing_x);

            i32 box_x0, box_y0, box_x1, box_y1;
            stbtt_GetCodepointBitmapBox(&slot->stb_info, codepoint, scale, scale,
                                        &box_x0, &box_y0, &box_x1, &box_y1);

            ret.advance   = (f32)unscaled_advance * scale;
            ret.bearing.x = (f32)box_x0;
            ret.bearing.y = (f32)box_y0;

            atlas *atlas = backend->atlas;
            uvec2 dim = uvec2(box_x1 - box_x0, box_y1 - box_y0);
            atlas_slot_key atlas_key = atlas_new_slot(atlas,  dim);

            urect atlas_rect = { 0 };
            if (atlas_get_slot(atlas, atlas_key, &atlas_rect)) {
                u32 stride = atlas->dim.x;
                u8 *pixel = &atlas->pixels[atlas_rect.x0 + atlas_rect.y0 * stride];
                u32 w = urect_dim_x(atlas_rect);
                u32 h = urect_dim_y(atlas_rect);
                stbtt_MakeCodepointBitmap(&slot->stb_info, pixel, w, h, stride, scale, scale, codepoint);
                atlas->dirty = true;
                ret.atlas_key = atlas_key;
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

static yo_font font_backend_init_font(font_backend *backend, str data) {
    yo_font ret = { 0 };

    if (backend->slot_count < countof(backend->slots)) {
        font_backend_slot *slot = &backend->slots[backend->slot_count++];
        ret.u16 = backend->slot_count;

        stbtt_InitFont(&slot->stb_info, data.v, 0); // @Todo Error handling, don't increment backend->slot_count in case of invalid data.

        int ascent, descent, linegap;
        stbtt_GetFontVMetrics(&slot->stb_info, &ascent, &descent, &linegap);
        slot->ascent = (f32)ascent;
        slot->descent = (f32)descent;
        slot->linegap = (f32)linegap;
        slot->lineheight = slot->linegap + slot->ascent - slot->descent;

        for_n (i32, i, countof(slot->advance_cache_ascii)) {
            i32 c = i;
            if (!u32_is_printable(c)) {
                c = ' ';
            }

            i32 advance = 0;
            stbtt_GetCodepointHMetrics(&slot->stb_info, i, &advance, null);
            slot->advance_cache_ascii[i] = (f32)advance;
        }
    }

    return ret;
}
