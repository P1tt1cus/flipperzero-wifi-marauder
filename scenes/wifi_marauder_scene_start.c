//** Includes sniffbt and sniffskim for compatible ESP32-WROOM hardware.
//wifi_marauder_app_i.h also changed **//
#include "../wifi_marauder_app_i.h"

// For each command, define whether additional arguments are needed
// (enabling text input to fill them out), and whether the console
// text box should focus at the start of the output or the end
typedef enum { NO_ARGS = 0, INPUT_ARGS, TOGGLE_ARGS } InputArgs;

typedef enum { FOCUS_CONSOLE_END = 0, FOCUS_CONSOLE_START, FOCUS_CONSOLE_TOGGLE } FocusConsole;

#define SHOW_STOPSCAN_TIP (true)
#define NO_TIP (false)

#define MAX_OPTIONS (24)
typedef struct {
    const char* item_string;
    const char* options_menu[MAX_OPTIONS];
    int num_options_menu;
    const char* actual_commands[MAX_OPTIONS];
    InputArgs needs_keyboard;
    FocusConsole focus_console;
    bool show_stopscan_tip;
} WifiMarauderItem;

// NUM_MENU_ITEMS defined in wifi_marauder_app_i.h - if you add an entry here, increment it!
const WifiMarauderItem items[NUM_MENU_ITEMS] = {
    {"View Log from", {"start", "end"}, 2, {"", ""}, NO_ARGS, FOCUS_CONSOLE_TOGGLE, NO_TIP},
    {"Scan",
     {"all", "ping", "arp"},
     3,
     {"scanall", "pingscan", "arpscan"},
     NO_ARGS,
     FOCUS_CONSOLE_END,
     SHOW_STOPSCAN_TIP},
    {"CSI Motion",
     {"start", "status", "recalibrate"},
     3,
     {"csimotion start", "csimotion status", "csimotion recalibrate"},
     TOGGLE_ARGS,
     FOCUS_CONSOLE_END,
     NO_TIP},
    {"Recon",
     {"wifi", "ble", "status", "stop"},
     4,
     {"recon wifi", "recon ble", "recon status", "recon stop"},
     NO_ARGS,
     FOCUS_CONSOLE_END,
     SHOW_STOPSCAN_TIP},
    {"Remote ID",
     {"scan", "list", "track", "clear"},
     4,
     {"remoteid scan", "remoteid list", "remoteid track", "remoteid clear"},
     NO_ARGS,
     FOCUS_CONSOLE_END,
     NO_TIP},
    {"SSID",
     {"add rand", "add name", "remove"},
     3,
     {"ssid -a -g", "ssid -a -n", "ssid -r"},
     INPUT_ARGS,
     FOCUS_CONSOLE_START,
     NO_TIP},
    {"List",
     {"ap", "ssid", "station", "airtag", "IPs", "probes", "bluetooth", "flipper", "pineapple", "multissid"},
     10,
     {"list -a",
      "list -s",
      "list -c",
      "list -t",
      "list -i",
      "list -p",
      "list -b",
      "list -f",
      "list -x",
      "list -m"},
     NO_ARGS,
     FOCUS_CONSOLE_START,
     NO_TIP},
    {"Select",
     {"ap", "ap filter", "ssid", "station"},
     4,
     {"select -a", "select -a -f", "select -s", "select -c"},
     INPUT_ARGS,
     FOCUS_CONSOLE_END,
     NO_TIP},
    {"AP Info",
     {""},
     1,
     {"info -a"},
     INPUT_ARGS,
     FOCUS_CONSOLE_END,
     NO_TIP},
    {"Set MAC",
     {"rand ap", "rand sta", "clone ap", "clone sta"},
     4,
     {"randapmac", "randstamac", "cloneapmac -a", "clonestamac -s"},
     TOGGLE_ARGS,
     FOCUS_CONSOLE_END,
     NO_TIP},
    {"Join WiFi",
     {"new", "saved"},
     2,
     {"join -a", "join -s"},
     INPUT_ARGS,
     FOCUS_CONSOLE_END,
     NO_TIP},
    {"Clear List",
     {"ap", "ssid", "station"},
     3,
     {"clearlist -a", "clearlist -s", "clearlist -c"},
     NO_ARGS,
     FOCUS_CONSOLE_END,
     NO_TIP},
    {"Add Target",
     {"ap", "station"},
     2,
     {"add -a -b", "add -c -b"},
     INPUT_ARGS,
     FOCUS_CONSOLE_END,
     NO_TIP},
    {"Attack",
     {"deauth",
      "probe",
      "rickroll",
      "funny",
      "badmsg",
      "sleep",
      "sae flood",
      "csa",
      "quiet",
      "sour apple",
      "apple juice",
      "swiftpair spam",
      "samsung spam",
      "google spam",
      "flipper spam",
      "bt spam all"},
     16,
     {"attack -t deauth",
      "attack -t probe",
      "attack -t rickroll",
      "attack -t funny",
      "attack -t badmsg",
      "attack -t sleep",
      "attack -t sae",
      "attack -t csa",
      "attack -t quiet",
      "blespam -t sourapple",
      "blespam -t applejuice",
      "blespam -t windows",
      "blespam -t samsung",
      "blespam -t google",
      "blespam -t flipper",
      "blespam -t all"},
     NO_ARGS,
     FOCUS_CONSOLE_END,
     SHOW_STOPSCAN_TIP},
    {"Airtag",
     {"spoof", "sound"},
     2,
     {"spoofat -t", "findmy -t"},
     INPUT_ARGS,
     FOCUS_CONSOLE_END,
     NO_TIP},
    {"Evil Portal",
     {"start", "set AP"},
     2,
     {"evilportal -c start", "evilportal -c setap"},
     TOGGLE_ARGS,
     FOCUS_CONSOLE_END,
     SHOW_STOPSCAN_TIP},
    {"HTML from Flipper",
     {""},
     1,
     {"evilportal -c sethtmlstr"},
     NO_ARGS,
     FOCUS_CONSOLE_END,
     NO_TIP},
    {"Targeted Attacks",
     {"deauth",
      "manual",
      "karma",
      "badmsg",
      "sleep"},
     5,
     {"attack -t deauth -c", 
      "attack -t deauth -s",
      "karma -p",
      "attack -t badmsg -c",
      "attack -t sleep -c"},
     INPUT_ARGS,
     FOCUS_CONSOLE_END,
     SHOW_STOPSCAN_TIP},
    {"Beacon Spam",
     {"ap list", "ssid list", "random"},
     3,
     {"attack -t beacon -a", "attack -t beacon -l", "attack -t beacon -r"},
     NO_ARGS,
     FOCUS_CONSOLE_END,
     SHOW_STOPSCAN_TIP},
    {"Port Scan",
     {"all", "ssh", "telnet", "dns", "http", "smtp", "https", "rdp"},
     8,
     {"portscan -a -t",
      "portscan -s ssh",
      "portscan -s telnet",
      "portscan -s dns",
      "portscan -s http",
      "portscan -s smtp",
      "portscan -s https",
      "portscan -s rdp"},
     INPUT_ARGS,
     FOCUS_CONSOLE_END,
     SHOW_STOPSCAN_TIP},
    {"Sniff",
     {"beacon", "deauth", "pmkid", "probe", "pwn", "raw", "bt", "skim", "airtag", "ibeacon", "flipper", "flock", "meta", "mactrack", "packetcount", "pineapple", "multissid", "sae"},
     18,
     {"sniffbeacon",
      "sniffdeauth",
      "sniffpmkid",
      "sniffprobe",
      "sniffpwn",
      "sniffraw",
      "sniffbt",
      "sniffskim",
      "sniffbt -t airtag",
      "sniffbt -t ibeacon",
      "sniffbt -t flipper",
      "sniffbt -t flock",
      "sniffbt -t meta",
      "mactrack",
      "packetcount",
      "sniffpinescan",
      "sniffmultissid",
      "sniffsae"},
     NO_ARGS,
     FOCUS_CONSOLE_END,
     SHOW_STOPSCAN_TIP},
    {"Fox Hunt",
     {"ap", "station", "bluetooth", "findmy", "flipper", "pineapple", "multissid"},
     7,
     {"foxhunt -w",
      "foxhunt -s",
      "foxhunt -b",
      "foxhunt -t",
      "foxhunt -f",
      "foxhunt -p",
      "foxhunt -m"},
     INPUT_ARGS,
     FOCUS_CONSOLE_END,
     SHOW_STOPSCAN_TIP},
    {"Channel",
     {"get", "set"},
     2,
     {"channel", "channel -s"},
     TOGGLE_ARGS,
     FOCUS_CONSOLE_END,
     NO_TIP},
    {"Settings",
     {"display", "restore", "ForcePMKID on", "ForcePMKID off", "ForceProbe on", "ForceProbe off", "SavePCAP on", "SavePCAP off", "EPDeauth on", "EPDeauth off", "Censor on", "Censor off", "other"},
     13,
     {"settings",
      "settings -r",
      "settings -s ForcePMKID enable",
      "settings -s ForcePMKID disable",
      "settings -s ForceProbe enable",
      "settings -s ForceProbe disable",
      "settings -s SavePCAP enable",
      "settings -s SavePCAP disable",
      "settings -s EPDeauth enable",
      "settings -s EPDeauth disable",
      "settings -s Censor enable",
      "settings -s Censor disable",
      "settings -s"},
     TOGGLE_ARGS,
     FOCUS_CONSOLE_START,
     NO_TIP},
    {"Shutdown WiFi", {""}, 1, {"stopscan -f"}, NO_ARGS, FOCUS_CONSOLE_START, NO_TIP},
    {"Check link", {""}, 1, {"protocolinfo --machine studio"}, NO_ARGS, FOCUS_CONSOLE_START, NO_TIP},
    {"Reboot", {""}, 1, {"reboot"}, NO_ARGS, FOCUS_CONSOLE_END, NO_TIP},
    {"Help", {""}, 1, {"help"}, NO_ARGS, FOCUS_CONSOLE_START, SHOW_STOPSCAN_TIP},
    {"Info", {""}, 1, {"info"}, NO_ARGS, FOCUS_CONSOLE_START, NO_TIP},
    {"Diagnostics", {""}, 1, {"diagnostics"}, NO_ARGS, FOCUS_CONSOLE_START, NO_TIP},
    {"Radio profile", {""}, 1, {"radiostatus --machine studio"}, NO_ARGS, FOCUS_CONSOLE_START, NO_TIP},
    {"Scripts", {""}, 1, {""}, NO_ARGS, FOCUS_CONSOLE_END, NO_TIP},
    {"Save to flipper sdcard", // keep as last entry or change logic in callback below
     {""},
     1,
     {""},
     NO_ARGS,
     FOCUS_CONSOLE_START,
     NO_TIP},
};

