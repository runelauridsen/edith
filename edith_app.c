////////////////////////////////////////////////////////////////
// rune: Performance timing

static void edith_perf_timing_begin(edith_perf_timings *timings) {
    timings->t_begin = os_get_performance_timestamp();
}

static void edith_perf_timing_end(edith_perf_timings *timings) {
    timings->t_end = os_get_performance_timestamp();
    timings->samples[timings->rolling_idx] = os_get_millis_between(timings->t_begin, timings->t_end);

    timings->rolling_idx = (timings->rolling_idx + 1) % countof(timings->samples);
    timings->samples_count = min(timings->samples_count + 1, countof(timings->samples));

    f64 min = F64_MAX;
    f64 max = F64_MIN;
    f64 sum = 0.0f;

    for_narray (f64, sample, timings->samples, timings->samples_count) {
        min_assign(&min, *sample);
        max_assign(&max, *sample);
        sum += *sample;
    }

    f64 avg = sum / (f64)timings->samples_count;

    timings->min = min;
    timings->max = max;
    timings->avg = avg;
}

////////////////////////////////////////////////////////////////
// rune: Editor tabs

static edith_editor_tab *edith_alloc_editor_tab(edith_app *app) {
    edith_editor_tab *ret = null;
    if (app->first_free_editor_tab) {
        ret = app->first_free_editor_tab;
        slstack_pop(&app->first_free_editor_tab);
    }

    if (!ret) {
        ret = arena_push_struct(app->perm, edith_editor_tab);
    }

    zero_struct(ret);
    edith_textview_create(&ret->v);
    return ret;
}

static void edith_free_editor_tab(edith_app *app, edith_editor_tab *editor) {
    edith_textview_destroy(&editor->v);
    zero_struct(editor);
    slstack_push(&app->first_free_editor_tab, editor);
}

////////////////////////////////////////////////////////////////
// rune: Command handlers

