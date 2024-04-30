static ui_toast_ctx *global_ui_toast_ctx;

static ui_toast *ui_alloc_toast(void) {
    ui_toast_ctx *ctx = global_ui_toast_ctx;
    ui_toast *ret = 0;
    if (ctx->first_free) {
        ret = ctx->first_free;
        slstack_pop(&ctx->first_free);
    } else {
        ret = &ctx->storage.v[0];
        for_array (ui_toast, it, ctx->storage) {
            if (it->born_tick < ret->born_tick) {
                ret = it;
            }
        }
    }
    zero_struct(ret);
    return ret;
}

static void ui_free_toast(ui_toast *data) {
    ui_toast_ctx *ctx = global_ui_toast_ctx;
    data->next = 0;
    data->prev = 0;
    slstack_push(&ctx->first_free, data);
}

static void ui_post_toast(i64 lifetime, str s) {
    if (lifetime == 0) lifetime = 5000;
    ui_toast_ctx *ctx = global_ui_toast_ctx;
    ui_toast *data = ui_alloc_toast();
    data->born_tick = ui_tick();
    data->dead_tick = ui_tick() + lifetime;
    data->text_len  = min(s.count, sizeof(data->text));
    memcpy(data->text, s.v, data->text_len);
    dlist_push(&ctx->active, data);
}

static void ui_toasts(rect area) {
    ui_toast_ctx *ctx = global_ui_toast_ctx;
    ui_toast *it = ctx->active.first;

    u32 background = rgb(64, 64, 64);
    u32 foreground = rgb(255, 255, 255);

    f32 item_height = 30.0f;
    f32 item_width = 300.0f;
    f32 item_spacing = 5.0f;

    while (it) {
        ui_toast *next = it->next;
        if (ui_tick() < it->dead_tick) {
            ui_cut_bot(&area, item_spacing);
            rect r = ui_cut_bot(&area, item_height);
            ui_max_left(&r, item_width);
            ui_cut_x(&r, 5);

            // TODO(rune): @Todo Support ease-in-out animations. This is not a direct user interaction,
            // so we don't want the usual behaviour of starting animation at a high velocity.
            ui_anim_damped_rect(&it->animated_rect, r, 10.0f, 0.1f);
            r = it->animated_rect.pos;

            ui_draw_rounded_rect_with_shadow(r, background, 5, 5, 0);

            ui_push_scissor();
            ui_set_scissor(r, 0);
            str s = str_make(it->text, it->text_len);
            ui_draw_text(r.p0, s, foreground);
            ui_pop_scissor();
        } else {
            dlist_remove(&ctx->active, it);
            ui_free_toast(it);
        }

        it = next;
    }
}
