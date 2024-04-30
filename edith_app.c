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
    if (app->yo_state->frame_counter == 1) {
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

    typedef struct edith_mapped_key edith_mapped_key;
    struct edith_mapped_key {
        yo_key key;
        yo_modifiers mods;
        edith_cmd_kind cmd_kind;
    };

    static readonly edith_mapped_key always_keymap[] = {
        { YO_KEY_O,             YO_MODIFIER_CTRL,                       EDITH_CMD_KIND_SHOW_OPEN_FILE_DIALOG,            },
        { YO_KEY_F,             YO_MODIFIER_CTRL,                       EDITH_CMD_KIND_SHOW_FIND_DIALOG,                 },
        { YO_KEY_F,             YO_MODIFIER_CTRL|YO_MODIFIER_SHIFT,     EDITH_CMD_KIND_SHOW_FIND_ALL_DIALOG,             },
        { YO_KEY_G,             YO_MODIFIER_CTRL,                       EDITH_CMD_KIND_SHOW_GOTO_LINE_DIALOG,            },
        { YO_KEY_P,             YO_MODIFIER_CTRL|YO_MODIFIER_SHIFT,     EDITH_CMD_KIND_SHOW_COMMAND_DIALOG,              },
        { YO_KEY_ESCAPE,        0,                                      EDITH_CMD_KIND_CLOSE_DIALOG,                     },
    };

    static readonly edith_mapped_key editor_keymap[] = {
        // rune: Move
        { YO_KEY_LEFT,          0,                                      EDITH_CMD_KIND_MOVE_LEFT                        },
        { YO_KEY_RIGHT,         0,                                      EDITH_CMD_KIND_MOVE_RIGHT                       },
        { YO_KEY_UP,            0,                                      EDITH_CMD_KIND_MOVE_UP                          },
        { YO_KEY_DOWN,          0,                                      EDITH_CMD_KIND_MOVE_DOWN                        },

        { YO_KEY_LEFT,          YO_MODIFIER_CTRL,                       EDITH_CMD_KIND_MOVE_BY_WORD_LEFT                },
        { YO_KEY_RIGHT,         YO_MODIFIER_CTRL,                       EDITH_CMD_KIND_MOVE_BY_WORD_RIGHT               },
        { YO_KEY_UP,            YO_MODIFIER_CTRL,                       EDITH_CMD_KIND_MOVE_BY_PARAGRAPH_UP             },
        { YO_KEY_DOWN,          YO_MODIFIER_CTRL,                       EDITH_CMD_KIND_MOVE_BY_PARAGRAPH_DOWN           },
        { YO_KEY_HOME,          0,                                      EDITH_CMD_KIND_MOVE_TO_LINE_LEFT                },
        { YO_KEY_END,           0,                                      EDITH_CMD_KIND_MOVE_TO_LINE_RIGHT               },
        { YO_KEY_PAGE_UP,       0,                                      EDITH_CMD_KIND_MOVE_BY_PAGE_UP                  },
        { YO_KEY_PAGE_DOWN,     0,                                      EDITH_CMD_KIND_MOVE_BY_PAGE_DOWN                },
        { YO_KEY_HOME,          YO_MODIFIER_CTRL,                       EDITH_CMD_KIND_MOVE_TO_DOCUMENT_START           },
        { YO_KEY_END,           YO_MODIFIER_CTRL,                       EDITH_CMD_KIND_MOVE_TO_DOCUMENT_END             },

        // rune: Select
        { YO_KEY_LEFT,          YO_MODIFIER_SHIFT,                      EDITH_CMD_KIND_SELECT_LEFT                      },
        { YO_KEY_RIGHT,         YO_MODIFIER_SHIFT,                      EDITH_CMD_KIND_SELECT_RIGHT                     },
        { YO_KEY_UP,            YO_MODIFIER_SHIFT,                      EDITH_CMD_KIND_SELECT_UP                        },
        { YO_KEY_DOWN,          YO_MODIFIER_SHIFT,                      EDITH_CMD_KIND_SELECT_DOWN                      },

        { YO_KEY_LEFT,          YO_MODIFIER_SHIFT|YO_MODIFIER_CTRL,     EDITH_CMD_KIND_SELECT_BY_WORD_LEFT              },
        { YO_KEY_RIGHT,         YO_MODIFIER_SHIFT|YO_MODIFIER_CTRL,     EDITH_CMD_KIND_SELECT_BY_WORD_RIGHT             },
        { YO_KEY_UP,            YO_MODIFIER_SHIFT|YO_MODIFIER_CTRL,     EDITH_CMD_KIND_SELECT_BY_PARAGRAPH_UP           },
        { YO_KEY_DOWN,          YO_MODIFIER_SHIFT|YO_MODIFIER_CTRL,     EDITH_CMD_KIND_SELECT_BY_PARAGRAPH_DOWN         },
        { YO_KEY_HOME,          YO_MODIFIER_SHIFT,                      EDITH_CMD_KIND_SELECT_TO_LINE_LEFT              },
        { YO_KEY_END,           YO_MODIFIER_SHIFT,                      EDITH_CMD_KIND_SELECT_TO_LINE_RIGHT             },
        { YO_KEY_PAGE_UP,       YO_MODIFIER_SHIFT,                      EDITH_CMD_KIND_SELECT_BY_PAGE_UP                },
        { YO_KEY_PAGE_DOWN,     YO_MODIFIER_SHIFT,                      EDITH_CMD_KIND_SELECT_BY_PAGE_DOWN              },
        { YO_KEY_HOME,          YO_MODIFIER_SHIFT|YO_MODIFIER_CTRL,     EDITH_CMD_KIND_SELECT_TO_DOCUMENT_START         },
        { YO_KEY_END,           YO_MODIFIER_SHIFT|YO_MODIFIER_CTRL,     EDITH_CMD_KIND_SELECT_TO_DOCUMENT_END           },

        { YO_KEY_A,             YO_MODIFIER_CTRL,                       EDITH_CMD_KIND_SELECT_ALL                       },

        // rune: Delete
        { YO_KEY_DELETE,        0,                                      EDITH_CMD_KIND_DELETE_LEFT,                     },
        { YO_KEY_BACKSPACE,     0,                                      EDITH_CMD_KIND_DELETE_RIGHT,                    },
        { YO_KEY_DELETE,        YO_MODIFIER_CTRL,                       EDITH_CMD_KIND_DELETE_BY_WORD_LEFT,             },
        { YO_KEY_BACKSPACE,     YO_MODIFIER_CTRL,                       EDITH_CMD_KIND_DELETE_BY_WORD_RIGHT,            },

        // rune: Undo/redo
        { YO_KEY_Z,             YO_MODIFIER_CTRL,                       EDITH_CMD_KIND_UNDO,                            },
        { YO_KEY_Y,             YO_MODIFIER_CTRL,                       EDITH_CMD_KIND_REDO,                            },

        // rune: Cursors.
        { YO_KEY_UP,            YO_MODIFIER_SHIFT|YO_MODIFIER_ALT,      EDITH_CMD_KIND_ADD_CURSOR_ABOVE,                },
        { YO_KEY_DOWN,          YO_MODIFIER_SHIFT|YO_MODIFIER_ALT,      EDITH_CMD_KIND_ADD_CURSOR_BELOW,                },
        { YO_KEY_D,             YO_MODIFIER_CTRL,                       EDITH_CMD_KIND_ADD_CURSOR_AT_NEXT_OCCURENCE,    },
        { YO_KEY_D,             YO_MODIFIER_CTRL|YO_MODIFIER_SHIFT,     EDITH_CMD_KIND_ADD_CURSOR_AT_PREV_OCCURENCE,    },
        { YO_KEY_ESCAPE,        0,                                      EDITH_CMD_KIND_CLEAR_CURSORS,                   },
        { YO_KEY_M,             YO_MODIFIER_CTRL,                       EDITH_CMD_KIND_FLIP_CURSOR,                     },

        // rune: Tab
        { YO_KEY_S,             YO_MODIFIER_CTRL,                       EDITH_CMD_KIND_SAVE_ACTIVE_TAB,                 },
        { YO_KEY_W,             YO_MODIFIER_CTRL,                       EDITH_CMD_KIND_CLOSE_ACTIVE_TAB,                },

        // rune: Pane
        { YO_KEY_K,             YO_MODIFIER_CTRL,                       EDITH_CMD_KIND_NEXT_PANE,                       },
        { YO_KEY_K,             YO_MODIFIER_CTRL|YO_MODIFIER_SHIFT,     EDITH_CMD_KIND_PREV_PANE,                       },

        // rune: Misc move
        { YO_KEY_R,             YO_MODIFIER_CTRL,                       EDITH_CMD_KIND_MOVE_TO_NEXT_OCCURENCE,          },
        { YO_KEY_R,             YO_MODIFIER_CTRL|YO_MODIFIER_SHIFT,     EDITH_CMD_KIND_MOVE_TO_PREV_OCCURENCE,          },

        // rune: Clipboard
        { YO_KEY_X,             YO_MODIFIER_CTRL,                       EDITH_CMD_KIND_CUT,                             },
        { YO_KEY_C,             YO_MODIFIER_CTRL,                       EDITH_CMD_KIND_COPY,                            },
        { YO_KEY_V,             YO_MODIFIER_CTRL,                       EDITH_CMD_KIND_PASTE,                           },
    };

    for_list (yo_event, e, yo_events()) {
        bool eat = false;
        if (e->kind == YO_EVENT_KIND_KEY_PRESS) {
            for_sarray (edith_mapped_key, k, always_keymap) {
                if ((k->key == e->key) && (k->mods == e->mods)) {
                    edith_cmd cmd = { .kind = k->cmd_kind };
                    edith_cmd_ring_write(&edith_g_cmd_ring, &cmd);
                    eat = true;
                }
            }

            if (app->state == EDITH_APP_STATE_DEFAULT) {
                for_sarray (edith_mapped_key, k, editor_keymap) {
                    if ((k->key == e->key) && (k->mods == e->mods)) {
                        edith_cmd cmd = { .kind = k->cmd_kind };
                        edith_cmd_ring_write(&edith_g_cmd_ring, &cmd);
                        eat = true;
                    }
                }
            }
        }

        if (eat) {
            yo_event_eat(e);
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

// TODO(rune): Wrapping.
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
//#pragma warning ( error : 4061) // enumerator in switch of enum is not explicitly handled by a case label.
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
        yo_invalidate_next_frame();

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

#if 0
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
#endif

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
                        //ui_post_toast(0, ui_fmt("Saved file '%'.", tab->file_name));
                    } else {
                        //ui_post_toast(0, ui_fmt("Could not save file '%'.", tab->file_name));
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
        }
    }

    YO_PROFILE_END(edith_app_handle_command);

    return true;
}
#pragma warning ( pop )

