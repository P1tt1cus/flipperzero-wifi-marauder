"""Host regression tests for fragmented UART text and bounded scroll history."""
import pathlib
import re
import subprocess
import tempfile

ROOT = pathlib.Path(__file__).resolve().parents[1]
TEST = r'''
#include "terminal.h"
#include "link_report.h"
#include "command_status.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static bool response(StudioCommandStatus* s, const char* text, uint32_t now) {
    return studio_command_feed(s, (const uint8_t*)text, strlen(text), now);
}
static void test_command_status(void) {
    StudioCommandStatus s;
    studio_command_begin(&s, "protocolinfo --machine studio", false, false, 100);
    assert(!response(&s, "#protocolinfo --machine studio\r\n> ", 200));
    assert(!response(&s, "@MARAUDER:{invalid}\n", 300));
    assert(studio_command_state(&s, 10099) == StudioCommandWaiting);
    assert(studio_command_state(&s, 10100) == StudioCommandTimeout);
    assert(studio_command_can_retry(&s));
    studio_command_reply(&s, 10200, true); /* validated late link reply */
    assert(studio_command_state(&s, 500000) == StudioCommandDone);

    studio_command_begin(&s, "list -b", false, false, 0);
    assert(!response(&s, "> #list -b\r\n>", 100));
    assert(!s.complete);
    assert(!response(&s, " ", 101));
    assert(studio_command_state(&s, 50000) == StudioCommandEmpty);
    assert(!s.replied); /* bare echo/prompt never refreshes last reply */

    studio_command_begin(&s, "list -b", false, false, 0);
    const char* reply = "#list -b\r\n\033[32m[0] test\033[0m\r\n> ";
    bool got_reply = false;
    for(const char* c = reply; *c; c++) got_reply |= studio_command_feed(&s, (const uint8_t*)c, 1, 200);
    assert(got_reply && s.echo && s.last_reply == 200);
    assert(studio_command_state(&s, 50000) == StudioCommandDone);

    studio_command_begin(&s, "diagnostics", false, false, 0);
    assert(!response(&s, "#help\n> ", 200));
    assert(studio_command_state(&s, 10000) == StudioCommandTimeout);
    assert(response(&s, "chip: ESP32\n", 12000));
    assert(studio_command_state(&s, 12001) == StudioCommandReply);
    assert(studio_command_state(&s, 22000) == StudioCommandTimeout);

    studio_command_begin(&s, "sniffbeacon", true, false, 0);
    assert(!response(&s, "#sniffbeacon -serial\n> ", 100));
    assert(studio_command_state(&s, 14999) == StudioCommandWaiting);
    assert(studio_command_state(&s, 15000) == StudioCommandTimeout);
    assert(!studio_command_can_retry(&s));
    assert(response(&s, "Starting beacon sniffer\n> ", 16000));
    assert(studio_command_state(&s, 16000) == StudioCommandReply);
    assert(studio_command_state(&s, 46000) == StudioCommandQuiet);
    studio_command_reply(&s, 47000, false); /* capture-only traffic */
    assert(studio_command_state(&s, 47000) == StudioCommandReply);

    studio_command_begin(&s, "pingscan", true, false, 0);
    assert(studio_command_state(&s, 59999) == StudioCommandWaiting);
    assert(studio_command_state(&s, 60000) == StudioCommandTimeout);
    assert(response(&s, "Starting scan\n> ", 61000));
    assert(studio_command_state(&s, 121000) == StudioCommandQuiet);
    assert(!s.complete); /* firmware has no UART completion event for this scan */

    const char* mutations[] = {"reboot", "settings -s SavePCAP enable", "clearlist -a", "join -s", "evilportal -c sethtmlstr", "info; reboot", "list -b\nreboot"};
    for(unsigned i = 0; i < sizeof(mutations)/sizeof(*mutations); i++) {
        studio_command_begin(&s, mutations[i], false, false, 0);
        assert(!studio_command_can_retry(&s));
        assert(studio_command_state(&s, 29999) == StudioCommandWaiting);
        assert(studio_command_state(&s, 30000) == StudioCommandTimeout);
    }
    studio_command_begin(&s, "", false, true, 0);
    assert(studio_command_state(&s, 999999) != StudioCommandTimeout);
    assert(!studio_command_can_retry(&s));
    studio_command_begin(&s, NULL, false, false, 0);
    assert(!response(&s, "old saved output\n", 300));
    assert(studio_command_state(&s, 999999) != StudioCommandTimeout);

    studio_command_begin(&s, "help", false, false, UINT32_MAX - 999);
    assert(studio_command_state(&s, 8999) == StudioCommandWaiting);
    assert(studio_command_state(&s, 9000) == StudioCommandTimeout);
    char long_line[2048];
    memset(long_line, 'x', sizeof(long_line));
    studio_command_feed(&s, (const uint8_t*)long_line, sizeof(long_line), 10000);
    assert(response(&s, "\n", 10001));
    assert(response(&s, "normal reply\n", 10002));
    assert(!s.overflow && s.used == 0);
}
static void feed(StudioTerminal* t, const char* s) {
    studio_terminal_feed(t, (const uint8_t*)s, strlen(s));
}
int main(void) {
    test_command_status();
    StudioTerminal t;
    char rows[STUDIO_ROWS][STUDIO_COLUMNS + 1];
    studio_terminal_reset(&t);
    feed(&t, "12345678901234567890\nnext");
    assert(t.last == 1);
    assert(!strcmp(t.lines[0], "12345678901234567890"));
    assert(!strcmp(t.lines[1], "next"));
    studio_terminal_reset(&t);
    feed(&t, "123456789012345678901");
    assert(t.last == 1 && !strcmp(t.lines[1], "1"));
    studio_terminal_reset(&t);
    const char* fragments[] = {"\033", "[3", "1mred", "\033[0m\r", "\nnext", "\033]title", "\033", "\\!"};
    for(unsigned i = 0; i < sizeof(fragments)/sizeof(*fragments); i++) feed(&t, fragments[i]);
    assert(!strcmp(t.lines[0], "red"));
    assert(!strcmp(t.lines[1], "next!"));
    feed(&t, "\b?");
    assert(!strcmp(t.lines[1], "next?"));
    studio_terminal_reset(&t);
    for(unsigned i = 0; i < 10000; i++) {
        char s[24];
        snprintf(s, sizeof(s), "%u\n", i);
        feed(&t, s);
        assert(t.last - t.first < STUDIO_LINES);
    }
    assert(t.first == 9841 && t.last == 10000);
    studio_terminal_page(&t, studio_terminal_tail(&t), rows);
    assert(!strcmp(rows[0], "9997"));
    assert(!strcmp(rows[2], "9999"));
    assert(!strcmp(rows[3], ""));
    studio_terminal_page(&t, 0, rows);
    assert(!strcmp(rows[0], "9841"));
    studio_terminal_reset(&t);
    uint8_t bytes[256];
    for(unsigned i = 0; i < 256; i++) bytes[i] = i;
    for(unsigned i = 0; i < 1000; i++) studio_terminal_feed(&t, bytes, sizeof(bytes));
    for(unsigned i = 0; i < STUDIO_LINES; i++) assert(memchr(t.lines[i], 0, sizeof(t.lines[i])));
    char report[128];
    const char* response = "#protocolinfo --machine studio\n@MARAUDER:{\"protocol\":1,\"tx\":\"studio\",\"command\":\"protocolinfo\",\"status\":\"success\",\"firmware\":\"v1.20.0\"}\n>";
    assert(studio_link_report(response, report, sizeof(report)));
    assert(strstr(report, "FW v1.20.0"));
    char delayed[1024];
    snprintf(delayed, sizeof(delayed), "@MARAUDER:{invalid}\n%s", response);
    assert(studio_link_report(delayed, report, sizeof(report)));
    assert(!studio_link_report("@MARAUDER:{\"tx\":\"studio\"}", report, sizeof(report)));
    assert(!studio_link_report("@MARAUDER:{\"tx\":\"studio\"}\n", report, sizeof(report)));
    assert(!studio_link_report("@MARAUDER:{invalid}\n", report, sizeof(report)));
    assert(!studio_link_report("@MARAUDER:[[[[[[]]]]]]\n", report, sizeof(report)));
    assert(!studio_link_report("#protocolinfo\n>", report, sizeof(report)));
    bool diagnostics = false;
    assert(studio_link_report_with_features("@MARAUDER:{\"protocol\":1,\"tx\":\"studio\",\"command\":\"protocolinfo\",\"status\":\"success\",\"firmware\":\"v1.20.0\",\"profile\":\"studio-zy-esp32\",\"capabilities\":[\"wifi-2.4\",\"ble\",\"serial-pcap\",\"diagnostics\"]}\n", report, sizeof(report), &diagnostics));
    assert(diagnostics && strstr(report, "Profile confirmed"));
    diagnostics = true;
    assert(studio_link_report_with_features("@MARAUDER:{\"protocol\":1,\"tx\":\"studio\",\"command\":\"protocolinfo\",\"status\":\"success\",\"firmware\":\"v1.20.0\",\"profile\":\"studio-zy-esp32\",\"capabilities\":[]}\n", report, sizeof(report), &diagnostics));
    assert(!diagnostics);
    diagnostics = true;
    assert(studio_link_report_with_features("@MARAUDER:{\"protocol\":1,\"tx\":\"studio\",\"command\":\"protocolinfo\",\"status\":\"success\",\"firmware\":\"v1.20.0\",\"profile\":\"studio-zy-esp32\",\"capabilities\":[\"diagnostics\",12]}\n", report, sizeof(report), &diagnostics));
    assert(!diagnostics);
    diagnostics = true;
    assert(studio_link_report_with_features("@MARAUDER:{\"protocol\":1,\"tx\":\"studio\",\"command\":\"protocolinfo\",\"status\":\"success\",\"firmware\":\"v1.20.0\",\"profile\":\"studio-zy-esp32\",\"capabilities\":null}\n", report, sizeof(report), &diagnostics));
    assert(!diagnostics);
    diagnostics = true;
    assert(studio_link_report_with_features("@MARAUDER:{\"protocol\":1,\"tx\":\"studio\",\"command\":\"protocolinfo\",\"status\":\"success\",\"firmware\":\"v1.20.0\",\"profile\":\"another-board\",\"capabilities\":[\"wifi-2.4\",\"ble\",\"serial-pcap\",\"diagnostics\"]}\n", report, sizeof(report), &diagnostics));
    assert(!diagnostics);
    assert(studio_link_report_with_features(response, report, sizeof(report), &diagnostics));
    assert(!diagnostics);
    assert(!studio_link_report_with_features("@MARAUDER:{\"protocol\":2,\"tx\":\"studio\",\"command\":\"protocolinfo\",\"status\":\"success\",\"firmware\":\"v1.20.0\",\"profile\":\"studio-zy-esp32\",\"capabilities\":[\"wifi-2.4\",\"ble\",\"serial-pcap\",\"diagnostics\"]}\n", report, sizeof(report), &diagnostics));
    assert(!studio_link_report_with_features("@MARAUDER:{\"protocol\":1.5,\"tx\":\"studio\",\"command\":\"protocolinfo\",\"status\":\"success\",\"firmware\":\"v1.20.0\",\"profile\":\"studio-zy-esp32\",\"capabilities\":[\"wifi-2.4\",\"ble\",\"serial-pcap\",\"diagnostics\"]}\n", report, sizeof(report), &diagnostics));
    assert(!studio_link_report_with_features("@MARAUDER:{\"protocol\":1,\"tx\":\"stale\",\"command\":\"protocolinfo\",\"status\":\"success\",\"firmware\":\"v1.20.0\",\"profile\":\"studio-zy-esp32\",\"capabilities\":[\"wifi-2.4\",\"ble\",\"serial-pcap\",\"diagnostics\"]}\n", report, sizeof(report), &diagnostics));
    assert(!studio_link_report_with_features("@MARAUDER:{\"protocol\":1,\"tx\":\"studio\",\"command\":\"protocolinfo\",\"status\":\"error\",\"firmware\":\"v1.20.0\",\"profile\":\"studio-zy-esp32\",\"capabilities\":[\"wifi-2.4\",\"ble\",\"serial-pcap\",\"diagnostics\"]}\n", report, sizeof(report), &diagnostics));
    const char* unsupported[] = {"gps", "gpsdata", "nmea", "wardrive", "wardrivepoi", "gpspoi", "geofence", "upload", "led", "ls", "save", "load", "backupspiffs", "backupstatus", "restorespiffs", "update"};
    for(unsigned i = 0; i < NUM_MENU_ITEMS; i++) {
        assert(items[i].num_options_menu > 0 && items[i].num_options_menu <= MAX_OPTIONS);
        for(int j = 0; j < items[i].num_options_menu; j++) {
            const char* cmd = items[i].actual_commands[j];
            assert(cmd && items[i].options_menu[j]);
            for(unsigned k = 0; k < sizeof(unsupported)/sizeof(*unsupported); k++) {
                size_t n = strlen(unsupported[k]);
                assert(strncmp(cmd, unsupported[k], n) || (cmd[n] != 0 && cmd[n] != ' '));
            }
            assert(!strstr(cmd, "EnableLED"));
            assert(strcmp(cmd, "evilportal -c sethtml"));
        }
    }
    assert(!strcmp(items[NUM_MENU_ITEMS - 2].item_string, "Scripts"));
    assert(!strcmp(items[NUM_MENU_ITEMS - 1].item_string, "Save to flipper sdcard"));
    const uint8_t* groups[] = {discover_items, tools_items, device_items};
    size_t lengths[] = {sizeof(discover_items), sizeof(tools_items), sizeof(device_items)};
    for(unsigned g = 0; g < 3; g++) for(size_t j = 0; j < lengths[g]; j++) {
        assert(groups[g][j] < NUM_MENU_ITEMS - 2);
        for(size_t k = 0; k < j; k++) assert(groups[g][k] != groups[g][j]);
    }
    assert(!strcmp(items[device_items[0]].actual_commands[0], "protocolinfo --machine studio"));
    assert(!strcmp(items[device_items[1]].actual_commands[0], "diagnostics"));
    puts("PASS: command deadlines, retry policy, quiet scans, late replies, link response validation, wrapping, fragmented ANSI/OSC, CRLF, backspace, ring eviction, binary noise");
}
'''
# Compile the actual menu data to validate indexes and hardware-command omissions.
menu_source = (ROOT / 'scenes/wifi_marauder_scene_start.c').read_text()
menu_data = menu_source[menu_source.index('typedef enum { NO_ARGS'):menu_source.index('static bool wifi_marauder_command_stops_on_exit')]
menu_count = re.search(r'#define NUM_MENU_ITEMS \((\d+)\)', (ROOT / 'wifi_marauder_app_i.h').read_text())[1]
TEST = TEST.replace('int main(void)', '#define NUM_MENU_ITEMS '+menu_count+'\n'+menu_data+'\nint main(void)')
with tempfile.TemporaryDirectory() as temporary:
    source = pathlib.Path(temporary) / 'terminal_test.c'
    binary = pathlib.Path(temporary) / 'terminal_test'
    source.write_text(TEST)
    subprocess.run(['cc', '-std=c11', '-Wall', '-Wextra', '-Werror', '-Wno-deprecated-declarations', '-fsanitize=address,undefined',
                    '-I', str(ROOT / 'studio'), str(source), str(ROOT / 'studio/terminal.c'), str(ROOT / 'studio/command_status.c'), str(ROOT / 'studio/link_report.c'), str(ROOT / 'script/cJSON.c'),
                    '-o', str(binary), '-lm'], check=True)
    subprocess.run([str(binary)], check=True)