static void edith_app_gather_commands(edith_app *app) {
    // debug
    if (app->ui_ctx.frame_idx == 1) {
        //edith_push_cmd(EDITH_CMD_KIND_OPEN_FILE, str("W:\\edith\\main.c"));
        //edith_push_cmd(EDITH_CMD_KIND_OPEN_FILE, str("W:\\edith\\indexer\\test3.h"));
        //edith_push_cmd(EDITH_CMD_KIND_OPEN_FILE, str("C:\\users\\runel\\downloads\\miniaudio.h"));

        //edith_push_cmd(EDITH_CMD_KIND_SELECT_RANGE, (i64)30, (i64)33);
        //edith_push_cmd(EDITH_CMD_KIND_GOTO_LINE, (i64)40200);
        //edith_push_cmd(EDITH_CMD_KIND_GOTO_POS, (i64)1593303);

        //edith_push_cmd(EDITH_CMD_KIND_GOTO_POS, (i64)218);
        //edith_push_cmd(EDITH_CMD_KIND_SUBMIT_STR, str("{"));
    }

    ////////////////////////////////////////////////
    // Editor commands.

    ui_events events = ui_get_events();

    typedef struct edith_mapped_key edith_mapped_key;
    struct edith_mapped_key {
        ui_key key;
        ui_modifiers modifiers;
        edith_cmd_kind cmd_kind;
    };

    static readonly edith_mapped_key always_keymap[] = {
        { UI_KEY_O,             UI_MODIFIER_CTRL,                       EDITH_CMD_KIND_SHOW_OPEN_FILE_DIALOG,            },
        { UI_KEY_F,             UI_MODIFIER_CTRL,                       EDITH_CMD_KIND_SHOW_FIND_DIALOG,                 },
        { UI_KEY_F,             UI_MODIFIER_CTRL|UI_MODIFIER_SHIFT,     EDITH_CMD_KIND_SHOW_FIND_ALL_DIALOG,             },
        { UI_KEY_G,             UI_MODIFIER_CTRL,                       EDITH_CMD_KIND_SHOW_GOTO_LINE_DIALOG,            },
        { UI_KEY_P,             UI_MODIFIER_CTRL|UI_MODIFIER_SHIFT,     EDITH_CMD_KIND_SHOW_COMMAND_DIALOG,              },
        { UI_KEY_ESCAPE,        0,                                      EDITH_CMD_KIND_CLOSE_DIALOG,                     },
    };

    static readonly edith_mapped_key editor_keymap[] = {
        // rune: Move
        { UI_KEY_LEFT,          0,                                      EDITH_CMD_KIND_MOVE_LEFT                        },
        { UI_KEY_RIGHT,         0,                                      EDITH_CMD_KIND_MOVE_RIGHT                       },
        { UI_KEY_UP,            0,                                      EDITH_CMD_KIND_MOVE_UP                          },
        { UI_KEY_DOWN,          0,                                      EDITH_CMD_KIND_MOVE_DOWN                        },

        { UI_KEY_LEFT,          UI_MODIFIER_CTRL,                       EDITH_CMD_KIND_MOVE_BY_WORD_LEFT                },
        { UI_KEY_RIGHT,         UI_MODIFIER_CTRL,                       EDITH_CMD_KIND_MOVE_BY_WORD_RIGHT               },
        { UI_KEY_UP,            UI_MODIFIER_CTRL,                       EDITH_CMD_KIND_MOVE_BY_PARAGRAPH_UP             },
        { UI_KEY_DOWN,          UI_MODIFIER_CTRL,                       EDITH_CMD_KIND_MOVE_BY_PARAGRAPH_DOWN           },
        { UI_KEY_HOME,          0,                                      EDITH_CMD_KIND_MOVE_TO_LINE_LEFT                },
        { UI_KEY_END,           0,                                      EDITH_CMD_KIND_MOVE_TO_LINE_RIGHT               },
        { UI_KEY_PAGE_UP,       0,                                      EDITH_CMD_KIND_MOVE_BY_PAGE_UP                  },
        { UI_KEY_PAGE_DOWN,     0,                                      EDITH_CMD_KIND_MOVE_BY_PAGE_DOWN                },
        { UI_KEY_HOME,          UI_MODIFIER_CTRL,                       EDITH_CMD_KIND_MOVE_TO_DOCUMENT_START           },
        { UI_KEY_END,           UI_MODIFIER_CTRL,                       EDITH_CMD_KIND_MOVE_TO_DOCUMENT_END             },

        // rune: Select
        { UI_KEY_LEFT,          UI_MODIFIER_SHIFT,                      EDITH_CMD_KIND_SELECT_LEFT                      },
        { UI_KEY_RIGHT,         UI_MODIFIER_SHIFT,                      EDITH_CMD_KIND_SELECT_RIGHT                     },
        { UI_KEY_UP,            UI_MODIFIER_SHIFT,                      EDITH_CMD_KIND_SELECT_UP                        },
        { UI_KEY_DOWN,          UI_MODIFIER_SHIFT,                      EDITH_CMD_KIND_SELECT_DOWN                      },

        { UI_KEY_LEFT,          UI_MODIFIER_SHIFT|UI_MODIFIER_CTRL,     EDITH_CMD_KIND_SELECT_BY_WORD_LEFT              },
        { UI_KEY_RIGHT,         UI_MODIFIER_SHIFT|UI_MODIFIER_CTRL,     EDITH_CMD_KIND_SELECT_BY_WORD_RIGHT             },
        { UI_KEY_UP,            UI_MODIFIER_SHIFT|UI_MODIFIER_CTRL,     EDITH_CMD_KIND_SELECT_BY_PARAGRAPH_UP           },
        { UI_KEY_DOWN,          UI_MODIFIER_SHIFT|UI_MODIFIER_CTRL,     EDITH_CMD_KIND_SELECT_BY_PARAGRAPH_DOWN         },
        { UI_KEY_HOME,          UI_MODIFIER_SHIFT,                      EDITH_CMD_KIND_SELECT_TO_LINE_LEFT              },
        { UI_KEY_END,           UI_MODIFIER_SHIFT,                      EDITH_CMD_KIND_SELECT_TO_LINE_RIGHT             },
        { UI_KEY_PAGE_UP,       UI_MODIFIER_SHIFT,                      EDITH_CMD_KIND_SELECT_BY_PAGE_UP                },
        { UI_KEY_PAGE_DOWN,     UI_MODIFIER_SHIFT,                      EDITH_CMD_KIND_SELECT_BY_PAGE_DOWN              },
        { UI_KEY_HOME,          UI_MODIFIER_SHIFT|UI_MODIFIER_CTRL,     EDITH_CMD_KIND_SELECT_TO_DOCUMENT_START         },
        { UI_KEY_END,           UI_MODIFIER_SHIFT|UI_MODIFIER_CTRL,     EDITH_CMD_KIND_SELECT_TO_DOCUMENT_END           },

        { UI_KEY_A,             UI_MODIFIER_CTRL,                       EDITH_CMD_KIND_SELECT_ALL                       },

        // rune: Delete
        { UI_KEY_DELETE,        0,                                      EDITH_CMD_KIND_DELETE_LEFT,                     },
        { UI_KEY_BACKSPACE,     0,                                      EDITH_CMD_KIND_DELETE_RIGHT,                    },
        { UI_KEY_DELETE,        UI_MODIFIER_CTRL,                       EDITH_CMD_KIND_DELETE_BY_WORD_LEFT,             },
        { UI_KEY_BACKSPACE,     UI_MODIFIER_CTRL,                       EDITH_CMD_KIND_DELETE_BY_WORD_RIGHT,            },

        // rune: Undo/redo
        { UI_KEY_Z,             UI_MODIFIER_CTRL,                       EDITH_CMD_KIND_UNDO,                            },
        { UI_KEY_Y,             UI_MODIFIER_CTRL,                       EDITH_CMD_KIND_REDO,                            },

        // rune: Cursors.
        { UI_KEY_UP,            UI_MODIFIER_SHIFT|UI_MODIFIER_ALT,      EDITH_CMD_KIND_ADD_CURSOR_ABOVE,                },
        { UI_KEY_DOWN,          UI_MODIFIER_SHIFT|UI_MODIFIER_ALT,      EDITH_CMD_KIND_ADD_CURSOR_BELOW,                },
        { UI_KEY_D,             UI_MODIFIER_CTRL,                       EDITH_CMD_KIND_ADD_CURSOR_AT_NEXT_OCCURENCE,    },
        { UI_KEY_D,             UI_MODIFIER_CTRL|UI_MODIFIER_SHIFT,     EDITH_CMD_KIND_ADD_CURSOR_AT_PREV_OCCURENCE,    },
        { UI_KEY_ESCAPE,        0,                                      EDITH_CMD_KIND_CLEAR_CURSORS,                   },
        { UI_KEY_M,             UI_MODIFIER_CTRL,                       EDITH_CMD_KIND_FLIP_CURSOR,                     },

        // rune: Tab
        { UI_KEY_S,             UI_MODIFIER_CTRL,                       EDITH_CMD_KIND_SAVE_ACTIVE_TAB,                 },
        { UI_KEY_W,             UI_MODIFIER_CTRL,                       EDITH_CMD_KIND_CLOSE_ACTIVE_TAB,                },

        // rune: Pane
        { UI_KEY_K,             UI_MODIFIER_CTRL,                       EDITH_CMD_KIND_NEXT_PANE,                       },
        { UI_KEY_K,             UI_MODIFIER_CTRL|UI_MODIFIER_SHIFT,     EDITH_CMD_KIND_PREV_PANE,                       },

        // rune: Misc move
        { UI_KEY_R,             UI_MODIFIER_CTRL,                       EDITH_CMD_KIND_MOVE_TO_NEXT_OCCURENCE,          },
        { UI_KEY_R,             UI_MODIFIER_CTRL|UI_MODIFIER_SHIFT,     EDITH_CMD_KIND_MOVE_TO_PREV_OCCURENCE,          },

        // rune: Clipboard
        { UI_KEY_X,             UI_MODIFIER_CTRL,                       EDITH_CMD_KIND_CUT,                             },
        { UI_KEY_C,             UI_MODIFIER_CTRL,                       EDITH_CMD_KIND_COPY,                            },
        { UI_KEY_V,             UI_MODIFIER_CTRL,                       EDITH_CMD_KIND_PASTE,                           },
    };

    for_array (ui_event, e, events) {
        if (e->type == UI_EVENT_TYPE_KEYPRESS) {
            for_sarray (edith_mapped_key, k, always_keymap) {
                if ((k->key == e->key) && (k->modifiers == e->modifiers)) {
                    edith_cmd cmd = { .kind = k->cmd_kind };
                    edith_cmd_ring_write(&edith_global_cmd_ring, &cmd);
                }
            }

            if (app->state == EDITH_APP_STATE_DEFAULT) {
                for_sarray (edith_mapped_key, k, editor_keymap) {
                    if ((k->key == e->key) && (k->modifiers == e->modifiers)) {
                        edith_cmd cmd = { .kind = k->cmd_kind };
                        edith_cmd_ring_write(&edith_global_cmd_ring, &cmd);
                    }
                }
            }
        }
    }
}

static edith_editor_tab *edith_app_get_active_editor(edith_app *app) {
    edith_editor_tab *active_tab = null;
    if (app->active_pane) {
        active_tab = app->active_pane->tab;
    }
    return active_tab;
}

// TODO(rune): Wrapping
static i64 edith_find_next_occurence_of_selection(edith_textview *editor, edith_cursor cursor, dir dir) {
    i64 len = edith_textbuf_len(&editor->tb);
    i64 ret = -1;
    i64_range selection = edith_cursor_range(cursor);
    if ((selection.min != selection.max) && (
        (dir == DIR_FORWARD && selection.max < len) ||
        (dir == DIR_BACKWARD && selection.min > 0)
        )) {

        arena *temp = editor->local;
        arena_scope_begin(temp);

        str selected_text = edith_str_from_gapbuffer_range(&editor->tb.gb, selection, temp);
        i64_range range = { 0 };
        if (dir == DIR_FORWARD) {
            range = i64_range(selection.max, I64_MAX);
        }

        if (dir == DIR_BACKWARD) {
            range = i64_range(0, selection.min + 1);
        }

        ret = edith_next_occurence_in_gapbuffer(&editor->tb.gb, range, selected_text, dir, false, temp);

        arena_scope_end(temp);
    }
    return ret;
}

