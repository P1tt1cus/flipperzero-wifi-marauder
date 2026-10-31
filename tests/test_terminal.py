"""Host regression tests for fragmented UART text and bounded scroll history."""
import pathlib
import subprocess
import tempfile

ROOT = pathlib.Path(__file__).resolve().parents[1]
TEST = r'''
#include "terminal.h"
#include "link_report.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static void feed(StudioTerminal* t, const char* s) {
    studio_terminal_feed(t, (const uint8_t*)s, strlen(s));
}
int main(void) {
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
    assert(!studio_link_report("@MARAUDER:{\"tx\":\"studio\"}", report, sizeof(report)));
    assert(!studio_link_report("@MARAUDER:{\"tx\":\"studio\"}\n", report, sizeof(report)));
    assert(!studio_link_report("@MARAUDER:{invalid}\n", report, sizeof(report)));
    assert(!studio_link_report("@MARAUDER:[[[[[[]]]]]]\n", report, sizeof(report)));
    assert(!studio_link_report("#protocolinfo\n>", report, sizeof(report)));
    puts("PASS: link response validation, wrapping, fragmented ANSI/OSC, CRLF, backspace, ring eviction, binary noise");
}
'''
with tempfile.TemporaryDirectory() as temporary:
    source = pathlib.Path(temporary) / 'terminal_test.c'
    binary = pathlib.Path(temporary) / 'terminal_test'
    source.write_text(TEST)
    subprocess.run(['cc', '-std=c11', '-Wall', '-Wextra', '-Werror', '-Wno-deprecated-declarations', '-fsanitize=address,undefined',
                    '-I', str(ROOT / 'studio'), str(source), str(ROOT / 'studio/terminal.c'), str(ROOT / 'studio/link_report.c'), str(ROOT / 'script/cJSON.c'),
                    '-o', str(binary)], check=True)
    subprocess.run([str(binary)], check=True)
