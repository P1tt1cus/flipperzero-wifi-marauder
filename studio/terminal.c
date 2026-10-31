#include "terminal.h"
#include <string.h>

void studio_terminal_reset(StudioTerminal* terminal) {
    memset(terminal, 0, sizeof(*terminal));
}

static void next_line(StudioTerminal* terminal) {
    terminal->last++;
    if(terminal->last - terminal->first >= STUDIO_LINES) terminal->first++;
    terminal->column = 0;
    terminal->lines[terminal->last % STUDIO_LINES][0] = '\0';
}

void studio_terminal_feed(StudioTerminal* terminal, const uint8_t* data, size_t size) {
    for(size_t i = 0; i < size; i++) {
        uint8_t c = data[i];
        /* Strip CSI/OSC sequences even when split across UART reads. */
        if(terminal->escape == 1) {
            terminal->escape = c == '[' ? 2 : (c == ']' ? 3 : 0);
            continue;
        }
        if(terminal->escape == 2) {
            if(c >= 0x40 && c <= 0x7e) terminal->escape = 0;
            continue;
        }
        if(terminal->escape == 3) {
            if(c == 7) terminal->escape = 0;
            else if(c == 27) terminal->escape = 4;
            continue;
        }
        if(terminal->escape == 4) {
            terminal->escape = c == '\\' ? 0 : 3;
            continue;
        }
        if(c == 27) {
            terminal->escape = 1;
        } else if(c == '\r') {
            continue;
        } else if(c == '\n') {
            next_line(terminal);
        } else if(c == '\b') {
            if(terminal->column) terminal->lines[terminal->last % STUDIO_LINES][--terminal->column] = '\0';
        } else if(c == '\t' || (c >= 32 && c < 127)) {
            if(terminal->column == STUDIO_COLUMNS) next_line(terminal);
            char* line = terminal->lines[terminal->last % STUDIO_LINES];
            line[terminal->column++] = c == '\t' ? ' ' : c;
            line[terminal->column] = '\0';
        }
    }
}

uint32_t studio_terminal_tail(const StudioTerminal* terminal) {
    return terminal->last - terminal->first >= STUDIO_ROWS ? terminal->last - STUDIO_ROWS + 1 : terminal->first;
}

void studio_terminal_page(const StudioTerminal* terminal, uint32_t top, char rows[STUDIO_ROWS][STUDIO_COLUMNS + 1]) {
    if(top < terminal->first) top = terminal->first;
    for(size_t i = 0; i < STUDIO_ROWS; i++) {
        if(top + i <= terminal->last) memcpy(rows[i], terminal->lines[(top + i) % STUDIO_LINES], STUDIO_COLUMNS + 1);
        else rows[i][0] = '\0';
    }
}
