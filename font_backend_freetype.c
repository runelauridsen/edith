static bool font_backend_startup(font_backend *backend, atlas *atlas) {
    backend->atlas = atlas;
    map_create(&backend->glyph_map, countof(backend->glyph_storage));

    bool ret = false;

    if (!FT_Init_FreeType(&backend->library)) {
        ret = true;
    } else {
        printf("ERROR: FT_Init_FreeType failed.\n");
    }

    return ret;
}

static ui_font font_backend_init_font(font_backend *backend, span data) {
    ui_font ret = { 0 };

    if (backend->slot_count < countof(backend->slots)) {
        font_backend_slot *slot = &backend->slots[backend->slot_count++];
        ret.u16 = backend->slot_count;

        FT_Error error = FT_New_Memory_Face(backend->library, data.v, (FT_Long)data.len, 0, &slot->face);
        if (error) {
            printf("ERROR: FT_New_Memory_Face failed.\n");
        }
    }

    return ret;
}

static u64 font_backend_make_glyph_key(ui_face face, u32 codepoint) {
    u64 ret = pack_u32x2(face.u32, codepoint);
    return ret;
}

static font_backend_slot *font_backend_get_slot(font_backend *backend, ui_font font) {
    font_backend_slot *ret = null;
    if (font.u16 >= 1 && font.u16 <= countof(backend->slots)) {
        ret = &backend->slots[font.u16 - 1];
    }
    return ret;
}

static f32 font_backend_get_advance(void *userdata, ui_face face, u32 codepoint) {
    glyph glyph = font_backend_get_glyph(userdata, face, codepoint);
    return glyph.advance;
}

static f32 font_backend_get_lineheight(void *userdata, ui_face face) {
    font_metrics metrics = font_backend_get_font_metrics(userdata, face);
    return metrics.lineheight;
}

static font_metrics font_backend_get_font_metrics(font_backend *backend, ui_face face) {
    font_metrics ret = { 0 };

    font_backend_slot *slot = font_backend_get_slot(backend, face.font);

    if (!FT_Set_Pixel_Sizes(slot->face, 0, face.fontsize)) {
        // NOTE(rune): FreeType's metrics are in 1/64 pixels
        ret.ascent  = (f32)slot->face->size->metrics.ascender  / 64.0f;
        ret.descent = (f32)slot->face->size->metrics.descender / 64.0f;
        ret.linegap = (f32)slot->face->size->metrics.height    / 64.0f; // TODO(rune): What is face->height used for?
        ret.linegap = ret.ascent - ret.descent;
    } else {
        printf("ERROR: FT_Set_Pixel_Sizes failed.\n");
    }

    return ret;
}

static glyph font_backend_get_glyph(font_backend *backend, ui_face face, u32 codepoint) {
    println("font_backend_get_glyph(%(char,literal)", codepoint);

    glyph glyph = { 0 };
    u64 key = font_backend_make_glyph_key(face, codepoint);

    //- Use cached glyph or generate new glyph with freetype.
    u64 glyph_idx = 0;
    if (map_get(&backend->glyph_map, key, &glyph_idx)) {
        assert_array_bounds(glyph_idx, backend->glyph_storage);
        glyph = backend->glyph_storage[glyph_idx];
    } else {
        font_backend_slot *slot = font_backend_get_slot(backend, face.font);
        if (slot) {
            if (!FT_Set_Pixel_Sizes(slot->face, 0, face.fontsize)) {
                if (!FT_Load_Char(slot->face, codepoint, FT_LOAD_RENDER)) {
                    //- Get bit map size.
                    FT_Bitmap bitmap = slot->face->glyph->bitmap;
                    ivec2 dim = { bitmap.width, bitmap.rows };

                    //- Allocate space on atlas.
                    atlas *atlas = backend->atlas;
                    atlas_node *node = atlas_new_node(atlas, key, dim);
                    if (node) {
                        glyph.bearing.x = (f32)slot->face->glyph->bitmap_left;
                        glyph.bearing.y = (f32)slot->face->glyph->bitmap_top * - 1.0f;
                        glyph.advance   = (f32)slot->face->glyph->advance.x  / 64.0f;
                        glyph.atlas_idx = cast_u32(node - atlas->node_storage.v);
                        atlas->dirty = true;

                        //- Copy to atlas texture
                        i32 src_stride = bitmap.pitch;
                        i32 dst_stride = atlas->dim.x;
                        u8 *src = bitmap.buffer;
                        u8 *dst = &atlas->pixels.v[node->rect.x0 + node->rect.y0 * dst_stride];
                        for_n (i32, x, dim.x) {
                            for_n (i32, y, dim.y) {
                                dst[x + y * dst_stride] = src[x + y * src_stride];
                            }
                        }
                    }
                } else {
                    printf("ERROR: FT_Load_Char failed.\n");
                }
            } else {
                printf("ERROR: FT_Set_Pixel_Sizes failed.\n");
            }

            //- Save in glyph cache.
            if (backend->glyph_storage_count < countof(backend->glyph_storage)) {
                glyph_idx = backend->glyph_storage_count;
                backend->glyph_storage[backend->glyph_storage_count++] = glyph;
                map_put(&backend->glyph_map, key, glyph_idx);
            }
        }
    }

    return glyph;
}

