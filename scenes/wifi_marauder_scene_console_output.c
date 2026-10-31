#include "../wifi_marauder_app_i.h"
#include "../studio/link_report.h"

char* _wifi_marauder_get_prefix_from_cmd(const char* command) {
    int end = strcspn(command, " ");
    char* prefix = (char*)malloc(sizeof(char) * (end + 1));
    strncpy(prefix, command, end);
    prefix[end] = '\0';
    return prefix;
}

bool _wifi_marauder_is_saving_enabled(WifiMarauderApp* app) {
    // If it is a script that contains a sniff function
    if(app->script != NULL) {
        if(app->script->save_pcap == WifiMarauderScriptBooleanFalse) {
            return false;
        }
        if(app->script->save_pcap == WifiMarauderScriptBooleanUndefined) {
            if(!app->ok_to_save_pcaps) {
                return false;
            }
        }
        return wifi_marauder_script_has_stage(app->script, WifiMarauderScriptStageTypeSniffRaw) ||
               wifi_marauder_script_has_stage(
                   app->script, WifiMarauderScriptStageTypeSniffBeacon) ||
               wifi_marauder_script_has_stage(
                   app->script, WifiMarauderScriptStageTypeSniffDeauth) ||
               wifi_marauder_script_has_stage(app->script, WifiMarauderScriptStageTypeSniffEsp) ||
               wifi_marauder_script_has_stage(
                   app->script, WifiMarauderScriptStageTypeSniffPmkid) ||
               wifi_marauder_script_has_stage(app->script, WifiMarauderScriptStageTypeSniffPwn);
    }
    if(!app->ok_to_save_pcaps) {
        return false;
    }
    // If it is a sniff/wardrive/evilportal function
    return app->is_command && app->selected_tx_string &&
           (strncmp("sniff", app->selected_tx_string, strlen("sniff")) == 0 ||
            strncmp("wardrive", app->selected_tx_string, strlen("wardrive")) == 0 ||
            strncmp("evilportal", app->selected_tx_string, strlen("evilportal")) == 0);
}

void wifi_marauder_console_output_handle_rx_data_cb(uint8_t* buf, size_t len, void* context) {
    furi_assert(context);
    WifiMarauderApp* app = context;

    if(app->is_writing_log) {
        app->has_saved_logs_this_session = true;
        if(storage_file_write(app->log_file, buf, len) != len) {
            app->storage_error = true;
            app->is_writing_log = false;
        }
    }

    // If text box store gets too big, then truncate it
    app->text_box_store_strlen += len;
    if(app->text_box_store_strlen >= WIFI_MARAUDER_TEXT_BOX_STORE_SIZE - 1) {
        furi_string_right(app->text_box_store, app->text_box_store_strlen / 2);
        app->text_box_store_strlen = furi_string_size(app->text_box_store) + len;
    }

    // Null-terminate buf and append to text box store
    buf[len] = '\0';
    furi_string_cat_printf(app->text_box_store, "%s", buf);
    if(!app->link_summary_shown && studio_view_feed(app->studio, buf, len))
        wifi_marauder_note_reply(app);
    if(!app->link_summary_shown && app->selected_tx_string &&
       strcmp(app->selected_tx_string, "protocolinfo --machine studio") == 0) {
        char report[512];
        bool diagnostics, radio;
        if(studio_link_report_full(
               furi_string_get_cstr(app->text_box_store), report, sizeof(report), &diagnostics, &radio)) {
            atomic_store(&app->board_diagnostics, diagnostics);
            atomic_store(&app->board_radio, radio);
            studio_view_report(app->studio, report);
            wifi_marauder_note_reply(app);
            studio_view_link_verified(app->studio, wifi_marauder_now_ms());
            app->link_summary_shown = true;
        }
    } else if(!app->link_summary_shown && app->selected_tx_string &&
              strcmp(app->selected_tx_string, "radiostatus --machine studio") == 0) {
        char report[512];
        if(studio_radio_report(furi_string_get_cstr(app->text_box_store), report, sizeof(report))) {
            studio_view_report(app->studio, report);
            wifi_marauder_note_reply(app);
            studio_view_link_verified(app->studio, wifi_marauder_now_ms());
            app->link_summary_shown = true;
        }
    }
}

