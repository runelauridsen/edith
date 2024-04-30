////////////////////////////////////////////////////////////////
// rune: Common

static theme default_theme(void) {
    theme theme = {
        .bg_main     = rgb(30, 30, 30),
        .bg_hover    = rgb(40, 40, 40),
        .bg_hover1   = rgb(60, 60, 60),
        .bg_active   = rgb(20, 60, 60),
        .bg_border   = rgb(40, 40, 40),
        .fg_main     = rgb(200, 200, 200),
        .fg_inactive = rgb(200, 200, 200),
        .fg_active   = rgb(230, 230, 230),
        .roundness   = 5,

        .button_bg                 = rgb(35, 35, 35),
        .button_bg_hot             = rgb(40, 40, 40),
        .button_bg_active          = rgb(25, 25, 25),

        .button_fg                 = rgb(200, 200, 200),
        .button_fg_hot             = rgb(200, 200, 200),
        .button_fg_active          = rgb(200, 200, 200),

        .button_bc                 = rgb(50, 50, 50),
        .button_bc_hot             = rgb(70, 70, 70),
        .button_bc_active          = rgb(70, 70, 70),

        .button_border_thickness   = 1,
        .button_border_radius      = 2,

        .bg_bad                    = rgb(170, 90, 90),

    };
    return theme;
}

static rect calc_popup_area(rect client_rect) {
    f32 top_margin = 50.0f;
    vec2 min_dim = vec2(300, 200);
    vec2 max_dim = vec2(700, 400);
    f32 pref_frac = 0.6f;

    vec2 pref_dim = vec2_scale(rect_dim(client_rect), pref_frac);
    vec2 clamped_pref_dim = vec2_clamp(pref_dim, min_dim, max_dim);
    rect popup_area = client_rect;
    ui_cut_top(&popup_area, top_margin);
    ui_cut_aligned(&popup_area, clamped_pref_dim, UI_ALIGN_TOP_CENTER);
    rect rounded_popup_area = rect_round(popup_area);
    return rounded_popup_area;
}

static void cut_and_draw_popup_frame(rect *area) {
    theme theme = default_theme();

    ui_draw_rounded_rect_with_shadow(*area, theme.bg_border, theme.roundness, 10, 0);
    ui_cut_all(area, 1);
    ui_draw_rounded_rect(*area, theme.bg_main, 5);
}

static rect cut_and_draw_field_frame(rect *area, field_frame *frame) {
    // Input field.
    u32  border_color = rgb(40, 90, 90);
    u32  background_color = rgb(20, 20, 20);
    theme theme = default_theme();
    if (frame->bad_value) {
        border_color = theme.bg_bad;
    }

    f32 pad = frame->padding * 2 + frame->margin * 2;
    vec2 pref_dim = vec2_add(frame->content_pref_dim, vec2(pad, pad));
    rect field_area = ui_cut_top(area, pref_dim.y);

    ui_cut_all(&field_area, frame->margin);
    ui_draw_rect(field_area, border_color);

    ui_cut_all(&field_area, 1);
    ui_draw_rect(field_area, background_color);

    ui_cut_all(&field_area, frame->padding);
    return field_area;
}

static void ui_popup_frame_box(void) {
    theme theme = default_theme();

    ui_new_box(0);
    ui_set_border(1, theme.bg_border, theme.roundness);
    ui_set_background(theme.bg_main);
    ui_set_shadow(10);
}

static void ui_fuzzy_matched_string(str s, fuzzy_match_list matches) {
    ui_layout_x() {
        i64 at = 0;
        for (fuzzy_match_node *node = matches.first; node; node = node->next) {
            str before = substr_range(s, i64_range(at, node->range.min));
            str match = substr_range(s, node->range);

            ui_text_b(before);
            ui_set_text_color(rgb(200, 200, 200));

            ui_text_b(match);
            ui_set_text_color(rgb(100, 180, 180));

            at = node->range.max;
        }

        str after = substr_idx(s, at);
        ui_text_b(after);
        ui_set_text_color(rgb(200, 200, 200));
    }
}

////////////////////////////////////////////////////////////////
// rune: Find incremental dialog

