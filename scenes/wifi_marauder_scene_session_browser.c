#include "../wifi_marauder_app_i.h"

#define SESSION_FILE_SELECTED 300

static void session_file_selected(void* context) {
    WifiMarauderApp* app = context;
    view_dispatcher_send_custom_event(app->view_dispatcher, SESSION_FILE_SELECTED);
}

void wifi_marauder_scene_session_browser_on_enter(void* context) {
    WifiMarauderApp* app = context;
    const char* dirs[] = {MARAUDER_APP_FOLDER_LOGS, MARAUDER_APP_FOLDER_PCAPS, MARAUDER_APP_FOLDER_DUMPS};
    const char* extensions[] = {".log", ".pcap", ".txt"};
    furi_check(app->session_browser_kind < COUNT_OF(dirs));
    file_browser_configure(app->session_browser, extensions[app->session_browser_kind],
                           dirs[app->session_browser_kind], true, true, &I_Text_10x10, false);
    file_browser_set_callback(app->session_browser, session_file_selected, app);
    file_browser_start(app->session_browser, app->session_browser_path);
    app->session_browser_active = true;
    /* Keep the app's event loop running while the browser worker reads SD. */
    view_dispatcher_switch_to_view(app->view_dispatcher, WifiMarauderAppViewSessionBrowser);
}

bool wifi_marauder_scene_session_browser_on_event(void* context, SceneManagerEvent event) {
    WifiMarauderApp* app = context;
    if(event.type != SceneManagerEventTypeCustom || event.event != SESSION_FILE_SELECTED) return false;
    strlcpy(app->session_path, furi_string_get_cstr(app->session_browser_path), sizeof(app->session_path));
    if(app->session_browser_kind == 1) {
        app->session_info_kind = 2;
        scene_manager_next_scene(app->scene_manager, WifiMarauderSceneSessionInfo);
    } else {
        strlcpy(app->log_file_path, app->session_path, sizeof(app->log_file_path));
        app->browse_selected_log = true;
        if(app->session_browser_from_log) {
            app->open_log_file_page = 1;
            scene_manager_search_and_switch_to_previous_scene(app->scene_manager, WifiMarauderSceneLogViewer);
        } else {
            scene_manager_next_scene(app->scene_manager, WifiMarauderSceneLogViewer);
        }
    }
    return true;
}

void wifi_marauder_scene_session_browser_on_exit(void* context) {
    WifiMarauderApp* app = context;
    if(app->session_browser_active) {
        file_browser_stop(app->session_browser);
        app->session_browser_active = false;
    }
}
