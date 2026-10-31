//** Includes sniffbt and sniffskim for compatible ESP32-WROOM hardware.
// wifi_marauder_scene_start.c also changed **//
#pragma once

#include "wifi_marauder_app.h"
#include "scenes/wifi_marauder_scene.h"
#include "wifi_marauder_custom_event.h"
#include "wifi_marauder_uart.h"
#include "wifi_marauder_ep.h"
#include "file/sequential_file.h"
#include "script/wifi_marauder_script.h"
#include "script/wifi_marauder_script_worker.h"
#include "script/wifi_marauder_script_executor.h"
#include "script/menu/wifi_marauder_script_stage_menu.h"

#include <stdatomic.h>
#include <gui/gui.h>
#include "studio/studio_view.h"
#include <gui/view_dispatcher.h>
#include <gui/modules/file_browser.h>
#include <gui/scene_manager.h>
#include <gui/modules/text_box.h>
#include <gui/modules/submenu.h>
#include <gui/modules/variable_item_list.h>
#include <gui/modules/widget.h>
#include "wifi_marauder_text_input.h"

#include <marauder_studio_icons.h>
#include <storage/storage.h>
#include <lib/toolbox/path.h>
#include <dialogs/dialogs.h>

#define NUM_MENU_ITEMS (33)

#define WIFI_MARAUDER_TEXT_BOX_STORE_SIZE (4096)
#define WIFI_MARAUDER_TEXT_INPUT_STORE_SIZE (512)

#define MARAUDER_APP_FOLDER_USER "apps_data/marauder"
#define MARAUDER_APP_FOLDER EXT_PATH(MARAUDER_APP_FOLDER_USER)
#define MARAUDER_APP_FOLDER_HTML MARAUDER_APP_FOLDER "/html"
#define MARAUDER_APP_FOLDER_PCAPS MARAUDER_APP_FOLDER "/pcaps"
#define MARAUDER_APP_FOLDER_DUMPS MARAUDER_APP_FOLDER "/dumps"
#define MARAUDER_APP_FOLDER_LOGS MARAUDER_APP_FOLDER "/logs"
#define MARAUDER_APP_FOLDER_USER_PCAPS MARAUDER_APP_FOLDER_USER "/pcaps"
#define MARAUDER_APP_FOLDER_USER_LOGS MARAUDER_APP_FOLDER_USER "/logs"
#define MARAUDER_APP_FOLDER_SCRIPTS MARAUDER_APP_FOLDER "/scripts"
#define MARAUDER_APP_SCRIPT_PATH(file_name) MARAUDER_APP_FOLDER_SCRIPTS "/" file_name ".json"
#define SAVE_PCAP_SETTING_FILEPATH MARAUDER_APP_FOLDER "/save_pcaps_here.setting"
#define SAVE_LOGS_SETTING_FILEPATH MARAUDER_APP_FOLDER "/save_logs_here.setting"

typedef enum WifiMarauderUserInputType {
    WifiMarauderUserInputTypeString,
    WifiMarauderUserInputTypeNumber,
    WifiMarauderUserInputTypeFileName
} WifiMarauderUserInputType;

struct WifiMarauderApp {
    Gui* gui;
    ViewDispatcher* view_dispatcher;
    SceneManager* scene_manager;