void wifi_marauder_console_output_handle_rx_packets_cb(uint8_t* buf, size_t len, void* context) {
    furi_assert(context);
    WifiMarauderApp* app = context;

    studio_view_capture(app->studio, len);
    if(len) wifi_marauder_note_reply(app);
    if(app->is_writing_pcap) {
        if(storage_file_write(app->capture_file, buf, len) != len) {
            app->storage_error = true;
            app->is_writing_pcap = false;
        }
    }
}

void wifi_marauder_scene_console_output_on_enter(void* context) {
    WifiMarauderApp* app = context;

    app->storage_error = false;
    app->link_summary_shown = false;
    app->console_started = wifi_marauder_now_ms();
    studio_view_console(app->studio,
        app->script ? app->script->name : (app->selected_tx_string && strcmp(app->selected_tx_string, "protocolinfo --machine studio") == 0 ? "Check link" : (app->selected_tx_string && strcmp(app->selected_tx_string, "radiostatus --machine studio") == 0 ? "Radio profile" : (app->selected_tx_string ? app->selected_tx_string : "Console"))),
        !app->focus_console_start, app->stop_on_exit || app->script != NULL);
    studio_view_command(app->studio, NULL, false, false, app->console_started);
    wifi_marauder_show_reply_age(app);
    if(app->is_command || app->script) {
        furi_string_reset(app->text_box_store);
        app->text_box_store_strlen = 0;
    } else if(!furi_string_empty(app->text_box_store)) {
        studio_view_feed(app->studio, (const uint8_t*)furi_string_get_cstr(app->text_box_store), furi_string_size(app->text_box_store));
    }

    // Set scene state and switch view
    scene_manager_set_scene_state(app->scene_manager, WifiMarauderSceneConsoleOutput, 0);
    view_dispatcher_switch_to_view(app->view_dispatcher, WifiMarauderAppViewConsoleOutput);

    // Get ready to send command
    if((app->is_command && app->selected_tx_string) || app->script) {
        char* prefix_buf = NULL;
        if(app->selected_tx_string && strlen(app->selected_tx_string) > 0) {
            prefix_buf = _wifi_marauder_get_prefix_from_cmd(app->selected_tx_string);
        }
        const char* prefix = prefix_buf ? prefix_buf : // Function name
                                          (app->script ? app->script->name : "session"); // Script name

        // Create files *before* sending command
        // (it takes time to iterate through the directory)
        if(app->ok_to_save_logs) {
            char* resolved_path = sequential_file_resolve_path(
                app->storage, MARAUDER_APP_FOLDER_LOGS, prefix, "log");
            if(resolved_path != NULL) {
                strcpy(app->log_file_path, resolved_path);
                free(resolved_path);
                if(storage_file_open(
                       app->log_file, app->log_file_path, FSAM_WRITE, FSOM_CREATE_ALWAYS)) {
                    app->is_writing_log = true;
                } else {
                    dialog_message_show_storage_error(app->dialogs, "Cannot open log file");
                }
            } else {
                dialog_message_show_storage_error(app->dialogs, "Cannot resolve log path");
            }
        }

        // If it is a sniff/wardrive/evilportal function or script, open the capture file for recording
        if(_wifi_marauder_is_saving_enabled(app)) {
            const char* folder = NULL;
            const char* extension = NULL;
            if(app->script || // Scripts only support sniff functions, but selected_tx_string is empty
               strncmp("sniff", app->selected_tx_string, strlen("sniff")) == 0) {
                folder = MARAUDER_APP_FOLDER_PCAPS;
                extension = "pcap";
            } else {
                folder = MARAUDER_APP_FOLDER_DUMPS;
                extension = "txt";
            }
            if(sequential_file_open(app->storage, app->capture_file, folder, prefix, extension)) {
                app->is_writing_pcap = true;
            } else {
                dialog_message_show_storage_error(app->dialogs, "Cannot open capture file");
            }
        }

        bool send_html = false;
        uint8_t* the_html = NULL;
        size_t html_size = 0;
        if(app->selected_tx_string && strncmp(
                                          "evilportal -c sethtmlstr",
                                          app->selected_tx_string,
                                          strlen("evilportal -c sethtmlstr")) == 0) {
            send_html = wifi_marauder_ep_read_html_file(app, &the_html, &html_size);
        }

        // Start deadlines after file preparation and any storage-error dialogs.
        app->console_started = wifi_marauder_now_ms();
        studio_view_command(app->studio, app->selected_tx_string, app->stop_on_exit,
                            app->script != NULL, app->console_started);
        if(app->selected_tx_string && strcmp(app->selected_tx_string, "protocolinfo --machine studio") == 0)
        {
            atomic_store(&app->board_diagnostics, false);
            atomic_store(&app->board_radio, false);
        }

        // Files and view are ready before the worker is allowed to receive.
        wifi_marauder_uart_set_handle_rx_data_cb(app->uart, wifi_marauder_console_output_handle_rx_data_cb);
        wifi_marauder_uart_set_handle_rx_pcap_cb(app->uart, wifi_marauder_console_output_handle_rx_packets_cb);

        // Send command with newline '\n'
        if(app->selected_tx_string) {
            if(app->script == NULL) {
                wifi_marauder_uart_tx(
                    app->uart,
                    (uint8_t*)(app->selected_tx_string),
                    strlen(app->selected_tx_string));
                if(app->is_writing_pcap) {
                    wifi_marauder_uart_tx(
                        app->uart, (uint8_t*)(" -serial\n"), strlen(" -serial\n"));
                } else {
                    wifi_marauder_uart_tx(app->uart, (uint8_t*)("\n"), 1);
                }
            }
            if(send_html && the_html) {
                wifi_marauder_uart_tx(app->uart, the_html, html_size);
                wifi_marauder_uart_tx(app->uart, (uint8_t*)("\n"), 1);
                free(the_html);
                send_html = false;
            }
        }

        // Run the script if the file with the script has been opened
        if(app->script != NULL) {
            app->script_worker = wifi_marauder_script_worker_alloc(app->uart);
            wifi_marauder_script_worker_start(app->script_worker, app->script);
        }

        if(prefix_buf) {
            free(prefix_buf);
        }
    }
}

