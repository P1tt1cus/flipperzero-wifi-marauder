#include "../wifi_marauder_app_i.h"

static void more_select(void* context, uint32_t index) {
    WifiMarauderApp* app = context;
    view_dispatcher_send_custom_event(app->view_dispatcher, index);
}

void wifi_marauder_scene_more_on_enter(void* context) {
    WifiMarauderApp* app = context;
    static const EchoDashboardItem items[] = {
        {"Tools", "Additional commands", 0, EchoDashTools, false},
        {"Scripts", "Edit and run scripts", 1, EchoDashFiles, false},
        {"Device", "Info and diagnostics", 2, EchoDashLink, false},
    };
    echo_dashboard_configure(
        app->dashboard,
        "Radio",
        "More",
        EchoDashTools,
        items,
        COUNT_OF(items),
        scene_manager_get_scene_state(app->scene_manager, WifiMarauderSceneMore),
        more_select,
        app);
    view_dispatcher_switch_to_view(app->view_dispatcher, WifiMarauderAppViewDashboard);
}

bool wifi_marauder_scene_more_on_event(void* context, SceneManagerEvent event) {
    WifiMarauderApp* app = context;
    if(event.type != SceneManagerEventTypeCustom || event.event > 2) return false;
    scene_manager_set_scene_state(app->scene_manager, WifiMarauderSceneMore, event.event);
    if(event.event == 1) {
        app->selected_tx_string = "";
        app->is_command = false;
        app->is_custom_tx_string = false;
        app->stop_on_exit = false;
        app->focus_console_start = false;
        scene_manager_next_scene(app->scene_manager, WifiMarauderSceneScriptSelect);
    } else {
        app->menu_group = event.event == 0 ? 1 : 2;
        scene_manager_next_scene(app->scene_manager, WifiMarauderSceneStart);
    }
    return true;
}

void wifi_marauder_scene_more_on_exit(void* context) {
    UNUSED(context);
}