#if 0
static bool font_backend_load(font_backend *backend, font_backend_face *info, void *data, size_t data_size) {
    bool ret = false;

    FT_Error error = FT_New_Memory_Face(backend->library, data, (FT_Long)data_size, 0, &info->face);

    if (!error) {
        ret = true;
    } else {
        printf("ERROR: FT_New_Memory_Face failed.\n");
    }

    return ret;
}

static void font_backend_unload(font_backend *backend, font_backend_face *info) {
    YO_UNUSED(backend);

    if (!FT_Done_Face(info->face)) {
        // NOTE(rune): All good
    } else {
        printf("ERROR: FT_Done_Face failed.\n");
    }
}

static font_metrics_t font_backend_get_font_metrics(font_backend *backend, font_backend_face *info, u32 fontsize) {
    YO_UNUSED(backend);
    YO_UNUSED(fontsize);

    font_metrics_t ret = { 0 };

    if (!FT_Set_Pixel_Sizes(info->face, 0, fontsize)) {
        // NOTE(rune): FreeType's metrics are in 1/64 pixels
        ret.ascent   = (f32)info->face->size->metrics.ascender  / 64.0f;
        ret.descent  = (f32)info->face->size->metrics.descender / 64.0f;
        ret.line_gap = (f32)info->face->size->metrics.height    / 64.0f; // TODO(rune): What is face->height used for?
        ret.line_gap = ret.ascent - ret.descent;
    } else {
        printf("ERROR: FT_Set_Pixel_Sizes failed.\n");
    }

    return ret;
}

static glyph_metrics_t font_backend_get_glyph_metrics(font_backend *backend, font_backend_face *info, u32 codepoint, u32 fontsize) {
    YO_UNUSED(backend);

    glyph_metrics_t ret = { 0 };

    if (!FT_Set_Pixel_Sizes(info->face, 0, fontsize)) {
        // TODO(rune): We shouldn't render here
        if (!FT_Load_Char(info->face, codepoint, FT_LOAD_RENDER)) {
            ret.bearing_x = (f32)info->face->glyph->bitmap_left;
            ret.bearing_y = (f32)-info->face->glyph->bitmap_top;
            ret.advance_x = (f32)info->face->glyph->advance.x  / 64.0f;
            ret.dim.x     = info->face->glyph->bitmap.width;
            ret.dim.y     = info->face->glyph->bitmap.rows;
        } else {
            printf("ERROR: FT_Load_Char failed.\n");
        }
    } else {
        printf("ERROR: FT_Set_Pixel_Sizes failed.\n");
    }

    return ret;
}

static void font_backend_rasterize(font_backend *backend, font_backend_face *info, u32 codepoint, u32 fontsize, void *pixel, ivec2 dim, int32_t stride) {
    YO_UNUSED(backend);

    if (!FT_Set_Pixel_Sizes(info->face, 0, fontsize)) {
        if (!FT_Load_Char(info->face, codepoint, FT_LOAD_RENDER)) {
            FT_Bitmap bitmap = info->face->glyph->bitmap;

            for (u32 src_x = 0; src_x < bitmap.width; src_x++) {
                for (u32 src_y = 0; src_y < bitmap.rows; src_y++) {
                    YO_ASSERT((int32_t)src_x < dim.x);
                    YO_ASSERT((int32_t)src_y < dim.y);

                    u8 *bitmap_pixel = &bitmap.buffer[src_x + src_y * bitmap.pitch];
                    u8 *atlas_pixel  = &((u8 *)pixel)[src_x + src_y * stride];

                    *atlas_pixel = *bitmap_pixel;
                }
            }
        } else {
            printf("ERROR: FT_Load_Char failed.\n");
        }
    } else {
        printf("ERROR: FT_Set_Pixel_Sizes failed.\n");
    }
}
#endif