#pragma warning ( push )
#pragma warning ( error : 4061) // enumerator in switch of enum is not explicitly handled by a case label.
static bool edith_app_handle_command(edith_app *app, edith_cmd *cmd) {
    YO_PROFILE_BEGIN(edith_app_handle_command);

    bool skip = false;
    edith_cmd_spec *spec = edith_cmd_spec_from_kind(cmd->kind);
    if (spec->flags & EDITH_CMD_FLAG_NEED_ACTIVE_EDITOR) {
        edith_editor_tab *active_editor = edith_app_get_active_editor(app);
        if (active_editor == null) {
            skip = true;
        }
    }

    if (!skip) {
        //cmd_print(cmd);
        ui_invalidate_next_frame();

        switch (cmd->kind) {
            case EDITH_CMD_KIND_NONE: {
            } break;

            case EDITH_CMD_KIND_NEXT_TAB: {
                if (app->active_pane) {
                    if (app->active_pane->tab) {
                        app->active_pane->tab = coalesce(app->active_pane->tab->next, app->tabs.first);
                    } else {
                        app->active_pane->tab = app->tabs.first;
                    }
                }
            } break;

            case EDITH_CMD_KIND_PREV_TAB: {
                if (app->active_pane->tab) {
                    app->active_pane->tab = coalesce(app->active_pane->tab->prev, app->tabs.last);
                } else {
                    app->active_pane->tab = app->tabs.last;
                }
            } break;

            case EDITH_CMD_KIND_NEXT_PANE:
            case EDITH_CMD_KIND_PREV_PANE: {
                if (app->active_pane == &app->left_pane) {
                    app->active_pane = &app->right_pane;
                } else {
                    app->active_pane = &app->left_pane;
                }
            } break;

            case EDITH_CMD_KIND_SHOW_OPEN_FILE_DIALOG: {
                app->state = EDITH_APP_STATE_OPEN_FILE;
                app->dialog_open_file.name_len = 0;
                app->dialog_open_file.need_refresh = true;
            } break;

            case EDITH_CMD_KIND_SHOW_GOTO_LINE_DIALOG: {
                app->state = EDITH_APP_STATE_GOTO_LINE;
                app->dialog_goto_line.text_len = 0;
            } break;

            case EDITH_CMD_KIND_SHOW_FIND_DIALOG: {
                app->state = EDITH_APP_STATE_FIND;
                app->dialog_find.text_len = 0; // TODO(rune): Select whole text instead
            } break;

            case EDITH_CMD_KIND_SHOW_FIND_ALL_DIALOG: {
                app->state = EDITH_APP_STATE_FIND_ALL;
                app->dialog_find.text_len = 0; // TODO(rune): Select whole text instead
            } break;

            case EDITH_CMD_KIND_SHOW_COMMAND_DIALOG: {
                app->state = EDITH_APP_STATE_COMMAND_PALLETTE;
                app->dialog_commands.text_len = 0;
            } break;

            case EDITH_CMD_KIND_CLOSE_DIALOG: {
                app->state = EDITH_APP_STATE_DEFAULT;
            } break;

            case EDITH_CMD_KIND_OPEN_FILE: {
                str file_name = cmd->body;

                if (app->active_pane == null) {
                    app->active_pane = &app->left_pane;
                }

                edith_editor_tab *tab = edith_alloc_editor_tab(app);
                dlist_push(&app->tabs, tab);
                app->active_pane->tab = tab;

                edith_textview *editor = &tab->v;
                edith_textview_set_face(editor, app->editor_face);

                arena *temp = app->temp;
                arena_scope(temp) {

                    // TODO(rune): Error handling.
                    str content = os_read_entire_file(file_name, temp, 0);
                    edith_textbuf_create(&editor->tb);
                    edith_textbuf_insert(&editor->tb, 0, content, EDITH_TEXTBUF_EDIT_FLAG_NO_HISTORY);
                }

                app->state = EDITH_APP_STATE_DEFAULT;
                tab->file_name = arena_copy_str(editor->local, file_name);
            } break;

            case EDITH_CMD_KIND_SAVE_ACTIVE_TAB:
            case EDITH_CMD_KIND_SAVE_TAB: {
                edith_editor_tab *tab;
                if (cmd->kind == EDITH_CMD_KIND_SAVE_ACTIVE_TAB) {
                    tab = app->active_pane->tab;
                } else {
                    tab = ptr_from_u64(cmd->args[0]);
                }
                edith_gapbuffer *gb = &tab->v.tb.gb;
                arena *temp = app->temp;
                arena_scope(temp) {
                    str full_text = edith_doc_concat_fulltext(gb, temp); // @LargeFiles
                    bool succeeded = false;
                    os_write_entire_file(tab->file_name, full_text, temp, &succeeded);
                    if (succeeded) {
                        ui_post_toast(0, ui_fmt("Saved file '%'.", tab->file_name));
                    } else {
                        ui_post_toast(0, ui_fmt("Could not save file '%'.", tab->file_name));
                    }
                }
            } break;

            case EDITH_CMD_KIND_CLOSE_ACTIVE_TAB:
            case EDITH_CMD_KIND_CLOSE_TAB: {
                edith_editor_tab *tab;
                if (cmd->kind == EDITH_CMD_KIND_CLOSE_ACTIVE_TAB) {
                    tab = app->active_pane->tab;
                } else {
                    tab = ptr_from_u64(cmd->args[0]);
                }
                edith_textview_submit_barrier(&tab->v);

                if (app->left_pane.tab  == tab) app->left_pane.tab  = null;
                if (app->right_pane.tab == tab) app->right_pane.tab = null;

                // TODO(rune): Check for unsaved changes.
                dlist_remove(&app->tabs, tab);
                edith_free_editor_tab(app, tab);
            } break;

            case EDITH_CMD_KIND_SELECT_TAB: {
                edith_editor_tab *tab = ptr_from_u64(cmd->args[0]);
                app->active_pane->tab = tab;
            } break;

            case EDITH_CMD_KIND_FIND_NEXT:
            case EDITH_CMD_KIND_FIND_PREV: {
                edith_textview *tv  = &app->active_pane->tab->v;

                edith_textview_cursors_clear_secondary(tv);

                bool completed_request = 0;
                edith_search_result_array search_results = edith_search_results_from_key(tv->search_result_key_requested, &completed_request, 1);
                if (completed_request) {
                    if (search_results.count > 0) {
                        edith_cursor *cursor = edith_textview_cursors_get_primary(tv);
                        i64 cursor_pos = edith_cursor_range(*cursor).min;

                        // rune: Find next/prev result.
                        i64 result_idx = 0;
                        switch (cmd->kind) {
                            case EDITH_CMD_KIND_FIND_NEXT: {
                                result_idx = edith_search_result_idx_from_pos(search_results, cursor_pos + 1).max;
                                if (result_idx == -1) {
                                    result_idx = 0;
                                }
                            } break;

                            case EDITH_CMD_KIND_FIND_PREV: {
                                result_idx = edith_search_result_idx_from_pos(search_results, cursor_pos - 1).min;
                                if (result_idx == -1) {
                                    result_idx = search_results.count - 1;
                                }
                            } break;
                        }
                        edith_search_result result = search_results.v[result_idx];

                        // rune: Select and scroll to found result.
                        edith_textview_move_to_pos(tv, result.range.min, false);
                        edith_textview_move_to_pos(tv, result.range.max, true);
                        edith_textview_scroll_to_primary_cursor(tv, EDITH_TEXTVIEW_SCROLL_TARGET_CENTER);
                    }
                }
            } break;

            case EDITH_CMD_KIND_GOTO_LINE: {
                // rune: Unpack command
                i64 linenumber = cmd->args[0];

                // rune: Find pos
                edith_textview *editor = &app->active_pane->tab->v;
                i64 max_linenumber = edith_textbuf_row_count(&editor->tb);
                i64 goto_linenumber = min(linenumber, max_linenumber);
                i64 goto_pos = edith_textbuf_pos_from_row(&editor->tb, goto_linenumber);

                // rune: Move
                edith_textview_move_to_pos(editor, goto_pos, false);
                edith_textview_scroll_to_primary_cursor(editor, EDITH_TEXTVIEW_SCROLL_TARGET_CENTER);
            } break;

            case EDITH_CMD_KIND_GOTO_POS: {
                // rune: Unpack command
                i64 pos = cmd->args[0];
                edith_textview *editor = &app->active_pane->tab->v;

                // rune: Move
                edith_textview_move_to_pos(editor, pos, false);
                edith_textview_scroll_to_primary_cursor(editor, EDITH_TEXTVIEW_SCROLL_TARGET_CENTER);
            } break;

            case EDITH_CMD_KIND_SELECT_RANGE: {
                // rune: Unpack command
                i64 min = cmd->args[0];
                i64 max = cmd->args[1];

                // rune: Set selection
                edith_textview *editor = &app->active_pane->tab->v;
                edith_textview_cursors_clear_secondary(editor);
                edith_cursor *cursor = edith_textview_cursors_get_primary(editor);
                cursor->mark = min;
                cursor->caret = max;
                edith_textview_scroll_to_primary_cursor(editor, EDITH_TEXTVIEW_SCROLL_TARGET_CENTER);
            } break;

            case EDITH_CMD_KIND_UNDO: { edith_textview_unredo(&app->active_pane->tab->v, EDITH_UNREDO_UNDO); } break;
            case EDITH_CMD_KIND_REDO: { edith_textview_unredo(&app->active_pane->tab->v, EDITH_UNREDO_REDO); } break;

            case EDITH_CMD_KIND_ADD_CURSOR_ABOVE: { edith_textview_add_cursor_at_row_offset(&app->active_pane->tab->v, -1); } break;
            case EDITH_CMD_KIND_ADD_CURSOR_BELOW: { edith_textview_add_cursor_at_row_offset(&app->active_pane->tab->v, +1); } break;

            case EDITH_CMD_KIND_CLEAR_CURSORS: {
                edith_textview *editor = &app->active_pane->tab->v;
                edith_textview_cursors_clear_secondary(editor);
                edith_editor_clear_selections(editor);
            } break;

            case EDITH_CMD_KIND_FLIP_CURSOR: {
                edith_textview *editor = &app->active_pane->tab->v;
                for_array (edith_cursor, it, editor->cursors) {
                    i64 temp  = it->caret;
                    it->caret = it->mark;
                    it->mark  = temp;
                }
            } break;

            case EDITH_CMD_KIND_ADD_CURSOR_AT_NEXT_OCCURENCE:
            case EDITH_CMD_KIND_ADD_CURSOR_AT_PREV_OCCURENCE: {
                edith_textview *editor = &app->active_pane->tab->v;

                // NOTE(rune): First/last cursor in array is also first/last cursor in document.
                edith_cursor parent = { 0 };
                dir dir;
                if (cmd->kind == EDITH_CMD_KIND_ADD_CURSOR_AT_NEXT_OCCURENCE) {
                    dir = DIR_FORWARD;
                    parent = array_last(editor->cursors);
                } else {
                    dir = DIR_BACKWARD;
                    parent = array_first(editor->cursors);
                }

                // TODO(rune): If nothing is selected, use current word.
                i64 pos = edith_find_next_occurence_of_selection(editor, parent, dir);
                if (pos != -1) {
                    i64_range selection = edith_cursor_range(parent);
                    i64 selection_len = selection.max - selection.min;
                    i64 caret = parent.caret <  parent.mark ? pos : pos + selection_len;
                    i64 mark  = parent.caret >= parent.mark ? pos : pos + selection_len;

                    edith_textview_add_cursor_at_pos(editor, caret, mark, false, &parent);
                }
            } break;

            case EDITH_CMD_KIND_MOVE_TO_NEXT_OCCURENCE:
            case EDITH_CMD_KIND_MOVE_TO_PREV_OCCURENCE: {
                edith_textview *editor = &app->active_pane->tab->v;
                dir dir;
                if (cmd->kind == EDITH_CMD_KIND_MOVE_TO_NEXT_OCCURENCE) {
                    dir = DIR_FORWARD;
                } else {
                    dir = DIR_BACKWARD;
                }

                // TODO(rune): If nothing is selected, we should search for the word, which the caret is currently over.
                for_array (edith_cursor, it, editor->cursors) {
                    i64 pos = edith_find_next_occurence_of_selection(editor, *it, dir);
                    if (pos != -1) {
                        i64_range selection = edith_cursor_range(*it);
                        i64 offset = pos - selection.min;
                        it->caret += offset;
                        it->mark  += offset;
                    }
                }

                edith_textview_cursors_dedup(editor);
                edith_textview_scroll_to_primary_cursor(editor, EDITH_TEXTVIEW_SCROLL_TARGET_CENTER);
            } break;

            case EDITH_CMD_KIND_SELECT_ALL: {
                edith_textview *editor = &app->active_pane->tab->v;
                edith_textview_cursors_clear_secondary(editor);
                edith_textview_move(editor, MOVE_BY_DOCUMENT, DIR_BACKWARD, false);
                edith_textview_move(editor, MOVE_BY_DOCUMENT, DIR_FORWARD, true);
            } break;

            case EDITH_CMD_KIND_CUT:
            case EDITH_CMD_KIND_COPY: {
                arena *temp = app->temp;
                edith_textview *editor = &app->active_pane->tab->v;

                edith_clipboard *clip = &app->clipboard;
                edith_clipboard_reset(clip);

                // TODO(rune): Copy line, if nothing is selected.

                // rune: Selected ranges -> clipboard parts.
                i64 prev_linenumber = 0;
                for_array (edith_cursor, it, editor->cursors) {
                    edith_clipboard_part_flags flags = 0;
                    i64_range range = i64_range(it->caret, it->mark);

                    // rune: Newline flags.
                    i64 curr_linenumber = edith_textbuf_row_from_pos(&editor->tb, range.min);
                    if (curr_linenumber != prev_linenumber && prev_linenumber != 0) {
                        flags |= EDITH_CLIPBOARD_PART_FLAG_NEWLINE;
                    }
                    prev_linenumber = curr_linenumber;

                    // rune: Copy to clipboard arena.
                    str data = edith_str_from_gapbuffer_range(&editor->tb.gb, range, &clip->arena);

                    // rune: Add clipboard part.
                    edith_clipboard_part *part = arena_push_struct(&clip->arena, edith_clipboard_part);
                    part->data = data;
                    part->flags = flags;
                    slist_push(&clip->parts, part);
                }

                // rune: Cut as its own submission.
                if (cmd->kind == EDITH_CMD_KIND_CUT) {
                    edith_editor_delete_selected(editor);
                }

                // rune: Copy to system clipboard.
                str_list strings = { 0 };
                for_list (edith_clipboard_part, part, app->clipboard.parts) {
                    if (part->flags & EDITH_CLIPBOARD_PART_FLAG_NEWLINE) {
                        str_list_push(&strings, temp, str("\r\n")); // TODO(rune): Platform independent line endings.
                    }
                    str_list_push(&strings, temp, part->data);
                }
                str full = str_list_concat(&strings, temp);
                app->callbacks.set_clipboard(full);
            } break;

            case EDITH_CMD_KIND_PASTE: {
                arena *temp         = app->temp;
                edith_textview *editor      = &app->active_pane->tab->v;
                str_list strings = { 0 };

                // rune: Get clipboard data from system clipboard or internal clipboard.
                if (app->callbacks.clipboard_changed_externally()) {
                    str data = app->callbacks.get_clipboard(temp);
                    str_list_push(&strings, temp, data);
                } else {
                    for_list (edith_clipboard_part, part, app->clipboard.parts) {
                        str_list_push(&strings, temp, part->data);
                    }
                }

                // rune: Paste operation as its own submission.
                edith_textview_submit_barrier(editor);
                edith_textview_submit_str_list(editor, strings);
                edith_textview_submit_barrier(editor);
            } break;

            case EDITH_CMD_KIND_EXIT: {
                app->stop = true;
            } break;

            case EDITH_CMD_KIND_SUBMIT_STR: {
                // Unpack command.
                str s = cmd->body;

                edith_textview *editor      = &app->active_pane->tab->v;

                edith_textview_submit_str(editor, s);
            } break;


            case EDITH_CMD_KIND_MOVE_LEFT:
            case EDITH_CMD_KIND_MOVE_RIGHT:
            case EDITH_CMD_KIND_MOVE_UP:
            case EDITH_CMD_KIND_MOVE_DOWN:
            case EDITH_CMD_KIND_MOVE_BY_WORD_LEFT:
            case EDITH_CMD_KIND_MOVE_BY_WORD_RIGHT:
            case EDITH_CMD_KIND_MOVE_BY_PARAGRAPH_UP:
            case EDITH_CMD_KIND_MOVE_BY_PARAGRAPH_DOWN:
            case EDITH_CMD_KIND_MOVE_TO_LINE_LEFT:
            case EDITH_CMD_KIND_MOVE_TO_LINE_RIGHT:
            case EDITH_CMD_KIND_MOVE_BY_PAGE_DOWN:
            case EDITH_CMD_KIND_MOVE_BY_PAGE_UP:
            case EDITH_CMD_KIND_MOVE_TO_DOCUMENT_START:
            case EDITH_CMD_KIND_MOVE_TO_DOCUMENT_END:
            case EDITH_CMD_KIND_SELECT_LEFT:
            case EDITH_CMD_KIND_SELECT_RIGHT:
            case EDITH_CMD_KIND_SELECT_UP:
            case EDITH_CMD_KIND_SELECT_DOWN:
            case EDITH_CMD_KIND_SELECT_BY_WORD_LEFT:
            case EDITH_CMD_KIND_SELECT_BY_WORD_RIGHT:
            case EDITH_CMD_KIND_SELECT_BY_PARAGRAPH_UP:
            case EDITH_CMD_KIND_SELECT_BY_PARAGRAPH_DOWN:
            case EDITH_CMD_KIND_SELECT_TO_LINE_LEFT:
            case EDITH_CMD_KIND_SELECT_TO_LINE_RIGHT:
            case EDITH_CMD_KIND_SELECT_BY_PAGE_DOWN:
            case EDITH_CMD_KIND_SELECT_BY_PAGE_UP:
            case EDITH_CMD_KIND_SELECT_TO_DOCUMENT_START:
            case EDITH_CMD_KIND_SELECT_TO_DOCUMENT_END:
            case EDITH_CMD_KIND_DELETE_LEFT:
            case EDITH_CMD_KIND_DELETE_RIGHT:
            case EDITH_CMD_KIND_DELETE_BY_WORD_LEFT:
            case EDITH_CMD_KIND_DELETE_BY_WORD_RIGHT:
            case EDITH_CMD_KIND_DELETE_TO_LINE_LEFT:
            case EDITH_CMD_KIND_DELETE_TO_LINE_RIGHT: {
                typedef struct movement movement;
                struct movement {
                    move_by by;
                    dir dir;
                    bool select;
                    bool delete;
                };

                static readonly movement movement_table[] = {

                    // rune: Move
                    [EDITH_CMD_KIND_MOVE_LEFT] = { MOVE_BY_CHAR,       DIR_BACKWARD, false, false, },
                    [EDITH_CMD_KIND_MOVE_RIGHT]                 = { MOVE_BY_CHAR,       DIR_FORWARD,  false, false, },
                    [EDITH_CMD_KIND_MOVE_UP]                    = { MOVE_BY_LINE,       DIR_BACKWARD, false, false, },
                    [EDITH_CMD_KIND_MOVE_DOWN]                  = { MOVE_BY_LINE,       DIR_FORWARD,  false, false, },
                    [EDITH_CMD_KIND_MOVE_BY_WORD_LEFT]          = { MOVE_BY_WORD,       DIR_BACKWARD, false, false, },
                    [EDITH_CMD_KIND_MOVE_BY_WORD_RIGHT]         = { MOVE_BY_WORD,       DIR_FORWARD,  false, false, },
                    [EDITH_CMD_KIND_MOVE_BY_PARAGRAPH_UP]       = { MOVE_BY_PARAGRAPH,  DIR_BACKWARD, false, false, },
                    [EDITH_CMD_KIND_MOVE_BY_PARAGRAPH_DOWN]     = { MOVE_BY_PARAGRAPH,  DIR_FORWARD,  false, false, },
                    [EDITH_CMD_KIND_MOVE_TO_LINE_LEFT]          = { MOVE_BY_HOME_END,   DIR_BACKWARD, false, false, },
                    [EDITH_CMD_KIND_MOVE_TO_LINE_RIGHT]         = { MOVE_BY_HOME_END,   DIR_FORWARD,  false, false, },
                    [EDITH_CMD_KIND_MOVE_BY_PAGE_UP]            = { MOVE_BY_PAGE,       DIR_BACKWARD, false, false, },
                    [EDITH_CMD_KIND_MOVE_BY_PAGE_DOWN]          = { MOVE_BY_PAGE,       DIR_FORWARD,  false, false, },
                    [EDITH_CMD_KIND_MOVE_TO_DOCUMENT_START]     = { MOVE_BY_DOCUMENT,   DIR_BACKWARD, false, false, },
                    [EDITH_CMD_KIND_MOVE_TO_DOCUMENT_END]       = { MOVE_BY_DOCUMENT,   DIR_FORWARD,  false, false, },

                    // rune: Select
                    [EDITH_CMD_KIND_SELECT_LEFT]                = { MOVE_BY_CHAR,       DIR_BACKWARD, true,  false, },
                    [EDITH_CMD_KIND_SELECT_RIGHT]               = { MOVE_BY_CHAR,       DIR_FORWARD,  true,  false, },
                    [EDITH_CMD_KIND_SELECT_UP]                  = { MOVE_BY_LINE,       DIR_BACKWARD, true,  false, },
                    [EDITH_CMD_KIND_SELECT_DOWN]                = { MOVE_BY_LINE,       DIR_FORWARD,  true,  false, },
                    [EDITH_CMD_KIND_SELECT_BY_WORD_LEFT]        = { MOVE_BY_WORD,       DIR_BACKWARD, true,  false, },
                    [EDITH_CMD_KIND_SELECT_BY_WORD_RIGHT]       = { MOVE_BY_WORD,       DIR_FORWARD,  true,  false, },
                    [EDITH_CMD_KIND_SELECT_BY_PARAGRAPH_UP]     = { MOVE_BY_PARAGRAPH,  DIR_BACKWARD, true,  false, },
                    [EDITH_CMD_KIND_SELECT_BY_PARAGRAPH_DOWN]   = { MOVE_BY_PARAGRAPH,  DIR_FORWARD,  true,  false, },
                    [EDITH_CMD_KIND_SELECT_TO_LINE_LEFT]        = { MOVE_BY_HOME_END,   DIR_BACKWARD, true,  false, },
                    [EDITH_CMD_KIND_SELECT_TO_LINE_RIGHT]       = { MOVE_BY_HOME_END,   DIR_FORWARD,  true,  false, },
                    [EDITH_CMD_KIND_SELECT_BY_PAGE_UP]          = { MOVE_BY_PAGE,       DIR_BACKWARD, true,  false, },
                    [EDITH_CMD_KIND_SELECT_BY_PAGE_DOWN]        = { MOVE_BY_PAGE,       DIR_FORWARD,  true,  false, },
                    [EDITH_CMD_KIND_SELECT_TO_DOCUMENT_START]   = { MOVE_BY_DOCUMENT,   DIR_BACKWARD, true,  false, },
                    [EDITH_CMD_KIND_SELECT_TO_DOCUMENT_END]     = { MOVE_BY_DOCUMENT,   DIR_FORWARD,  true,  false, },

                    // rune: Delete
                    [EDITH_CMD_KIND_DELETE_LEFT]                = { MOVE_BY_CHAR,       DIR_BACKWARD, false, true, },
                    [EDITH_CMD_KIND_DELETE_RIGHT]               = { MOVE_BY_CHAR,       DIR_FORWARD,  false, true, },
                    [EDITH_CMD_KIND_DELETE_BY_WORD_LEFT]        = { MOVE_BY_WORD,       DIR_BACKWARD, false, true, },
                    [EDITH_CMD_KIND_DELETE_BY_WORD_RIGHT]       = { MOVE_BY_WORD,       DIR_FORWARD,  false, true, },
                    [EDITH_CMD_KIND_DELETE_TO_LINE_LEFT]        = { MOVE_BY_HOME_END,   DIR_BACKWARD, false, true, },
                    [EDITH_CMD_KIND_DELETE_TO_LINE_RIGHT]       = { MOVE_BY_HOME_END,   DIR_FORWARD,  false, true, },
                };

                assert_bounds(cmd->kind, countof(movement_table));
                movement m = movement_table[cmd->kind];
                if (m.by) {
                    assert(m.dir);
                    if (app->active_pane && app->active_pane->tab) {
                        edith_textview *editor = &app->active_pane->tab->v;
                        if (m.delete) {
                            assert(m.select == false);
                            edith_textview_submit_delete(editor, m.by, m.dir);
                        } else {
                            edith_textview_move(editor, m.by, m.dir, m.select);
                        }
                    }
                }
            } break;

            default: {
                assert(!"Unknown command kind.");
            } break;
        }
    }

    YO_PROFILE_END(edith_app_handle_command);

    return true;
}
#pragma warning ( pop )