/* Only commands supported by our headless ZY-ESP32 hardware profile. */
static const uint8_t discover_items[] = {1, 6, 8, 20, 21, 22, 3, 4, 2, 19};
static const uint8_t tools_items[] = {5, 7, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 24};
static const uint8_t device_items[] = {25, 30, 29, 28, 22, 23, 26, 27};

static bool wifi_marauder_command_stops_on_exit(const char* command) {
    if(command == NULL) return false;

    return strcmp(command, "scanall") == 0 || strcmp(command, "pingscan") == 0 ||
           strcmp(command, "arpscan") == 0 || strcmp(command, "recon wifi") == 0 ||
           strcmp(command, "recon ble") == 0 || strcmp(command, "remoteid scan") == 0 ||
           strcmp(command, "remoteid track") == 0 ||
           strncmp(command, "attack ", strlen("attack ")) == 0 ||
           strncmp(command, "blespam ", strlen("blespam ")) == 0 ||
           strncmp(command, "spoofat ", strlen("spoofat ")) == 0 ||
           strcmp(command, "evilportal -c start") == 0 ||
           strncmp(command, "karma ", strlen("karma ")) == 0 ||
           strncmp(command, "sniff", strlen("sniff")) == 0 ||
           strcmp(command, "mactrack") == 0 || strcmp(command, "packetcount") == 0 ||
           strncmp(command, "foxhunt ", strlen("foxhunt ")) == 0 ||
           strncmp(command, "portscan ", strlen("portscan ")) == 0 ||
           strcmp(command, "csimotion start") == 0;
}