static void edith_dialog_find(rect r, edith_dialog_find_data *data, edith_textview *tv) {

    YO_PROFILE_BEGIN(edith_dialog_find);

    theme theme = default_theme();

    ui_box *root = ui_new_box(0);
    ui_set_align(UI_ALIGN_TOP_RIGHT);
    ui_set_dim_x(ui_len_abs(600));
    ui_set_child_axis(AXIS2_X);
    ui_set_background(theme.bg_main);
    ui_set_shadow(10);
    ui_children_begin();

    ////////////////////////////////////////////////
    // Search box.

    bool restart = false;

    edith_search_result_array visible_results = edith_search_results_from_key(tv->search_result_key_visible, 0, 1);
    bool had_no_results = visible_results.count == 0 && data->text_len > 0;
    ui_id txt = ui_id("txt1");
    ui_set_active_id(txt);
    if (ui_text_field_b(txt, data->text_buf, sizeof(data->text_buf), &data->text_len, str("Find string"))) {
        restart = true;
    }
    ui_set_dim_x(ui_len_rel(1));
    if (had_no_results) {
        ui_set_border(1, rgb(170, 90, 90), 0);
    }

    ////////////////////////////////////////////////
    // Num results.

    edith_cursor *primary_cursor = edith_textview_cursors_get_primary(tv);
    i64 curr_result_idx = edith_search_result_idx_from_pos(visible_results, primary_cursor->caret).min;
    ui_new_box(0);
    ui_set_text(ui_fmt("%/%", visible_results.count == 0 ? 0 : curr_result_idx + 1, visible_results.count));
    ui_set_text_align(UI_TEXT_ALIGN_CENTER);
    ui_set_dim_x(ui_len_abs(100));
    ui_set_dim_y_min(UI_LEN_TEXT);

    ////////////////////////////////////////////////
    // Options.

    ui_id case_id = ui_id("case");
    ui_new_box(0);
    ui_set_text(str("Case"));
    ui_set_text_align(UI_TEXT_ALIGN_CENTER);
    ui_set_dim_x(ui_len_abs(100));
    ui_set_dim_y_min(UI_LEN_TEXT);
    if (ui_clicked_on_rect(ui_get_widget_hitbox(case_id))) {
        data->case_sensitive = !data->case_sensitive;
        restart = true;
    }

    if (ui_mouse_is_over(ui_get_widget_hitbox(case_id))) {
        ui_set_background(theme.bg_hover);
    }

    if (data->case_sensitive) {
        ui_set_background(theme.bg_active);
    }

    ui_children_end();

    ////////////////////////////////////////////////
    // Interaction.

    ui_events events = ui_get_events();
    for_array (ui_event, e, events) {
        if (ui_event_is_keypress_with_modifiers(e, UI_KEY_F3, UI_MODIFIER_NONE) ||
            ui_event_is_keypress_with_modifiers(e, UI_KEY_DOWN, UI_MODIFIER_NONE)) {
            edith_cmd_ring_write(&edith_global_cmd_ring, &(edith_cmd) { EDITH_CMD_KIND_FIND_NEXT });
            data->has_moved_to_result = true;
        }

        if (ui_event_is_keypress_with_modifiers(e, UI_KEY_F3, UI_MODIFIER_SHIFT) ||
            ui_event_is_keypress_with_modifiers(e, UI_KEY_UP, UI_MODIFIER_NONE)) {
            edith_cmd_ring_write(&edith_global_cmd_ring, &(edith_cmd) { EDITH_CMD_KIND_FIND_PREV });
            data->has_moved_to_result = true;
        }

        if (ui_event_is_keypress_with_modifiers(e, UI_KEY_ENTER, UI_MODIFIER_NONE)) {
            if (data->has_moved_to_result == false) {
                edith_cmd_ring_write(&edith_global_cmd_ring, &(edith_cmd) { EDITH_CMD_KIND_FIND_NEXT });
            }
            edith_cmd_ring_write(&edith_global_cmd_ring, &(edith_cmd) { EDITH_CMD_KIND_CLOSE_DIALOG });
            data->has_moved_to_result = true;
        }
    }

    if (restart) {
        data->has_moved_to_result = false;

        // TODO(rune): Finder flags case sensitive.
    }

    str needle = str_make(data->text_buf, data->text_len);
    if (needle.len > 0) {
        u64 needle_hash = ui_hash_str(needle);
        edith_textbuf *textbuf = &tv->tb;
        u128 key = u128(needle_hash, u64_from_ptr(textbuf));
        bool completed = 0;

        edith_submit_textbuf_query(key, needle, textbuf, &completed);
        edith_search_results_from_key(key, &completed, 1);

        tv->search_result_key_requested = key;
        if (completed) {
            tv->search_result_key_visible = key;
        }
    } else {
        tv->search_result_key_requested = U128_ZERO;
        tv->search_result_key_visible   = U128_ZERO;
    }

    ////////////////////////////////////////////////
    // Draw.

    f32 margin = 10.0f;
    ui_cut_right(&r, margin);
    ui_draw_boxes(root, r, r);

    YO_PROFILE_END(edith_dialog_find);
}