static void edith_app_consume_commands(edith_app *app) {
    while (1) {
        edith_cmd cmd2 = { 0 };
        i64 peeked_size = edith_cmd_ring_peek(&edith_global_cmd_ring, &cmd2, 0, edith_temp());
        if (peeked_size == 0) {
            break;
        }

        print("from cmd queue %\n", edith_cmd_spec_from_kind(cmd2.kind)->name);
        bool handled = edith_app_handle_command(app, &cmd2);
        if (handled) {
            edith_cmd_ring_skip(&edith_global_cmd_ring, peeked_size);
        } else {
            break;
        }
    }
}

static ui_ops edith_temp_get_ui_ops(void) {
    ui_ctx *ctx = global_ui_ctx;

    buf op_data_buf = ctx->op_data_buf;
    buf op_type_buf = ctx->op_def_buf;

    ui_ops ops = { 0 };
    ops.defs       = (ui_op_def *)op_type_buf.v;
    ops.def_count  = (u32)(op_type_buf.len / sizeof(ui_op_def));
    ops.data       = op_data_buf.v;
    ops.data_size  = op_data_buf.count;

    return ops;
}

static void edith_debug_print_ui_ops(ui_ops ops) {
    println(ANSI_FG_GRAY "======================================" ANSI_RESET);

    for_narray (ui_op_def, it, ops.defs, ops.def_count) {
        println("% \t data_offset = % ", ui_op_type_as_cstr(it->type), it->data_offset);
    }
}

