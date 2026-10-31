#include "../wifi_marauder_app_i.h"

static void log_button(GuiButtonType result, InputType type, void* context) {
    WifiMarauderApp* app = context;
    if(type == InputTypeShort) view_dispatcher_send_custom_event(app->view_dispatcher, result);
}

static void log_page(WifiMarauderApp* app) {
    bool file_open = storage_file_is_open(app->log_file);
    furi_string_reset(app->file_view_store);
    if(file_open) {
        if(!storage_file_seek(app->log_file, (app->open_log_file_page - 1) * WIFI_MARAUDER_TEXT_BOX_STORE_SIZE, true)) {
            furi_string_set(app->file_view_store, "Cannot seek in this file.");
        } else {
            char data[65];
            for(size_t read = 0; read < WIFI_MARAUDER_TEXT_BOX_STORE_SIZE; read += 64) {
                size_t count = storage_file_read(app->log_file, data, 64);
                if(!count) break;
                data[count] = '\0';
                furi_string_cat_str(app->file_view_store, data);
            }
            if(storage_file_get_error(app->log_file) != FSE_OK) {
                furi_string_set(app->file_view_store, "SD read failed.\nReopen the file to retry.");
            }
        }
    } else {
        furi_string_set(app->file_view_store, app->text_box_store);
    }
    if(furi_string_empty(app->file_view_store)) {
        furi_string_set(app->file_view_store, file_open ? "This file is empty.\nNo output was recorded." : "No console output.\nRun a command or browse\na saved log.");
    }
    widget_reset(app->widget);
    char heading[28];
    snprintf(heading, sizeof(heading), "%s %d/%d", file_open ? "Saved output" : "Last output", app->open_log_file_page, app->open_log_file_num_pages);
    widget_add_string_element(app->widget, 2, 9, AlignLeft, AlignBottom, FontPrimary, heading);
    widget_add_text_scroll_element(app->widget, 0, 13, 128, 39, furi_string_get_cstr(app->file_view_store));
    widget_add_button_element(app->widget, GuiButtonTypeCenter, "Browse", log_button, app);
    if(app->open_log_file_page > 1) widget_add_button_element(app->widget, GuiButtonTypeLeft, "Prev", log_button, app);
    if(app->open_log_file_page < app->open_log_file_num_pages) widget_add_button_element(app->widget, GuiButtonTypeRight, "Next", log_button, app);
}

static void open_log(WifiMarauderApp* app) {
    storage_file_close(app->log_file);
    if(storage_file_open(app->log_file, app->log_file_path, FSAM_READ, FSOM_OPEN_EXISTING)) {
        uint64_t size = storage_file_size(app->log_file);
        app->open_log_file_num_pages = (size + WIFI_MARAUDER_TEXT_BOX_STORE_SIZE - 1) / WIFI_MARAUDER_TEXT_BOX_STORE_SIZE;
        if(!app->open_log_file_num_pages) app->open_log_file_num_pages = 1;
    } else {
        dialog_message_show_storage_error(app->dialogs, "Cannot open output file");
    }
    app->open_log_file_page = 1;
}

void wifi_marauder_scene_log_viewer_on_enter(void* context) {
    WifiMarauderApp* app = context;
    app->open_log_file_page = 1;
    app->open_log_file_num_pages = 1;
    if(app->browse_selected_log) open_log(app);
    app->browse_selected_log = false;
    log_page(app);
    view_dispatcher_switch_to_view(app->view_dispatcher, WifiMarauderAppViewWidget);
}

bool wifi_marauder_scene_log_viewer_on_event(void* context, SceneManagerEvent event) {
    WifiMarauderApp* app = context;
    if(event.type != SceneManagerEventTypeCustom) return false;
    if(event.event == GuiButtonTypeCenter) {
        FuriString* path = furi_string_alloc_set_str(MARAUDER_APP_FOLDER_LOGS);
        DialogsFileBrowserOptions options;
        dialog_file_browser_set_basic_options(&options, ".log", &I_Text_10x10);
        options.base_path = MARAUDER_APP_FOLDER_LOGS;
        options.hide_ext = false;
        if(dialog_file_browser_show(app->dialogs, path, path, &options)) {
            strlcpy(app->log_file_path, furi_string_get_cstr(path), sizeof(app->log_file_path));
            open_log(app);
        }
        furi_string_free(path);
    } else if(event.event == GuiButtonTypeRight && app->open_log_file_page < app->open_log_file_num_pages) {
        app->open_log_file_page++;
    } else if(event.event == GuiButtonTypeLeft && app->open_log_file_page > 1) {
        app->open_log_file_page--;
    }
    log_page(app);
    return true;
}

void wifi_marauder_scene_log_viewer_on_exit(void* context) {
    WifiMarauderApp* app = context;
    storage_file_close(app->log_file);
    widget_reset(app->widget);
}