////////////////////////////////////////////////////////////////
// rune: Find all

#if 0
static void dialog_find_all(rect r, edith_dialog_find_data *data, edith_finder *finder, u64 selected_result_idx, edith_textview *editor) {
    unused(selected_result_idx);

    YO_PROFILE_BEGIN(dialog_find_all);

    theme theme = default_theme();
    unused(theme);

    bool restart = false;

    ui_box *root = ui_new_box(ui_id("dialog_find_all"));
    ui_set_background(rgba(0, 0, 0, 200));
    ui_children() {
        ui_layout_y() {
            ui_id txt1_id = ui_id("txt1");
            if (ui_get_active_id() == 0) {
                ui_set_active_id(txt1_id);
            }
            if (ui_text_field_b(txt1_id, data->text_buf, sizeof(data->text_buf), &data->text_len, str("Find string"))) {
                restart = true;
            }

            YO_PROFILE_BEGIN(list1);
            ui_list(ui_id("list1")) {
                for_list (edith_finder_result_chunk, chunk, finder->chunks) {
                    for_n (u64, i, chunk->count) {
                        ui_list_item_signal signal = { 0 };
                        ui_list_item(ui_id(chunk->results[i]), &signal) {

                            u64_range match_range = {
                                .min = chunk->results[i],
                                .max = chunk->results[i] + editor->current_search_needle_len,
                            };

                            // Get line string.
                            i64       line_number = edith_textbuf_row_from_pos(&editor->tb, match_range.min);
                            u64_range line_range  = edith_textbuf_pos_range_from_row(&editor->tb, line_number);
                            str       line        = edith_str_from_gapbuffer_range(&editor->tb.gb, line_range, ui_arena());

                            // Trim left.
                            while (line.len && u8_is_whitespace(line.v[0])) {
                                line.v++;
                                line.len--;
                                line_range.min++;
                            }

                            // Trim right.
                            while (line.len && u8_is_whitespace(line.v[line.len - 1])) {
                                line.len--;
                                line_range.max--;
                            }

                            // Construct fake fuzzy match list.
                            fuzzy_match_node match_node = {
                                .range.min = match_range.min - line_range.min,
                                .range.max = match_range.max - line_range.min,
                            };
                            fuzzy_match_list match_list = {
                                .first = &match_node,
                                .count = 1,
                            };

                            // Draw fake fuzzy match.
                            ui_set_face(ui_make_face(global_mono_font, 12));
                            ui_fuzzy_matched_string(line, match_list);
                        }

                        if (signal.clicked) {
                            u64 goto_offset = chunk->results[i];
                            edith_push_cmd(EDITH_CMD_KIND_GOTO_POS, goto_offset);
                        }
                    }
                }
            }
            YO_PROFILE_END(list1);
        }
    }

    if (restart) {
        // TODO(rune): Run query
    }

    ////////////////////////////////////////////////
    // Draw.

    ui_draw_boxes(root, r, r);

    YO_PROFILE_END(dialog_find_all);
}
#endif

////////////////////////////////////////////////////////////////
// rune: Go to line dialog

