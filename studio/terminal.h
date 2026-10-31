#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define STUDIO_COLUMNS 20
#define STUDIO_LINES 160
#define STUDIO_ROWS 4

/* Bounded terminal history. Absolute line numbers keep scrolling stable as data arrives. */
typedef struct {
    char lines[STUDIO_LINES][STUDIO_COLUMNS + 1];
    uint32_t first;
    uint32_t last;
    uint8_t column;
    uint8_t escape;
} StudioTerminal;

void studio_terminal_reset(StudioTerminal* terminal);
void studio_terminal_feed(StudioTerminal* terminal, const uint8_t* data, size_t size);
uint32_t studio_terminal_tail(const StudioTerminal* terminal);
void studio_terminal_page(const StudioTerminal* terminal, uint32_t top, char rows[STUDIO_ROWS][STUDIO_COLUMNS + 1]);