static vec2 edith_transform_vec2(vec2 v, vec2 transform) {
    return vec2_add(v, transform);
}

static rect edith_transform_rect(rect v, vec2 transform) {
    return rect_make(vec2_add(v.p0, transform), vec2_add(v.p1, transform));
}

static void edith_app_draw_ui_ops(edith_app *app, ui_ops ops) {
    YO_PROFILE_BEGIN(edith_app_draw_ui_ops);

    ui_stack(rect) scissors = { 0 };
    ui_stack(vec2) transforms = { 0 };

    ui_stack_init(vec2, &transforms, ui_arena(), VEC2_ZERO);
    ui_stack_init(rect, &scissors, ui_arena(), rect_make(VEC2_MIN, VEC2_MAX));

    for_narray (ui_op_def, tag, ops.defs, ops.def_count) {
        assert(tag->data_offset < ops.data_size);
        void *data = ops.data + tag->data_offset;
        switch (tag->type) {
            case UI_OP_TYPE_GLYPH: {
                ui_op_glyph *op = data;
                draw_glyph(op->codepoint, edith_transform_vec2(op->p, transforms.top), op->color, op->face, &app->font_backend);
            } break;

            case UI_OP_TYPE_TEXT: {
                ui_op_text *op = data;
                draw_text(op->s, edith_transform_vec2(op->p, transforms.top), op->color, op->face, &app->font_backend);
            } break;

            case UI_OP_TYPE_AABB: {
                ui_op_aabb *op = data;
                r_rect_instance inst = {
                    .dst_rect   = edith_transform_rect(op->rect, transforms.top),
                    .color      = { op->color, op->color, op->color, op->color }
                };
                draw_rect_instance(inst);
            } break;

            case UI_OP_TYPE_AABB_EX: {
                ui_op_aabb_ex *op = data;
                r_rect_instance inst = {
                    .dst_rect   = edith_transform_rect(op->dst_rect, transforms.top),
                    .tex_rect   = op->tex_rect,
                    .tex_weight = op->tex_weight,
                    .softness   = op->softness,
                    .roundness  = op->corner_radius[0], // @Todo Different radius per. corner.
                    .color[0]   = op->color[0],
                    .color[1]   = op->color[1],
                    .color[2]   = op->color[2],
                    .color[3]   = op->color[3],
                };
                draw_rect_instance(inst);
            } break;

            case UI_OP_TYPE_QUAD: {
            } break;

            case UI_OP_TYPE_SCISSOR: {
                ui_op_scissor *op = data;

#ifdef APP_DOES_SCISSOR_INTERSECT_AND_TRANSFORM_ADD
                rect scissor = transform_rect(op->rect, transforms.top);
                if (!(op->flags & UI_SCISSOR_FLAG_NO_INTERSECT)) {
                    scissor = rect_intersect(scissor, ui_stack_get_prev(rect, &scissors));
                }
#else
                rect scissor = op->rect;
#endif

                ui_stack_set(rect, &scissors, scissor);
                draw_set_scissor(scissors.top);
            } break;

            case UI_OP_TYPE_TRANSFORM: {
                ui_op_transform *op = data;
#ifdef APP_DOES_SCISSOR_INTERSECT_AND_TRANSFORM_ADD
                vec2 transform = op->vec;
                if (!(op->flags & UI_TRANSFORM_FLAG_NO_RELATIVE)) {
                    transform = transform_vec2(op->vec, transforms.top);
                }
#else
                vec2 transform = op->vec;
#endif
                ui_stack_set(vec2, &transforms, transform);
            } break;

            case UI_OP_TYPE_PUSH_SCISSOR: {
                ui_stack_push(rect, &scissors);
            } break;

            case UI_OP_TYPE_POP_SCISSOR: {
                ui_stack_pop(rect, &scissors);
                draw_set_scissor(scissors.top);
            } break;

            case UI_OP_TYPE_PUSH_TRANSFORM: {
                ui_stack_push(vec2, &transforms);
            } break;

            case UI_OP_TYPE_POP_TRANSFORM: {
                ui_stack_pop(vec2, &transforms);
            } break;

            default: {
                assert(!"Invalid code path.");
            } break;
        }
    }
    YO_PROFILE_END(edith_app_draw_ui_ops);
}

