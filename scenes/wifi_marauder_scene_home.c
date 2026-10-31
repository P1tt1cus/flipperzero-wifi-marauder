#include "../wifi_marauder_app_i.h"

void wifi_marauder_scene_home_on_enter(void* context) {
    WifiMarauderApp* app = context;
    wifi_marauder_show_reply_age(app);
    studio_view_home(app->studio, app->home_index, app->ok_to_save_logs, app->ok_to_save_pcaps);
    view_dispatcher_switch_to_view(app->view_dispatcher, WifiMarauderAppViewConsoleOutput);
}

bool wifi_marauder_scene_home_on_event(void* context, SceneManagerEvent event) {
    WifiMarauderApp* app = context;
    if(event.type == SceneManagerEventTypeTick) {
        wifi_marauder_show_reply_age(app);
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
    if(event.type == SceneManagerEventTypeCustom && event.event >= 100 && event.event < 106) {
        app->launch_check_started = true;
        app->home_index = event.event - 100;
        switch(app->home_index) {
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

void wifi_marauder_scene_home_on_exit(void* context) { UNUSED(context); }