static void edith_dialog_goto_line(rect r, edith_dialog_goto_line_data *data) {
    theme theme = default_theme();

    ui_box *root = ui_new_box(ui_id("dialog_goto_line"));
    ui_set_align(UI_ALIGN_TOP_RIGHT);
    ui_set_dim_x(ui_len_abs(200));
    ui_set_child_axis(AXIS2_X);
    ui_set_background(theme.bg_main);
    ui_set_shadow(10);
    ui_children_begin();

    ////////////////////////////////////////////////
    // Search box.

    bool format_ok = false;
    i64 parsed = 0;

    ui_id txt = ui_id("txt1");
    ui_set_active_id(txt);
    ui_text_field_b(txt, data->text_buf, sizeof(data->text_buf), &data->text_len, str("Go to line"));
    ui_set_dim_x(ui_len_rel(1));

    str s = str_make(data->text_buf, data->text_len);
    if (fmt_parse_u64(s, (u64 *)&parsed)) {
        if (parsed > 0) {
            format_ok = true;
        }
    }

    if (!format_ok && !data->text_len == 0) {
        ui_set_border(1, theme.bg_bad, 0);
    }

    ui_children_end();

    ////////////////////////////////////////////////
    // Interaction.

    ui_events events = ui_get_events();
    for_array (ui_event, e, events) {
        if (ui_event_is_keypress_with_modifiers(e, UI_KEY_ENTER, UI_MODIFIER_NONE)) {
            if (format_ok && data->text_len > 0) {
                // NOTE(rune): User types 1-based linenumber, but the rest of the code
                // assumes 0-based linenumbers.
                edith_cmd_ring_write(&edith_global_cmd_ring, &(edith_cmd) { EDITH_CMD_KIND_GOTO_LINE, parsed - 1 });
            }
        }
    }

    ////////////////////////////////////////////////
    // Draw.

    f32 margin = 10.0f;
    ui_cut_right(&r, margin);
    ui_draw_boxes(root, r, r);
}

////////////////////////////////////////////////////////////////
// rune: Commands dialog

static dialog_command_node *dialog_command_list_push(dialog_command_list *list, arena *arena) {
    dialog_command_node *node = arena_push_struct(arena, dialog_command_node);
    slist_push(list, node);
    list->count++;
    return node;
}

static i32 dialog_command_item_cmp(const dialog_command_item *a, const dialog_command_item *b) {
    return str_cmp_nocase(a->spec->name, b->spec->name);
}

static void edith_dialog_commands(rect client_rect, edith_dialog_commands_data *data) {
    YO_PROFILE_BEGIN(edith_dialog_commands);

    theme theme = default_theme();
    unused(theme);

    arena *arena = ui_arena();
    rect area = calc_popup_area(client_rect);

    bool text_changed = false;
    ui_id id = ui_id("dialog_commands");
    ui_box *root = ui_new_box(id);
    ui_children() {
        ui_popup_frame() {
            ui_layout_y() {
                // Build input field.
                ui_padding(10, 10) {
                    ui_children() {
                        ui_id txt1_id = ui_id("commands_txt");
                        if (ui_get_active_id() == 0) {
                            ui_set_active_id(txt1_id);
                        }
                        text_changed = ui_text_field_b(txt1_id, data->text_buf, sizeof(data->text_buf), &data->text_len, str("Command name"));
                    }
                }

                // Find commands which match the search string.
                str_list needles = str_split_by_whitespace(str_make(data->text_buf, data->text_len), arena);
                dialog_command_list cmd_list = { 0 };
                for_sarray (edith_cmd_spec, spec, edith_cmd_specs) {
                    if (spec->name.len) {  // NOTE(rune): Skip EDITH_CMD_KIND_NONE.
                        fuzzy_match_list fuzzy_matches = fuzzy_match_list_from_str(spec->name, needles, arena);
                        if (fuzzy_matches.count == needles.count) {
                            dialog_command_node *node = dialog_command_list_push(&cmd_list, arena);
                            node->v.spec = spec;
                            node->v.fuzzy_matches = fuzzy_matches;
                        }
                    }
                }

                // Convert to array.
                u64 cmd_count = cmd_list.count;
                dialog_command_item *cmds = arena_push_array(arena, dialog_command_item, cmd_count);
                {
                    u64 i = 0;
                    for (dialog_command_node *node = cmd_list.first; node; node = node->next) {
                        cmds[i++] = node->v;
                    }
                }

                // Sort array.
                qsort(cmds, cmd_count, sizeof(cmds[0]), dialog_command_item_cmp);

                // Build list.
                ui_list(ui_id("list1")) {
                    for (u64 i = 0; i < cmd_count; i++) {
                        ui_id item_id = ui_id((void *)cmds[i].spec);

                        if (i == 0 && text_changed) {
                            ui_set_focused_id(item_id);
                        }

                        if (ui_get_focused_id() == 0) {
                            global_ui_ctx->this_frame.focused_id = item_id;
                        }

                        // Build item.
                        ui_list_item_signal signal = { 0 };
                        ui_list_item(item_id, &signal) {
                            ui_padding(10, 2) {
                                ui_layout_x() {
                                    ui_text_b(str("D")); // TODO(rune): Icon
                                    ui_set_text_color(rgb(150, 150, 150));
                                    ui_set_dim_x(ui_len_abs(25));

                                    ui_fuzzy_matched_string(cmds[i].spec->name, cmds[i].fuzzy_matches);
                                }
                            }
                        }

                        if (signal.clicked) {
                            edith_cmd_kind cmd_kind = cmds[i].spec - edith_cmd_specs;
                            edith_cmd_ring_write(&edith_global_cmd_ring, &(edith_cmd) { cmd_kind });
                        }
                    }
                }

                // Bottom padding
                ui_space_y(5);
            }
        }
    }

    ui_draw_boxes(root, area, client_rect);

    YO_PROFILE_END(edith_dialog_commands);
}