static void edith_app_do_ui_pane(edith_app *app, edith_pane *pane, rect area) {
    ui_events events = ui_get_events();

    bool active = (pane == app->active_pane);
    if (active) {
        ui_cut_and_draw_border(&area, ui_make_border(1, rgb(20, 60, 60), 0), rgb(0, 0, 0));
    } else {
        ui_cut_and_draw_border(&area, ui_make_border(1, rgb(30, 30, 30), 0), rgb(0, 0, 0));
    }

    f32 target_active_t = active ? 0.0f : 1.0f;
    ui_anim_f32(&pane->active_t, target_active_t, 15.0f);

    if (pane->tab) {
        ////////////////////////////////////////////////////////////////
        // rune: Editor

        ui_id id = ui_id(pane);
        ui_set_face(app->editor_face);
        if (app->state == EDITH_APP_STATE_DEFAULT && active) {
            if (app->window_has_focus) {
                ui_set_active_id(id);
            }
            edith_editor_tab_handle_events(id, pane->tab, events, app->temp);
        }

        rect editor_area = area;
        rect status_bar_area = ui_cut_bot(&editor_area, 25.0f);

        // rune: Draw editor
        edith_textview *editor = &pane->tab->v;
        ui_draw_editor(id, &pane->tab->v, editor_area, app->editor_face);

        // rune: Find row/column
        edith_cursor *cursor = edith_textview_cursors_get_primary(editor);
        i64 caret_pos        = cursor->caret;
        i64 row              = edith_textbuf_row_from_pos(&editor->tb, caret_pos);
        i64 col              = edith_textbuf_col_from_pos(&editor->tb, caret_pos);
        i64 sel              = range_len(edith_cursor_range(*cursor));

        // rune: Draw status bar
        ui_draw_rect(status_bar_area, rgb(20, 30, 30));
        rect status_text_area = status_bar_area;
        ui_cut_centered_y(&status_text_area, ui_em(1));
        ui_cut_left(&status_text_area, 5);
        ui_draw_text_r(status_text_area, ui_fmt("line: %      col: %      pos: %      sel: %", row + 1, col + 1, caret_pos, sel), rgb(200, 200, 200));
    } else {

        ////////////////////////////////////////////////////////////////
        // rune: Home screen

        ui_draw_rect(area, rgb(20, 20, 20));
        ui_set_face(ui_make_face(app->sans_font, 20));
        str text = str("Edith v0.1");
        vec2 text_dim = ui_measure_text(text);
        rect text_rect = area;
        ui_cut_centered(&text_rect, text_dim);

        ui_draw_text_r(text_rect, text, rgb(200, 200, 200));
    }

    ui_draw_rect(area, rgba(0, 0, 0, (u8)(pane->active_t.pos * 100.0f)));
}

