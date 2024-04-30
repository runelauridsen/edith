static void ui_demo(rect area) {

    unused(area);

    YO_PROFILE_BEGIN(ui_demo);
#if 0


#elif 0

    ////////////////////////////////
    // Construct widgets.

    ui_set_layout(ui_make_layout_x(area, UI_ALIGN_CENTER, 0));

    ui_widget_a();
    ui_widget_b();

#elif 0
    ui_push_transform();
    ui_pop_transform();

    ui_push_transform();
    ui_set_transform(v2(10, 10));
    ui_pop_transform();



    ui_push_scissor();
    ui_pop_scissor();

    ui_push_scissor();
    ui_set_scissor((rect) { 1, 2, 3, 4 }, 0);
    ui_pop_scissor();

    ui_push_scissor();
    ui_set_scissor((rect) { 5, 6, 7, 8 }, 0);
    ui_pop_scissor();

#elif 0

    ui_set_transform(v2(10, 10), 0);

    static u32 color;
    ui_color_picker(ui_id("picker"), &color);

#elif 0
    static u8 buf0[256] = { 0 };
    static u8 buf1[256] = { 0 };
    static u8 buf2[256] = { 0 };

    static u64 len0 = { 0 };
    static u64 len1 = { 0 };
    static u64 len2 = { 0 };
    static f32 slider3 = { 0 };

    u32 fg = rgb(200, 200, 200);

    ui_cut_all(&area, 10);
    ui_cut_left(&area, 50);
    ui_cut_right(&area, 50);

    ui_draw_rect(area, rgb(30, 40, 40));
    ui_cut_all(&area, 2);

    ui_set_layout(ui_make_layout_y(area, UI_ALIGN_STRETCH, 10.0f));

    ui_text_field_ex(ui_id("txt0"), spanof(buf0), &len0, str("I ain't 'fraid of no ghosts."), 0, null);
    ui_text_field(ui_id("txt1"), spanof(buf1), &len1, null);
    ui_text_field(ui_id("txt2"), spanof(buf2), &len2, null);
    ui_slider_f32(ui_id("sli3"), &slider3, 1, 100, AXIS2_X);
    static i32 slideri;
    ui_slider_i32(ui_id("slii"), &slideri, 90, 91, AXIS2_X);

    if (ui_button(ui_id("btn1"), str("Click me!"))) {
        static i32 click_counter = 0;
        println("Clicked % times!", ++click_counter);
    }
    ui_text(ui_fmt("Slider value is %", slider3), fg);
    ui_text(ui_fmt("Slider value is %", slideri), fg);

    ui_text(str("yo"), fg);
    ui_text(str("yy"), fg);

    rect scroll_area = ui_place_rect(v2(F32_MAX, F32_MAX));
    ui_draw_rect(scroll_area, rgb(40, 30, 30));
    ui_scrollable_rect(ui_id("scroll0"), scroll_area) {
        for_range (i32, i, 0, cast_i32(slider3)) {
            ui_text(ui_fmt(i + 1), fg);
        }

        static u32 color;
        ui_color_picker(ui_id("color_picker"), &color);
        ui_place_spacing(10);
    }

    //ui_slider_f32(ui_id("sli4"), null, range_f32(2, 100), AXIS2_Y);

    YO_PROFILE_END(ui_demo);
#endif
}