////////////////////////////////////////////////////////////////
// rune: Tab switcher

static str str_chop_file_name(str *s) {
    i64 file_name_start_idx = 0;
    for (i64 i = 0; i < s->count; i++) {
        i64 i_rev = s->count - 1 - i;
        if (s->v[i_rev] == '\\' || s->v[i_rev] == '/') {
            file_name_start_idx = i_rev + 1;
            break;
        }
    }

    str ret = str_chop_right(s, file_name_start_idx);
    return ret;
}

static void ui_cut_and_draw_border(rect *r, ui_border border, u32 color) {
    ui_draw_border(*r, border);
    ui_cut_border(r, border);
    border.color = color;
    ui_draw_border(*r, border);
}

static void edith_dialog_tabswitcher(ui_id id, rect client_rect, edith_editor_tab_list tabs, edith_editor_tab *active_tab) {
    unused(id);

    rect area = calc_popup_area(client_rect);

    theme theme = default_theme();
    unused(theme);

    ui_events events = ui_get_events();
    for_array (ui_event, e, events) {
        if (tabs.first) {
            if (e->type == UI_EVENT_TYPE_KEYPRESS && e->key == UI_KEY_TAB) {
                if (e->modifiers & UI_MODIFIER_SHIFT) {
                    edith_cmd_ring_write(&edith_global_cmd_ring, &(edith_cmd) { EDITH_CMD_KIND_PREV_TAB });
                } else {
                    edith_cmd_ring_write(&edith_global_cmd_ring, &(edith_cmd) { EDITH_CMD_KIND_NEXT_TAB });
                }
            }
        }
    }

    ui_box *root = ui_new_box(0);
    ui_children() {
        ui_popup_frame() {
            ui_padding(0, 20) {
                ui_list(ui_id("tab_list")) {
                    for_list (edith_editor_tab, tab, tabs) {
                        ui_id item_id = ui_id((void *)tab);
                        if (tab == active_tab) {
                            ui_set_focused_id(item_id);
                        }

                        ui_list_item_signal signal = { 0 };
                        ui_list_item(item_id, &signal) {
                            str path = tab->file_name;
                            str file = str_chop_file_name(&path);
                            ui_layout_y() {
                                ui_set_padding_x(10);
                                ui_text_b(file);
                                ui_text_b(tab->file_name);
                                ui_set_text_color(rgb(100, 100, 100));
                            }
                        }

                        if (signal.clicked) {
                            edith_cmd_ring_write(&edith_global_cmd_ring, &(edith_cmd) { EDITH_CMD_KIND_PREV_TAB, u64_from_ptr(tab) });
                        }
                    }
                }
            }
        }
    }

    ui_draw_boxes(root, area, client_rect);
}

////////////////////////////////////////////////////////////////
// rune: Open file dialog

static i32 cmd_dialog_open_file_item(const void *a_, const void *b_) {
    const dialog_open_file_item *a = a_;
    const dialog_open_file_item *b = b_;

    if (a->match_idx != b->match_idx) {
        return a->match_idx - b->match_idx;
    } else {
        i32 cmp = _strnicmp((char *)a->string.v, (char *)b->string.v, min(a->string.len, b->string.len));
        if (cmp != 0) {
            return cmp;
        } else {
            return (i32)(a->string.len - b->string.len);
        }
    }
}

