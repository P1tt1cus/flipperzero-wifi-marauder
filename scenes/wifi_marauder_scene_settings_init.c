#include "../wifi_marauder_app_i.h"

static void settings_changed(VariableItem* item) {
    WifiMarauderApp* app = variable_item_get_context(item);
    uint8_t row = variable_item_list_get_selected_item_index(app->var_item_list);
    bool enabled = variable_item_get_current_value_index(item) != 0;
    bool previous = row == 0 ? app->ok_to_save_logs : app->ok_to_save_pcaps;
    File* file = row == 0 ? app->save_logs_setting_file : app->save_pcap_setting_file;
    const char* path = row == 0 ? SAVE_LOGS_SETTING_FILEPATH : SAVE_PCAP_SETTING_FILEPATH;
    bool saved = storage_file_open(file, path, FSAM_WRITE, FSOM_CREATE_ALWAYS);
    if(saved) saved = storage_file_write(file, enabled ? "Y" : "N", 1) == 1;
    storage_file_close(file);
    if(!saved) {
        enabled = previous;
        variable_item_set_current_value_index(item, enabled);
        view_dispatcher_send_custom_event(app->view_dispatcher, 300);
    }
    if(row == 0) app->ok_to_save_logs = enabled;
    else app->ok_to_save_pcaps = enabled;
    variable_item_set_current_value_text(item, enabled ? "On" : "Off");
}

static void settings_select(void* context, uint32_t index) {
    WifiMarauderApp* app = context;
    if(index == 2) view_dispatcher_send_custom_event(app->view_dispatcher, 301);
}

void wifi_marauder_scene_settings_init_on_enter(void* context) {
    WifiMarauderApp* app = context;
    variable_item_list_set_enter_callback(app->var_item_list, settings_select, app);
    const char* labels[] = {"Save logs", "Save captures"};
    for(uint8_t row = 0; row < 2; row++) {
        bool enabled = row == 0 ? app->ok_to_save_logs : app->ok_to_save_pcaps;
        VariableItem* item = variable_item_list_add(app->var_item_list, labels[row], 2, settings_changed, app);
        variable_item_set_current_value_index(item, enabled);
        variable_item_set_current_value_text(item, enabled ? "On" : "Off");
    }
    variable_item_list_add(app->var_item_list, "Board and controls", 0, NULL, app);
    app->need_to_prompt_settings_init = false;
    view_dispatcher_switch_to_view(app->view_dispatcher, WifiMarauderAppViewVarItemList);
}

bool wifi_marauder_scene_settings_init_on_event(void* context, SceneManagerEvent event) {
    WifiMarauderApp* app = context;
    if(event.type != SceneManagerEventTypeCustom) return false;
    if(event.event == 300) {
        dialog_message_show_storage_error(app->dialogs, "Cannot save settings");
    } else if(event.event == 301) {
        DialogMessage* message = dialog_message_alloc();
        dialog_message_set_header(message, "Your LDDB board", 64, 1, AlignCenter, AlignTop);
        dialog_message_set_text(message, "GPS is not built in.\nConsole: OK hold/follow\nArrows scroll; Back exits.\nCheck link in Device.", 2, 17, AlignLeft, AlignTop);
        dialog_message_set_buttons(message, NULL, "Back", NULL);
        dialog_message_show(app->dialogs, message);
        dialog_message_free(message);
    }
    return true;
}

void wifi_marauder_scene_settings_init_on_exit(void* context) {
    WifiMarauderApp* app = context;
    variable_item_list_reset(app->var_item_list);
}