static void wifi_marauder_scene_start_var_list_enter_callback(void* context, uint32_t index) {
    furi_assert(context);
    WifiMarauderApp* app = context;

    furi_assert(index < app->menu_count);
    app->menu_positions[app->menu_group] = index;
    index = app->menu_map[index];
    const WifiMarauderItem* item = &items[index];

    const int selected_option_index = app->selected_option_index[index];
    furi_assert(selected_option_index < item->num_options_menu);
    app->selected_tx_string = item->actual_commands[selected_option_index];
    app->is_command = (1 <= index);
    app->is_custom_tx_string = false;
    app->selected_menu_index = index;
    app->focus_console_start = (item->focus_console == FOCUS_CONSOLE_TOGGLE) ?
                                   (selected_option_index == 0) :
                                   item->focus_console;
    app->stop_on_exit = wifi_marauder_command_stops_on_exit(app->selected_tx_string);
    app->show_stopscan_tip = app->stop_on_exit;

    if(!app->is_command && selected_option_index == 0) {
        // View Log from start
        view_dispatcher_send_custom_event(app->view_dispatcher, WifiMarauderEventStartLogViewer);
        return;
    }

    if(app->selected_tx_string &&
       strncmp("sniffpmkid", app->selected_tx_string, strlen("sniffpmkid")) == 0) {
        // sniffpmkid submenu
        view_dispatcher_send_custom_event(
            app->view_dispatcher, WifiMarauderEventStartSniffPmkidOptions);
        return;
    }

    // Select automation script
    if(index == NUM_MENU_ITEMS - 2) {
        view_dispatcher_send_custom_event(
            app->view_dispatcher, WifiMarauderEventStartScriptSelect);
        return;
    }

    if(index == NUM_MENU_ITEMS - 1) {
        // "Save to flipper sdcard" special case - start SettingsInit widget
        view_dispatcher_send_custom_event(
            app->view_dispatcher, WifiMarauderEventStartSettingsInit);
        return;
    }

    bool needs_keyboard = (item->needs_keyboard == TOGGLE_ARGS) ? (selected_option_index != 0) :
                                                                  item->needs_keyboard;
    if(strcmp(app->selected_tx_string, "remoteid track") == 0 ||
       strcmp(app->selected_tx_string, "csimotion start") == 0) {
        needs_keyboard = true;
    } else if(strcmp(app->selected_tx_string, "csimotion status") == 0 ||
              strcmp(app->selected_tx_string, "csimotion recalibrate") == 0 ||
              strcmp(app->selected_tx_string, "join -s") == 0 ||
              strcmp(app->selected_tx_string, "randapmac") == 0 ||
              strcmp(app->selected_tx_string, "randstamac") == 0 ||
              (strncmp(app->selected_tx_string, "settings", strlen("settings")) == 0 &&
               strcmp(app->selected_tx_string, "settings -s") != 0)) {
        needs_keyboard = false;
    }
    if(needs_keyboard) {
        view_dispatcher_send_custom_event(app->view_dispatcher, WifiMarauderEventStartKeyboard);
    } else {
        view_dispatcher_send_custom_event(app->view_dispatcher, WifiMarauderEventStartConsole);
    }
}