// NOTE(rune): If there's a large number of matching files/directories in the
// search path, we prioritize by earliest match in the file/directory name.
static dialog_open_file_item *dialog_open_file_alloc_item(edith_dialog_open_file_data *data, i32 match_idx, i32 *highest_match_idx) {
    dialog_open_file_item *item = null;
    if (match_idx != -1) {
        if (data->item_count < countof(data->items)) {
            item = &data->items[data->item_count++];
        } else if (match_idx < *highest_match_idx) {
            for_narray (dialog_open_file_item, it, data->items, data->item_count) {
                if (!item && it->match_idx == *highest_match_idx) {
                    item = it;
                    item->match_idx = match_idx;
                    *highest_match_idx = 0;
                }
                max_assign(highest_match_idx, it->match_idx);
            }
        }
    }

    if (item) {
        zero_struct(item);
        item->match_idx = match_idx;
        max_assign(highest_match_idx, match_idx);
    }

    return item;
}

static void edith_dialog_open_file_refresh(edith_dialog_open_file_data *data) {
    // Clear existing items.

    data->need_refresh = false;
    data->item_count = 0;
    if (data->scratch == null) {
        data->scratch = edith_arena_create_default(str("dialog_open_file/scratch"));
    }

    arena *arena = data->scratch;
    arena_reset(arena);

    i32 highest_match_idx = 0;
    str search_name = str_make(data->name_buf, data->name_len);
    str search_path = str_make(data->path_buf, data->path_len);

    // Query either files+directories or logical drives.
    if (search_path.len) {
        os_file_info_list infos = os_get_files_from_path(search_path, I64_MAX, arena);
        for_list (os_file_info, info, infos) {
            i64 match_idx = str_idx_of_str_nocase(info->name, search_name);
            if (match_idx != -1) {
                dialog_open_file_item *item = dialog_open_file_alloc_item(data, (i32)match_idx, &highest_match_idx);
                if (item) {
                    item->string = info->name;
                    item->type = (info->flags & OS_FILE_FLAG_DIRECTORY) ? UI_LISTER_ITEM_TYPE_DIRECTORY : UI_LISTER_ITEM_TYPE_FILE;
                }
            }
        }
    } else {
        // TODO(rune): This is not very cross platform.
        u32 mask = os_get_logical_drives();
        while (mask) {
            i32 bit = u32_pop_first_bit_set(&mask);
            u32 letter = 'A' + bit;
            str drive_name = arena_print(arena, "%(c):", (char)letter);
            i64 match_idx = str_idx_of_str_nocase(drive_name, search_name);
            if (match_idx != -1) {
                dialog_open_file_item *item = dialog_open_file_alloc_item(data, (i32)match_idx, &highest_match_idx);
                if (item) {
                    item->string = drive_name;
                    item->type = UI_LISTER_ITEM_TYPE_DRIVE;
                }
            }
        }
    }

    // Sort items by match_idx.
    qsort(data->items, data->item_count, sizeof(dialog_open_file_item), cmd_dialog_open_file_item);
}

static void edith_dialog_open_file_init(edith_dialog_open_file_data *data) {
    if (!data->init) {
        data->init = true;
        data->item_count = 0;

        edith_dialog_open_file_refresh(data);
    }
}