static void edith_app_do_ui(edith_app *app, rect client_rect) {
#if 0 // ui library demo
    ui_set_face(ui_make_face(app->sans_font, 18));
    ui_demo(client_rect);
#else
    // TODO(rune): How to handle tab switcher special interaction with commands.h style commands?
    ui_events events = ui_get_events();
    for_array (ui_event, e, events) {
        if (ui_event_is_keypress_with_modifiers(e, UI_KEY_TAB, UI_MODIFIER_CTRL)) {
            app->state = EDITH_APP_STATE_TAB_SWITCHER;
        }

        if (ui_event_is_key_release(e, UI_KEY_CTRL) && app->state == EDITH_APP_STATE_TAB_SWITCHER) {
            app->state = EDITH_APP_STATE_DEFAULT;
        }
    }

    rect main_rect = client_rect;
    ui_draw_rect(main_rect, rgb(20, 20, 20));

    ////////////////////////////////////////////////
    // Editor or homescreen.

    rect left_rect  = main_rect;
    rect right_rect = main_rect;

    left_rect  = ui_get_left(&main_rect, rect_dim_x(main_rect) * 0.5f);
    right_rect = ui_get_right(&main_rect, rect_dim_x(main_rect) * 0.5f);

    edith_app_do_ui_pane(app, &app->left_pane, left_rect);
    edith_app_do_ui_pane(app, &app->right_pane, right_rect);

    // rune: File lister.
    ui_set_face(ui_make_face(app->sans_font, 20));
    if (app->state == EDITH_APP_STATE_OPEN_FILE) {
        edith_dialog_open_file(client_rect, ui_id("lister"), &app->dialog_open_file, app->state == EDITH_APP_STATE_OPEN_FILE, events);
    }

    // rune: Tab switcher.
    if (app->state == EDITH_APP_STATE_TAB_SWITCHER) {
        edith_dialog_tabswitcher(ui_id("current_files"), client_rect, app->tabs, app->active_pane->tab);
    }

    // rune: Find dialog.
    if (app->state == EDITH_APP_STATE_FIND) {
        // TODO(rune): Per tab finder.
        edith_dialog_find(client_rect, &app->dialog_find, &edith_app_get_active_editor(app)->v);
    }

    // rune: Find all dialog.
    if (app->state == EDITH_APP_STATE_FIND_ALL) {
        // TODO(rune): Per tab finder.
        //dialog_find_all(client_rect, &app->dialog_find, &app->active_pane->tab->v.finder, app->active_pane->tab->v.search_result_idx, &app->active_pane->tab->v);
    }

    // rune: Goto line dialog.
    if (app->state == EDITH_APP_STATE_GOTO_LINE) {
        // TODO(rune): Per tab goto line.
        edith_dialog_goto_line(client_rect, &app->dialog_goto_line);
    }

    // rune: Command lister.
    if (app->state == EDITH_APP_STATE_COMMAND_PALLETTE) {
        edith_dialog_commands(client_rect, &app->dialog_commands);
    }

    ui_toasts(client_rect);
#endif

#if 0
    ui_draw_textured(rect_make_xy(512, 512), rgb(255, 255, 255), rect_make_xy(1, 1));
#endif

    ////////////////////////////////////////////////
    // Performance HUD

#if 0
    {
        u32 bg = rgba(0, 0, 0, 50);
        rect perf_area = ui_get_bot(&client_rect, 150);
        ui_draw_rect(perf_area, bg);

        ui_set_layout(ui_make_layout_y(perf_area, UI_ALIGN_LEFT, 0));

        ui_ops ops = temp_get_ui_ops();
        ui_set_face(ui_make_face(app->mono_font, 12));
        ui_text_b(ui_fmt("FRAME    : avg % ms : min % ms : max % ms", app->frame_timing.avg, app->frame_timing.min, app->frame_timing.max));
        ui_text_b(ui_fmt("UI_BUILD : avg % ms : min % ms : max % ms", app->build_ui_timing.avg, app->build_ui_timing.min, app->build_ui_timing.max));
        ui_text_b(ui_fmt("UI_OPS   : avg % ms : min % ms : max % ms", app->draw_ui_ops_timing.avg, app->draw_ui_ops_timing.min, app->draw_ui_ops_timing.max));
        ui_text_b(ui_fmt("rect instance count : %", app->last_frame_rect_instance_count));
        ui_text_b(ui_fmt("ui_op_def count     : %", ops.def_count));
        ui_text_b(ui_fmt("ui_op_def size      : %", ops.def_count * sizeof(ui_op_def)));
        ui_text_b(ui_fmt("ui_op data size     : %", ops.data_size));

        //if (app->active_pane->tab) {
            //ui_text_b(ui_fmt("caret : %    mark : %     doclen : %     # cursors : %", editor_get_primary_cursor(&app->tabs.active->v)->caret, editor_get_primary_cursor(&app->tabs.active->v)->mark, editor_get_len(&app->tabs.active->v), app->tabs.active->v.cursors.count));
        //}
    }
#endif

#if 0
    ui_draw_text_f(v2(0, 0), input.clientrect, rgb(255, 255, 255), "update %f ms     ui %f ms", app->perf.last_update_ms, app->perf.last_ui_ms);
#endif
}