static void wifi_marauder_scene_start_var_list_change_callback(VariableItem* item) {
    furi_assert(item);

    WifiMarauderApp* app = variable_item_get_context(item);
    furi_assert(app);

    uint8_t row = variable_item_list_get_selected_item_index(app->var_item_list);
    app->selected_menu_index = app->menu_map[row];
    const WifiMarauderItem* menu_item = &items[app->selected_menu_index];
    uint8_t item_index = variable_item_get_current_value_index(item);
    furi_assert(item_index < menu_item->num_options_menu);
    variable_item_set_current_value_text(item, menu_item->options_menu[item_index]);
    app->selected_option_index[app->selected_menu_index] = item_index;
}

void wifi_marauder_scene_start_on_enter(void* context) {
    WifiMarauderApp* app = context;
    VariableItemList* var_item_list = app->var_item_list;

    variable_item_list_set_enter_callback(
        var_item_list, wifi_marauder_scene_start_var_list_enter_callback, app);

    const uint8_t* group = app->menu_group == 0 ? discover_items :
                           app->menu_group == 1 ? tools_items : device_items;
    app->menu_count = app->menu_group == 0 ? COUNT_OF(discover_items) :
                      app->menu_group == 1 ? COUNT_OF(tools_items) : COUNT_OF(device_items);
    uint8_t group_count = app->menu_count;
    app->menu_count = 0;
    for(uint8_t row = 0; row < group_count; row++) {
        uint8_t i = group[row];
        if(strcmp(items[i].actual_commands[0], "diagnostics") == 0 &&
           !atomic_load(&app->board_diagnostics)) continue;
        if(strcmp(items[i].actual_commands[0], "radiostatus --machine studio") == 0 &&
           !atomic_load(&app->board_radio)) continue;
        app->menu_map[app->menu_count++] = i;
    }
    if(app->menu_positions[app->menu_group] >= app->menu_count)
        app->menu_positions[app->menu_group] = 0;
    for(uint8_t row = 0; row < app->menu_count; row++) {
        uint8_t i = app->menu_map[row];
        VariableItem* item = variable_item_list_add(
            var_item_list,
            items[i].item_string,
            items[i].num_options_menu,
            wifi_marauder_scene_start_var_list_change_callback,
            app);
        variable_item_set_current_value_index(item, app->selected_option_index[i]);
        variable_item_set_current_value_text(item, items[i].options_menu[app->selected_option_index[i]]);
    }
    variable_item_list_set_selected_item(var_item_list, app->menu_positions[app->menu_group]);

    view_dispatcher_switch_to_view(app->view_dispatcher, WifiMarauderAppViewVarItemList);


}

