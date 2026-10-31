#include "../wifi_marauder_app_i.h"

static void sessions_select(void* context, uint32_t index) {
    WifiMarauderApp* app = context;
    view_dispatcher_send_custom_event(app->view_dispatcher, index);
}

void wifi_marauder_scene_sessions_on_enter(void* context) {
    WifiMarauderApp* app = context;
    static const EchoDashboardItem items[] = {
        {"Captures", "Inspect PCAP files", 201, EchoDashRadio, false},
        {"Logs", "Read saved output", 200, EchoDashFiles, false},
        {"Text dumps", "Browse saved text", 202, EchoDashFiles, false},
        {"Last output", "Review console text", 203, EchoDashRead, false},
        {"Export help", "Copy files to your Mac", 204, EchoDashTools, false},
    };
    uint32_t selected =
        scene_manager_get_scene_state(app->scene_manager, WifiMarauderSceneSessions);
    echo_dashboard_configure(
        app->dashboard,
        "Sessions",
        storage_sd_status(app->storage) == FSE_OK ? "SD ready" : "No SD",
        EchoDashFiles,
        items,
        COUNT_OF(items),
        selected ? selected : 201,
        sessions_select,
        app);
    view_dispatcher_switch_to_view(app->view_dispatcher, WifiMarauderAppViewDashboard);
}

bool wifi_marauder_scene_sessions_on_event(void* context, SceneManagerEvent event) {
    WifiMarauderApp* app = context;
    if(event.type != SceneManagerEventTypeCustom || event.event < 200 || event.event > 204)
        return false;
    uint32_t index = event.event - 200;
    scene_manager_set_scene_state(app->scene_manager, WifiMarauderSceneSessions, event.event);
    if(index == 3) {
        if(furi_string_empty(app->text_box_store)) {
            app->session_info_kind = 1;
            scene_manager_next_scene(app->scene_manager, WifiMarauderSceneSessionInfo);
        } else {
            app->browse_selected_log = false;
            scene_manager_next_scene(app->scene_manager, WifiMarauderSceneLogViewer);
        }
        return true;
    }
    if(index == 4) {
        app->session_info_kind = 0;
        scene_manager_next_scene(app->scene_manager, WifiMarauderSceneSessionInfo);
        return true;
    }
    const char* dirs[] = {
        MARAUDER_APP_FOLDER_LOGS, MARAUDER_APP_FOLDER_PCAPS, MARAUDER_APP_FOLDER_DUMPS};
    app->session_browser_kind = index;
    app->session_browser_from_log = false;
    furi_string_set(app->session_browser_path, dirs[index]);
    scene_manager_next_scene(app->scene_manager, WifiMarauderSceneSessionBrowser);
    return true;
}

void wifi_marauder_scene_sessions_on_exit(void* context) {
    WifiMarauderApp* app = context;
    submenu_reset(app->submenu);
}
