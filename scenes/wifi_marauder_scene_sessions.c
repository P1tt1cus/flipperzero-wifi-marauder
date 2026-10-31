#include "../wifi_marauder_app_i.h"

static void sessions_select(void* context, uint32_t index) {
    WifiMarauderApp* app = context;
    view_dispatcher_send_custom_event(app->view_dispatcher, 200 + index);
}

static void sessions_info(WifiMarauderApp* app, const char* title, const char* text) {
    DialogMessage* message = dialog_message_alloc();
    dialog_message_set_header(message, title, 64, 2, AlignCenter, AlignTop);
    dialog_message_set_text(message, text, 2, 18, AlignLeft, AlignTop);
    dialog_message_set_buttons(message, NULL, "Back", NULL);
    dialog_message_show(app->dialogs, message);
    dialog_message_free(message);
}

void wifi_marauder_scene_sessions_on_enter(void* context) {
    WifiMarauderApp* app = context;
    submenu_set_header(app->submenu, "Sessions / Flipper SD");
    submenu_add_item(app->submenu, "Browse logs", 0, sessions_select, app);
    submenu_add_item(app->submenu, "Browse captures", 1, sessions_select, app);
    submenu_add_item(app->submenu, "Browse text dumps", 2, sessions_select, app);
    submenu_add_item(app->submenu, "Last console output", 3, sessions_select, app);
    submenu_add_item(app->submenu, "Where are my files?", 4, sessions_select, app);
    submenu_set_selected_item(app->submenu, scene_manager_get_scene_state(app->scene_manager, WifiMarauderSceneSessions));
    view_dispatcher_switch_to_view(app->view_dispatcher, WifiMarauderAppViewSubmenu);
}

bool wifi_marauder_scene_sessions_on_event(void* context, SceneManagerEvent event) {
    WifiMarauderApp* app = context;
    if(event.type != SceneManagerEventTypeCustom || event.event < 200 || event.event > 204) return false;
    uint32_t index = event.event - 200;
    scene_manager_set_scene_state(app->scene_manager, WifiMarauderSceneSessions, index);
    if(index == 3) {
        if(furi_string_empty(app->text_box_store)) {
            sessions_info(app, "No console output", "Run a command first.\nSaved logs are in\nBrowse logs.");
        } else {
            app->browse_selected_log = false;
            scene_manager_next_scene(app->scene_manager, WifiMarauderSceneLogViewer);
        }
        return true;
    }
    if(index == 4) {
        sessions_info(app, "Files on Flipper SD", "apps_data/marauder/\nlogs/   pcaps/   dumps/\nExport with qFlipper.\nESP32 SD is separate.");
        return true;
    }
    const char* dirs[] = {MARAUDER_APP_FOLDER_LOGS, MARAUDER_APP_FOLDER_PCAPS, MARAUDER_APP_FOLDER_DUMPS};
    const char* extensions[] = {".log", ".pcap", ".txt"};
    FuriString* path = furi_string_alloc_set_str(dirs[index]);
    DialogsFileBrowserOptions options;
    dialog_file_browser_set_basic_options(&options, extensions[index], &I_Text_10x10);
    options.base_path = dirs[index];
    options.hide_ext = false;
    bool selected = dialog_file_browser_show(app->dialogs, path, path, &options);
    if(selected) {
        strlcpy(app->session_path, furi_string_get_cstr(path), sizeof(app->session_path));
        if(index != 1) {
            strlcpy(app->log_file_path, app->session_path, sizeof(app->log_file_path));
            app->browse_selected_log = true;
            scene_manager_next_scene(app->scene_manager, WifiMarauderSceneLogViewer);
        } else {
            FileInfo info;
            if(storage_common_stat(app->storage, app->session_path, &info) == FSE_OK) {
                const char* name = strrchr(app->session_path, '/');
                char text[128];
                snprintf(text, sizeof(text), "%.22s\n%llu bytes\nExport with qFlipper.\nOpen in Wireshark.", name ? name + 1 : app->session_path, (unsigned long long)info.size);
                sessions_info(app, info.size ? "Capture file" : "Empty capture", text);
            } else {
                dialog_message_show_storage_error(app->dialogs, "Cannot read capture");
            }
        }
    }
    furi_string_free(path);
    return true;
}

void wifi_marauder_scene_sessions_on_exit(void* context) {
    WifiMarauderApp* app = context;
    submenu_reset(app->submenu);
}