static void test_textview_init_text(edith_textview *editor, str initial_text); // @cleanup

static edith_app_output edith_app_update_and_render(edith_app *app, edith_app_input input) {
    YO_PROFILE_FRAME_MARK();
    YO_PROFILE_BEGIN(edith_app_update_and_render);

    edith_perf_timing_begin(&app->frame_timing);

    //
    // Startup.
    //

    if (!app->initialized) {
        app->initialized = true;

        app->perm = edith_arena_create_default(str("app/perm"));
        app->temp = edith_arena_create_default(str("app/temp"));

        edith_thread_local_arena = app->temp;

        atlas_create(&app->atlas, ivec2(512, 512), 4096);
        app->ui_toast_ctx.storage = (ui_toast_array) { app->ui_toast_storage, countof(app->ui_toast_storage) };


        font_backend_stb_startup(&app->font_backend, &app->atlas);

        global_mono_font = app->mono_font = font_backend_stb_init_font(&app->font_backend, liberation_mono_data, sizeof(liberation_mono_data));
        global_sans_font = app->sans_font = font_backend_stb_init_font(&app->font_backend, opensans_stripped_regular_data, sizeof(opensans_stripped_regular_data));

        app->editor_face = ui_make_face(app->mono_font, 10);

        app->ui_ctx.invalidate_next_frame = true;
        app->ui_ctx.font_backend.userdata = &app->font_backend;
        app->ui_ctx.font_backend.get_advance = font_backend_stb_get_advance;
        app->ui_ctx.font_backend.get_lineheight = font_backend_stb_get_lineheight;
    }

    arena_scope_begin(app->temp);

    ////////////////////////////////////////////////////////////////
    // rune: Setup globals.

    global_deltatime = input.deltatime;
    global_tick = input.tick;

    global_ui_toast_ctx = &app->ui_toast_ctx;
    ui_select_ctx(&app->ui_ctx);
    draw_select_ctx(&app->draw_ctx);

    ////////////////////////////////////////////////////////////////
    // rune: Update and draw UI.

    bool need_redraw = false;
    r_pass_list pass_list = { 0 };
    darray_reset(&app->ui_rect_storage);

    app->window_has_focus = input.window_has_focus;

    draw_begin();

    ui_input ui_input = { 0 };
    ui_input.events = input.events;
    ui_input.mouse_pos = input.mouse_pos;
    ui_input.mouse_buttons = input.mouse_buttons;
    ui_input.client_rect = input.client_rect;
    draw_begin_rect_pass(input.client_rect, &app->ui_rect_storage, &app->atlas);
    edith_perf_timing_begin(&app->build_ui_timing);
    {
        if (global_ui_ctx->invalidate_next_frame || input.events.count > 0) {
            // DEBUG
            //println(ANSI_FG_GRAY "======================================" ANSI_RESET);

            ui_next_frame(&ui_input);
            edith_app_gather_commands(app);
            edith_app_do_ui(app, input.client_rect);
            ui_end_frame();
            need_redraw = true;
        }
    }
    edith_perf_timing_end(&app->build_ui_timing);
    edith_perf_timing_begin(&app->draw_ui_ops_timing);
#if 0
    debug_print_ui_ops(temp_get_ui_ops()); // DEBUG
#endif
    edith_app_draw_ui_ops(app, edith_temp_get_ui_ops());
    edith_perf_timing_end(&app->draw_ui_ops_timing);

    draw_end_rect_pass();
    draw_submit_current_pass();

    pass_list = draw_end();

    app->last_frame_rect_instance_count = pass_list.last->type_rects.instances.count;

    ////////////////////////////////////////////////////////////////
    // rune: Process ui commands

    edith_app_consume_commands(app);

    ////////////////////////////////////////////////////////////////
    // rune: Finish frame

    edith_app_output ret = { 0 };
    ret.render_passes = pass_list;
    ret.need_redraw = need_redraw;

    //doc_debug_print_state_(&app->editor.doc);

    edith_perf_timing_end(&app->frame_timing);
#if 0
    // DEBUG
    print(ANSI_HOME);
    print(ANSI_ERASE_SCREEN);
    for_val (landmark, it, app->editor.doc.landmarks) {
        println("pos = % \t linenumber = %", it.pos, it.linenumber);
        if (it_idx > 20) {
            break;
        }
    }

    println("caret = %", app->editor.active_cursors[0].caret);
    println("doc_iter_count = %", global_doc_iter_counter);
    println("caret linenumber = %", get_linenumber_from_pos(&app->editor.doc, app->editor.active_cursors[0].caret));
#endif

    edith_global_doc_iter_counter = 0;

    arena_scope_end(app->temp);

    ui_invalidate_next_frame();

    //edith_mem_track_print();
    //edith_mem_track_dump_csv(str("C:\\temp\\edith_mem.csv"));


#ifdef TRACK_ALLOCATIONS
    for_array (ui_event, e, input.events) {
        if (ui_event_is_keypress(e, UI_KEY_K)) {
            print(ANSI_ERASE_SCREEN);
            print(ANSI_HOME);
            idk_print_tracked_allocations(true, true);
        }
    }
#endif

    edith_global_frame_counter += 1;

    YO_PROFILE_END(edith_app_update_and_render);
    return ret;
        }
