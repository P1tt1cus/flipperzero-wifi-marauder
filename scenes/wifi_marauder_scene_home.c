#include "../wifi_marauder_app_i.h"

static void home_select(void* context, uint32_t index) {
    WifiMarauderApp* app = context;
    view_dispatcher_send_custom_event(app->view_dispatcher, index);
}

static void home_status(WifiMarauderApp* app) {
    char status[9] = "No reply";
    if(atomic_load(&app->board_reply_seen)) {
        uint32_t age = (wifi_marauder_now_ms() - atomic_load(&app->board_reply_at)) / 1000;
        if(age > 9999)
            snprintf(status, sizeof(status), "R >9999s");
        else
            snprintf(status, sizeof(status), "R %lus", (unsigned long)age);
    }
    echo_dashboard_status(app->dashboard, status);
}

void wifi_marauder_scene_home_on_enter(void* context) {
    WifiMarauderApp* app = context;
    wifi_marauder_show_reply_age(app);
    static const EchoDashboardItem items[] = {
        {"Discover", "Nearby devices", 100, EchoDashRadio, false},
        {"Captures & logs", "Browse saved sessions", 101, EchoDashFiles, false},
        {"Check link", "Verify a board reply", 106, EchoDashLink, false},
        {"Settings", "Recording preferences", 105, EchoDashSettings, false},
        {"More tools", "Scripts and board", 107, EchoDashTools, false},
    };
    echo_dashboard_configure(
        app->dashboard,
        "Radio",
        "No reply",
        EchoDashRadio,
        items,
        COUNT_OF(items),
        100 + app->home_index,
        home_select,
        app);
    home_status(app);
    view_dispatcher_switch_to_view(app->view_dispatcher, WifiMarauderAppViewDashboard);
}

bool wifi_marauder_scene_home_on_event(void* context, SceneManagerEvent event) {
    WifiMarauderApp* app = context;
    if(event.type == SceneManagerEventTypeTick) {
        wifi_marauder_show_reply_age(app);
        home_status(app);
        /* Let the board boot after GPIO power-up; navigation can skip this check. */
        if(!app->launch_check_started && wifi_marauder_now_ms() - app->launch_started >= 1500) {
            app->launch_check_started = true;
            app->launch_check = true;
            app->selected_tx_string = "protocolinfo --machine studio";
            app->is_command = true;
            app->is_custom_tx_string = false;
            app->stop_on_exit = false;
            app->focus_console_start = true;
            scene_manager_next_scene(app->scene_manager, WifiMarauderSceneConsoleOutput);
        }
        return true;
    }
    if(event.type == SceneManagerEventTypeCustom && event.event >= 100 && event.event < 108) {
        app->launch_check_started = true;
        app->home_index = event.event - 100;
        switch(app->home_index) {
        case 6:
            app->launch_check = false;
            app->selected_tx_string = "protocolinfo --machine studio";
            app->is_command = true;
            app->is_custom_tx_string = false;
            app->stop_on_exit = false;
            app->focus_console_start = true;
            scene_manager_next_scene(app->scene_manager, WifiMarauderSceneConsoleOutput);
            break;
        case 7:
            scene_manager_next_scene(app->scene_manager, WifiMarauderSceneMore);
            break;
        case 0:
        case 2:
        case 4:
            app->menu_group = app->home_index / 2;
            scene_manager_next_scene(app->scene_manager, WifiMarauderSceneStart);
            break;
        case 1:
            scene_manager_next_scene(app->scene_manager, WifiMarauderSceneSessions);
            break;
        case 3:
            app->selected_tx_string = "";
            app->is_command = false;
            app->is_custom_tx_string = false;
            app->stop_on_exit = false;
            app->focus_console_start = false;
            scene_manager_next_scene(app->scene_manager, WifiMarauderSceneScriptSelect);
            break;
        case 5:
            scene_manager_next_scene(app->scene_manager, WifiMarauderSceneSettingsInit);
            break;
        }
        return true;
    }
    return false;
}

void wifi_marauder_scene_home_on_exit(void* context) {
    UNUSED(context);
}