bool wifi_marauder_scene_start_on_event(void* context, SceneManagerEvent event) {
    UNUSED(context);
    WifiMarauderApp* app = context;
    bool consumed = false;

    if(event.type == SceneManagerEventTypeCustom) {
        if(event.event == WifiMarauderEventStartKeyboard) {
            scene_manager_set_scene_state(
                app->scene_manager, WifiMarauderSceneStart, app->selected_menu_index);
            scene_manager_next_scene(app->scene_manager, WifiMarauderSceneTextInput);
        } else if(event.event == WifiMarauderEventStartConsole) {
            scene_manager_set_scene_state(
                app->scene_manager, WifiMarauderSceneStart, app->selected_menu_index);
            scene_manager_next_scene(app->scene_manager, WifiMarauderSceneConsoleOutput);
        } else if(event.event == WifiMarauderEventStartSettingsInit) {
            scene_manager_set_scene_state(
                app->scene_manager, WifiMarauderSceneStart, app->selected_menu_index);
            scene_manager_next_scene(app->scene_manager, WifiMarauderSceneSettingsInit);
        } else if(event.event == WifiMarauderEventStartLogViewer) {
            scene_manager_set_scene_state(
                app->scene_manager, WifiMarauderSceneStart, app->selected_menu_index);
            scene_manager_next_scene(app->scene_manager, WifiMarauderSceneLogViewer);
        } else if(event.event == WifiMarauderEventStartScriptSelect) {
            scene_manager_set_scene_state(
                app->scene_manager, WifiMarauderSceneStart, app->selected_menu_index);
            scene_manager_next_scene(app->scene_manager, WifiMarauderSceneScriptSelect);
        } else if(event.event == WifiMarauderEventStartSniffPmkidOptions) {
            scene_manager_set_scene_state(
                app->scene_manager, WifiMarauderSceneStart, app->selected_menu_index);
            scene_manager_next_scene(app->scene_manager, WifiMarauderSceneSniffPmkidOptions);
        }
        consumed = true;
    } else if(event.type == SceneManagerEventTypeTick) {
        uint8_t row = variable_item_list_get_selected_item_index(app->var_item_list);
        app->menu_positions[app->menu_group] = row;
        app->selected_menu_index = app->menu_map[row];
        consumed = true;
    } else if(event.type == SceneManagerEventTypeBack) {
        scene_manager_previous_scene(app->scene_manager);
        consumed = true;
    }

    return consumed;
}

void wifi_marauder_scene_start_on_exit(void* context) {
    WifiMarauderApp* app = context;
    variable_item_list_reset(app->var_item_list);
}