    char text_input_store[WIFI_MARAUDER_TEXT_INPUT_STORE_SIZE + 1];
    FuriString* text_box_store;
    FuriString* file_view_store;
    size_t text_box_store_strlen;
    StudioView* studio;
    uint8_t home_index;
    uint8_t menu_group;
    uint8_t menu_map[NUM_MENU_ITEMS];
    uint8_t menu_count;
    uint8_t menu_positions[3];
    uint32_t console_started;
    uint32_t launch_started;
    bool launch_check_started;
    bool launch_check;
    atomic_bool board_reply_seen;
    atomic_uint_least32_t board_reply_at;
    atomic_bool storage_error;
    bool link_summary_shown;
    atomic_bool board_diagnostics;
    atomic_bool board_radio;
    bool browse_selected_log;
    bool log_open_failed;
    FileBrowser* session_browser;
    FuriString* session_browser_path;
    uint8_t session_browser_kind; /* logs, captures, text dumps */
    uint8_t session_info_kind; /* file locations, no output, capture details */
    bool session_browser_active;
    bool session_browser_from_log;
    char session_path[256];
    WIFI_TextInput* text_input;
    Storage* storage;
    File* capture_file;
    File* log_file;
    char log_file_path[256];
    File* save_pcap_setting_file;
    File* save_logs_setting_file;
    bool need_to_prompt_settings_init;
    int which_prompt;
    bool ok_to_save_pcaps;
    bool ok_to_save_logs;
    bool has_saved_logs_this_session;
    DialogsApp* dialogs;

    VariableItemList* var_item_list;
    Widget* widget;
    Submenu* submenu;
    int open_log_file_page;
    int open_log_file_num_pages;

    WifiMarauderUart* uart;
    int selected_menu_index;
    int selected_option_index[NUM_MENU_ITEMS];
    const char* selected_tx_string;
    bool is_command;
    bool is_custom_tx_string;
    bool focus_console_start;
    bool show_stopscan_tip;
    bool stop_on_exit;
    atomic_bool is_writing_pcap;
    atomic_bool is_writing_log;

    // User input
    WifiMarauderUserInputType user_input_type;
    char** user_input_string_reference;
    int* user_input_number_reference;
    char* user_input_file_dir;
    char* user_input_file_extension;

    // Automation script
    WifiMarauderScript* script;
    WifiMarauderScriptWorker* script_worker;
    FuriString** script_list;
    int script_list_count;
    WifiMarauderScriptStage* script_edit_selected_stage;
    WifiMarauderScriptStageMenu* script_stage_menu;
    WifiMarauderScriptStageListItem* script_stage_edit_first_item;
    char*** script_stage_edit_strings_reference;
    int* script_stage_edit_string_count_reference;
    int** script_stage_edit_numbers_reference;
    int* script_stage_edit_number_count_reference;

    // For input source and destination MAC in targeted deauth attack
    int special_case_input_step;
    char special_case_input_src_addr[20];
    char special_case_input_dst_addr[20];
};

// Supported commands:
// https://github.com/justcallmekoko/ESP32Marauder/wiki/cli
//   Scan
//    -> If list is empty, then start a new scanap. (Tap any button to stop.)
//    -> If there's a list, provide option to rescan and dump list of targets to select.
//    -> Press BACK to go back to top-level.
//   Attack
//    -> Beacon
//    -> Deauth
//    -> Probe
//    -> Rickroll
//   Sniff
//    -> Beacon
//    -> Deauth
//    -> ESP
//    -> PMKID
//    -> Pwnagotchi
//   Channel
//   Update
//   Reboot

typedef enum {
    WifiMarauderAppViewVarItemList,
    WifiMarauderAppViewConsoleOutput,
    WifiMarauderAppViewTextInput,
    WifiMarauderAppViewWidget,
    WifiMarauderAppViewSubmenu,
    WifiMarauderAppViewSessionBrowser,
} WifiMarauderAppView;

static inline uint32_t wifi_marauder_now_ms(void) {
    return (uint32_t)((uint64_t)furi_get_tick() * 1000 / furi_kernel_get_tick_frequency());
}

static inline void wifi_marauder_note_reply(WifiMarauderApp* app) {
    atomic_store(&app->board_reply_at, wifi_marauder_now_ms());
    atomic_store(&app->board_reply_seen, true);
}

static inline void wifi_marauder_show_reply_age(WifiMarauderApp* app) {
    bool seen = atomic_load(&app->board_reply_seen);
    studio_view_reply_age(app->studio, seen,
                          atomic_load(&app->board_reply_at));
}