bool wifi_marauder_scene_console_output_on_event(void* context, SceneManagerEvent event) {
    WifiMarauderApp* app = context;

    bool consumed = false;

    if(event.type == SceneManagerEventTypeCustom) {
        if(event.event == 106) {
            unsigned action = studio_view_recovery(app->studio);
            if(action) {
                // Join callbacks and close files before resetting any shared state.
                wifi_marauder_scene_console_output_on_exit(app);
                if(action == 2) {
                    app->selected_tx_string = "protocolinfo --machine studio";
                    app->is_command = true;
                    app->is_custom_tx_string = false;
                    app->stop_on_exit = false;
                    app->focus_console_start = true;
                }
                wifi_marauder_scene_console_output_on_enter(app);
            }
        }
        consumed = true;
    } else if(event.type == SceneManagerEventTypeTick) {
        wifi_marauder_show_reply_age(app);
        studio_view_status(app->studio, (wifi_marauder_now_ms() - app->console_started) / 1000,
            app->is_writing_log, app->is_writing_pcap, app->storage_error);
        if(app->launch_check && studio_view_link_complete(app->studio) &&
           wifi_marauder_now_ms() - atomic_load(&app->board_reply_at) >= 800)
            scene_manager_previous_scene(app->scene_manager);
        consumed = true;
    }

    return consumed;
}

void wifi_marauder_scene_console_output_on_exit(void* context) {
    WifiMarauderApp* app = context;
    const bool ran_script = app->script != NULL;
    app->launch_check = false;

    // Stop the worker before stopping any operation its final stage may have started.
    if(app->script_worker) {
        wifi_marauder_script_worker_free(app->script_worker);
        app->script_worker = NULL;
    }

    if(app->stop_on_exit || ran_script) {
        wifi_marauder_uart_tx(app->uart, (uint8_t*)("stopscan\n"), strlen("stopscan\n"));
        furi_delay_ms(50);
    }

    // Unregister rx callback
    wifi_marauder_uart_set_handle_rx_data_cb(app->uart, NULL);
    wifi_marauder_uart_set_handle_rx_pcap_cb(app->uart, NULL);

    app->is_writing_pcap = false;
    if(app->capture_file && storage_file_is_open(app->capture_file)) {
        storage_file_close(app->capture_file);
    }

    app->is_writing_log = false;
    if(app->log_file && storage_file_is_open(app->log_file)) {
        storage_file_close(app->log_file);
    }
}