static void edith_dialog_open_file(rect client_rect, ui_id id, edith_dialog_open_file_data *data, bool is_active, ui_events events) {
    unused(id, is_active);

    rect area = calc_popup_area(client_rect);

    edith_dialog_open_file_init(data);

    // TODO(rune): Automatically replace forward-slash with back-slash on windows

    bool should_reset_caret = false;
    for_array (ui_event, e, events) {
        // Pop last part of path.
        if (ui_event_is_keypress(e, UI_KEY_BACKSPACE) && data->name_len == 0) {
            str path = str_make(data->path_buf, data->path_len);
            i64 path_sep_idx = str_idx_of_last_u8(path, '\\');
            if (path_sep_idx != -1) {
                data->path_len = path_sep_idx;
            } else {
                data->path_len = 0;
            }

            data->need_refresh = true;
            should_reset_caret = true;
        }
    }

    theme theme = default_theme();
    unused(theme);

    ui_popup_frame_box();
    ui_box *root = ui_get_selected_box();
    ui_set_child_axis(AXIS2_Y);
    ui_children() {
        // Search field.
        ui_padding(10, 10) {
            ui_field_frame() {
                ui_layout_x() {
                    // Path bubbles.
                    u32 bubble_bg = rgb(0, 60, 60);
                    str path = str_make(data->path_buf, data->path_len);
                    while (path.len) {
                        str part = str_chop_by_delim(&path, str("\\"));
                        ui_text_b(part);
                        ui_set_padding_x(5);
                        ui_set_border(0, bubble_bg, 3);

                        ui_space_x(5);
                    }

                    // Text input.
                    ui_new_box(ui_id("txt_container"));
                    ui_set_child_axis(AXIS2_X);
                    ui_children() {
                        ui_text_edit_data *txt_data = ui_get_data(ui_text_edit_data);
                        if (data->should_reset_caret) {
                            // NOTE(rune): Restart caret animation, when path bubble has
                            // been added/removed, so that caret doens't jump.
                            txt_data->animated_caret_x.started = false;
                            data->should_reset_caret = false;
                        }

                        ui_id txt_id = ui_id("txt");
                        if (ui_get_active_id() == 0) {
                            ui_set_active_id(txt_id);
                        }

                        bool text_changed = ui_text_edit_b(txt_id, data->name_buf, sizeof(data->name_buf), &data->name_len, txt_data);
                        if (text_changed) {
                            data->animated_scroll.pos = 0.0f;
                            data->target_scroll = 0.0f;
                            data->need_refresh = true;
                        }

                        if (data->need_refresh) {
                            edith_dialog_open_file_refresh(data);
                        }


                        if (data->item_count == 1 && data->items[0].match_idx == 0) {
                            str hint = substr_idx(data->items[0].string, data->name_len);
                            ui_text_b(hint);
                            ui_set_text_color(rgb(100, 100, 100));
                        }
                    }
                }
            }
        }

        // Animate.
        ui_anim_f32(&data->animated_scroll, data->target_scroll, 30.0f);

        // Items.
        str pattern = str_make(data->name_buf, data->name_len);
        ui_list(ui_id("file_list")) {
            for_n (i64, i, data->item_count) {
                dialog_open_file_item *item = &data->items[i];
                ui_id item_id = ui_id(item->string);
                if (ui_get_focused_id() == 0) {
                    ui_set_focused_id(item_id);
                }

                // Build item.
                ui_list_item_signal signal = { 0 };
                ui_list_item(item_id, &signal) {
                    ui_padding(10, 2) {
                        ui_layout_x() {
                            str icon = str("");
                            if (item->type == UI_LISTER_ITEM_TYPE_DIRECTORY) icon = str("D");
                            if (item->type == UI_LISTER_ITEM_TYPE_DRIVE) icon = str("L");

                            ui_text_b(icon);
                            ui_set_text_color(rgb(150, 150, 150));
                            ui_set_dim_x(ui_len_abs(25));

                            str_x3 tri = str_split_x3(item->string, item->match_idx, item->match_idx + pattern.len);
                            ui_text_b(tri.v[0]);
                            ui_text_b(tri.v[1]); ui_set_text_color(rgb(150, 255, 255));
                            ui_text_b(tri.v[2]);
                        }
                    }
                }

                // Action on selected item.
                if (signal.clicked) {
                    if (item->type == UI_LISTER_ITEM_TYPE_FILE) {
                        // Open file.
                        str name = item->string;
                        str path = str_make(data->path_buf, data->path_len);
                        str file_name = arena_print(ui_arena(), "%\\%", path, name);

                        edith_cmd cmd = { EDITH_CMD_KIND_OPEN_FILE };
                        cmd.body = file_name;
                        edith_cmd_ring_write(&edith_global_cmd_ring, &cmd);
                    } else {
                        // Append to path.
                        buf buf = make_buf(data->path_buf, sizeof(data->path_buf), data->path_len);
                        str insert = item->string;
                        if (data->path_len > 0) {
                            buf_append_str(&buf, str("\\"));
                        }
                        buf_append_str(&buf, insert);
                        data->path_len = buf.len;
                        data->name_len = 0;
                        data->need_refresh = true;
                        data->should_reset_caret = true;
                    }
                }
            }
        }
    }

    ui_draw_boxes(root, area, client_rect);
}
