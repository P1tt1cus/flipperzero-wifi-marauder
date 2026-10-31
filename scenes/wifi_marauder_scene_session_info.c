#include "../wifi_marauder_app_i.h"

static void session_info_back(GuiButtonType button, InputType type, void* context) {
    UNUSED(button);
    WifiMarauderApp* app = context;
    if(type == InputTypeShort) view_dispatcher_send_custom_event(app->view_dispatcher, 301);
}

void wifi_marauder_scene_session_info_on_enter(void* context) {
    WifiMarauderApp* app = context;
    const char* title = "Files on Flipper SD";
    const char* text = "apps_data/marauder/\nlogs/  pcaps/  dumps/\nExport with qFlipper.\nESP32 SD is separate.";
    char details[144];
    if(app->session_info_kind == 1) {
        title = "No console output";
        text = "Run a command first.\nSaved logs are in\nBrowse logs.";
    } else if(app->session_info_kind == 2) {
        FileInfo info;
        if(storage_common_stat(app->storage, app->session_path, &info) == FSE_OK) {
            const char* name = strrchr(app->session_path, '/');
            title = info.size ? "Capture file" : "Empty capture";
            snprintf(details, sizeof(details), "%.22s\n%llu bytes\nExport with qFlipper.\nOpen in Wireshark.",
                     name ? name + 1 : app->session_path, (unsigned long long)info.size);
            text = details;
        } else {
            title = "Cannot read capture";
            text = "Check the SD card,\nthen reopen the file.";
        }
    }
    widget_reset(app->widget);
    widget_add_string_element(app->widget, 2, 9, AlignLeft, AlignBottom, FontPrimary, title);
    widget_add_text_scroll_element(app->widget, 0, 13, 128, 39, text);
    widget_add_button_element(app->widget, GuiButtonTypeCenter, "Back", session_info_back, app);
    view_dispatcher_switch_to_view(app->view_dispatcher, WifiMarauderAppViewWidget);
}

bool wifi_marauder_scene_session_info_on_event(void* context, SceneManagerEvent event) {
    WifiMarauderApp* app = context;
    if(event.type != SceneManagerEventTypeCustom || event.event != 301) return false;
    scene_manager_previous_scene(app->scene_manager);
    return true;
}

void wifi_marauder_scene_session_info_on_exit(void* context) {
    WifiMarauderApp* app = context;
    widget_reset(app->widget);
}