static void edith_app_consume_commands(edith_app *app) {
    while (1) {
        edith_cmd cmd2 = { 0 };
        i64 peeked_size = edith_cmd_ring_peek(&edith_g_cmd_ring, &cmd2, 0, edith_temp());
        if (peeked_size == 0) {
            break;
        }

        print("from cmd queue %\n", edith_cmd_spec_from_kind(cmd2.kind)->name);
        bool handled = edith_app_handle_command(app, &cmd2);
        if (handled) {
            edith_cmd_ring_skip(&edith_g_cmd_ring, peeked_size);
        } else {
            break;
        }
    }
}

static void edith_app_do_ui_pane(edith_app *app, edith_pane *pane, rect area) {
    bool active = (pane == app->active_pane);
#if 0
    if (active) {
        ui_cut_and_draw_border(&area, ui_make_border(1, rgb(20, 60, 60), 0), rgb(0, 0, 0));
    } else {
        ui_cut_and_draw_border(&area, ui_make_border(1, rgb(30, 30, 30), 0), rgb(0, 0, 0));
    }
#endif

    f32 target_active_t = active ? 0.0f : 1.0f;
    yo_anim_f32(&pane->active_t, target_active_t, 15.0f);

    if (pane->tab) {
        ////////////////////////////////////////////////////////////////
        // rune: Editor

        yo_id id = yo_id(pane);
        yo_face_set(app->editor_face);
        if (app->state == EDITH_APP_STATE_DEFAULT && active) {
            if (app->window_has_focus) {
                yo_state_get()->active_id = id;
            }
            editor_tab_handle_events(id, pane->tab, app->temp);
        }

        rect editor_area = area;
        rect status_bar_area = yo_rect_cut_bot(&editor_area, 25.0f);

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
        yo_draw_rect(status_bar_area, rgb(20, 30, 30));
        rect status_text_area = status_bar_area;
        yo_rect_cut_centered_y(&status_text_area, yo_em(1));
        yo_rect_cut_left(&status_text_area, 5);
        yo_draw_text(status_text_area.p0, yo_fmt("line: %      col: %      pos: %      sel: %", row + 1, col + 1, caret_pos, sel), rgb(200, 200, 200));
    } else {

        ////////////////////////////////////////////////////////////////
        // rune: Home screen

        yo_draw_rect(area, rgb(20, 20, 20));
        yo_face_set(yo_face_make(app->sans_font, 20));
        str text = str("Edith v0.1");
        vec2 text_dim = yo_dim_from_text(text);
        rect text_rect = area;
        yo_rect_cut_centered(&text_rect, text_dim);

        yo_draw_text(text_rect.p0, text, rgb(200, 200, 200));
    }

    yo_draw_rect(area, rgba(0, 0, 0, (u8)(pane->active_t * 100.0f)));
}

static void edith_app_do_ui(edith_app *app, rect client_rect) {
#if 1
    yo_node(0) {
        yo_node_get()->tag = "main";
        yo_draw_rect(rect_make(vec2(0, 0), vec2(100, 100)), YO_COLOR_GREEN);

        yo_node(0) {
            yo_node_get()->tag = "a";
        }

        yo_node(0) {
            yo_node_get()->tag = "b";
            yo_node_get()->rel_offset = vec2(20, 20);
            yo_node_get()->rel_rect = rect_make(vec2(0, 0), vec2(100, 100));
            yo_node_get()->flags = YO_NODE_FLAG_CLIP;

            yo_draw_rect(rect_make(vec2(0, 0), vec2(100, 100)), YO_COLOR_RED);


            yo_node(0) {
                yo_node_get()->tag = "b/a";
                yo_node_get()->rel_offset = vec2(20, 20);
                yo_draw_rect(rect_make(vec2(0, 0), vec2(100, 100)), YO_COLOR_ORANGE);
            }
            yo_node(0) {
                yo_node_get()->tag = "b/b";
            }
            yo_node(0) {
                yo_node_get()->tag = "b/c";
            }
            yo_node(0) {
                yo_node_get()->tag = "b/d";
            }
        }
        yo_node(0) {
            yo_node_get()->tag = "c";
        }

        yo_node(0) {
            yo_node_get()->tag = "d";
            yo_node(0) {
                yo_node_get()->tag = "b/a";
            }
            yo_node(0) {
                yo_node_get()->tag = "b/b";
            }
            yo_node(0) {
                yo_node_get()->tag = "b/c";
            }
            yo_node(0) {
                yo_node_get()->tag = "b/d";
            }
        }
    }

    return;
#endif

#if 0 // ui library demo
    ui_set_face(ui_make_face(app->sans_font, 18));
    ui_demo(client_rect);
#else
#if 0
    // TODO(rune): How to handle tab switcher special interaction with commands.h style commands?
    for_list (yo_event, e, yo_events()) {
        if (yo_event_is_key_press(e, YO_KEY_TAB, YO_MODIFIER_CTRL)) {
            app->state = EDITH_APP_STATE_TAB_SWITCHER;
        }

        if (yo_event_is_key_release(e, YO_KEY_CTRL) && app->state == EDITH_APP_STATE_TAB_SWITCHER) {
            app->state = EDITH_APP_STATE_DEFAULT;
        }
    }
#endif

    rect main_rect = client_rect;
    yo_draw_rect(main_rect, rgb(20, 20, 20));

    ////////////////////////////////////////////////
    // Editor or homescreen.

    rect left_rect  = main_rect;
    rect right_rect = main_rect;

    left_rect  = yo_rect_get_left(&main_rect, rect_dim_x(main_rect) * 0.5f);
    right_rect = yo_rect_get_right(&main_rect, rect_dim_x(main_rect) * 0.5f);

    edith_app_do_ui_pane(app, &app->left_pane, left_rect);
    edith_app_do_ui_pane(app, &app->right_pane, right_rect);

#if 0
    // rune: File lister.
    yo_face_set(yo_face_make(app->sans_font, 20));
    if (app->state == EDITH_APP_STATE_OPEN_FILE) {
        dialog_open_file(client_rect, ui_id("lister"), &app->dialog_open_file, app->state == EDITH_APP_STATE_OPEN_FILE, events);
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

    ////////////////////////////////////////////////////////////////
    // rune: Startup

    if (!app->initialized) {
        app->initialized = true;

        app->perm = edith_arena_create_default(str("app/perm"));
        app->temp = edith_arena_create_default(str("app/temp"));

        edith_thread_local_arena = app->temp;

        atlas_init(&app->atlas, uvec2(512, 512), 4096, app->perm);

        font_backend_startup(&app->font_backend, &app->atlas);

        g_mono_font = app->mono_font = font_backend_init_font(&app->font_backend, str_from_sarray(liberation_mono_data));
        g_sans_font = app->sans_font = font_backend_init_font(&app->font_backend, str_from_sarray(liberation_mono_data));

        app->editor_face = yo_face_make(app->mono_font, 10);

        app->yo_state = yo_state_create();
        app->yo_state->invalidate_next_frame       = true;
        app->yo_state->font_backend.userdata       = &app->font_backend;
        app->yo_state->font_backend.get_advance    = font_backend_get_advance;
        app->yo_state->font_backend.get_lineheight = font_backend_get_lineheight;
    }

    arena_scope_begin(app->temp);

    ////////////////////////////////////////////////////////////////
    // rune: Setup globals.

    g_deltatime = input.deltatime;
    g_tick      = input.tick;

    yo_state_select(app->yo_state);

    ////////////////////////////////////////////////////////////////
    // rune: Update and draw UI.

    bool need_redraw = false;

    app->window_has_focus = input.window_has_focus;

    yo_input yo_input = { 0 };
    yo_input.events = input.events;
    yo_input.mouse_pos = input.mouse_pos;
    //yo_input.mouse_buttons = input.mouse_buttons;
    yo_input.client_rect = input.client_rect;
    yo_input.delta_time = g_deltatime;
    edith_perf_timing_begin(&app->build_ui_timing);
    {
        if (1) {
            yo_frame_begin(&yo_input);
            edith_app_gather_commands(app);
            edith_app_do_ui(app, input.client_rect);
            yo_frame_end();
            need_redraw = true;
        }
    }
    edith_perf_timing_end(&app->build_ui_timing);
    edith_perf_timing_begin(&app->draw_ui_ops_timing);
#if 0
    debug_print_ui_ops(temp_get_ui_ops()); // DEBUG
#endif
    //edith_app_draw_ui_ops(app, edith_temp_get_ui_ops());
    edith_perf_timing_end(&app->draw_ui_ops_timing);

    ////////////////////////////////////////////////////////////////
    // rune: Process ui commands

    edith_app_consume_commands(app);

    ////////////////////////////////////////////////////////////////
    // rune: Finish frame

    edith_app_output ret = { 0 };
    ret.root = app->yo_state->root;
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

    yo_invalidate_next_frame();

    edith_mem_track_print();
    //edith_mem_track_dump_csv(str("C:\\temp\\edith_mem.csv"));


#ifdef TRACK_ALLOCATIONS
    for_array (ui_event, e, input.events) {
        if (ui_event_is_keypress(e, YO_KEY_K)) {
            print(ANSI_ERASE_SCREEN);
            print(ANSI_HOME);
            idk_print_tracked_allocations(true, true);
        }
    }
#endif

    edith_g_frame_counter += 1;

    YO_PROFILE_END(edith_app_update_and_render);
    return ret;
}
