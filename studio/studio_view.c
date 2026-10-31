#include "studio_view.h"
#include "terminal.h"
#include "command_status.h"
#include <furi.h>
#include <stdio.h>
#include <string.h>

struct StudioView {
    View* view;
    StudioSelectCallback callback;
    void* context;
};

typedef struct {
    StudioTerminal terminal;
    StudioCommandStatus command;
    uint32_t now;
    uint32_t last_reply;
    bool reply_seen;
    bool timeout_hidden;
    char rows[STUDIO_ROWS][STUDIO_COLUMNS + 1];
    char title[17];
    uint32_t top;
    uint32_t received;
    uint32_t captured;
    uint32_t seconds;
    uint8_t selected;
    bool home;
    bool follow;
    bool held;
    bool stop;
    bool logs;
    bool captures;
    bool error;
} StudioModel;

static const char* home_labels[] = {"Discover", "Sessions", "Tools", "Scripts", "Device", "Settings"};
static uint32_t now_ms(void) {
    return (uint32_t)((uint64_t)furi_get_tick() * 1000 / furi_kernel_get_tick_frequency());
}

static void reply_age(const StudioModel* model, char* text, size_t size) {
    if(model->reply_seen) snprintf(text, size, "Reply %lus ago", (unsigned long)((model->now - model->last_reply) / 1000));
    else snprintf(text, size, "No board reply yet");
}

static bool recovery_available(const StudioModel* model) {
    StudioCommandState state = studio_command_state(&model->command, model->now);
    return state == StudioCommandTimeout || state == StudioCommandQuiet;
}

static void studio_draw(Canvas* canvas, void* context) {
    StudioModel* model = context;
    canvas_clear(canvas);
    canvas_set_font(canvas, FontSecondary);
    if(model->home) {
        canvas_set_font(canvas, FontPrimary);
        canvas_draw_str(canvas, 2, 9, "Marauder Studio");
        canvas_set_font(canvas, FontSecondary);
        for(uint8_t i = 0; i < 6; i++) {
            uint8_t x = (i % 2) * 64;
            uint8_t y = 13 + (i / 2) * 13;
            if(model->selected == i) {
                canvas_draw_rbox(canvas, x, y, 63, 12, 2);
                canvas_set_color(canvas, ColorWhite);
            }
            canvas_draw_str(canvas, x + 4, y + 9, home_labels[i]);
            canvas_set_color(canvas, ColorBlack);
        }
        char status[32];
        reply_age(model, status, sizeof(status));
        canvas_draw_str(canvas, 2, 63, status);
    } else {
        char status[24];
        canvas_draw_box(canvas, 0, 0, 128, 12);
        canvas_set_color(canvas, ColorWhite);
        canvas_draw_str(canvas, 2, 9, model->title);
        snprintf(status, sizeof(status), "%lus", (unsigned long)model->seconds);
        canvas_draw_str_aligned(canvas, 126, 9, AlignRight, AlignBottom, status);
        canvas_set_color(canvas, ColorBlack);
        canvas_set_font(canvas, FontKeyboard);
        StudioCommandState state = studio_command_state(&model->command, model->now);
        if(state == StudioCommandTimeout && !model->timeout_hidden) {
            canvas_draw_str(canvas, 2, 23, studio_command_timeout_title(&model->command));
            canvas_draw_str(canvas, 2, 33, model->command.kind == StudioCommandLink ? "Check power / TX-RX" : "Board state unknown");
            canvas_draw_str(canvas, 2, 43, studio_command_can_retry(&model->command) ? "Hold OK: retry" : "Hold OK: check link");
            canvas_draw_str(canvas, 2, 53, "OK: output Back:exit");
        } else if(!model->received) {
            canvas_draw_str(canvas, 2, 23, model->captured ? "Receiving capture..." : (model->seconds < 5 ? "Waiting for reply..." : "No reply yet."));
            canvas_draw_str(canvas, 2, 33, "OK: hold / follow");
            canvas_draw_str(canvas, 2, 43, "Arrows: scroll");
            canvas_draw_str(canvas, 2, 53, model->stop ? "Back: stop and exit" : "Back: return");
        } else {
            for(size_t i = 0; i < STUDIO_ROWS; i++) canvas_draw_str(canvas, 2, 23 + i * 10, model->rows[i]);
        }
        canvas_set_font(canvas, FontSecondary);
        canvas_draw_line(canvas, 0, 55, 127, 55);
        if(model->error) {
            canvas_draw_str(canvas, 2, 63, "SD write failed");
        } else {
            if(state == StudioCommandTimeout) snprintf(status, sizeof(status), "Hold OK: recover");
            else if(state == StudioCommandQuiet) snprintf(status, sizeof(status), "Quiet / hold OK");
            else if(model->seconds % 6 < 3) reply_age(model, status, sizeof(status));
            else if(state == StudioCommandEmpty) snprintf(status, sizeof(status), "Done: no output");
            else if(state == StudioCommandDone) snprintf(status, sizeof(status), "Reply complete");
            else snprintf(status, sizeof(status), "%s %luB", model->follow ? "Live" : (model->held ? "Hold" : "Top"), (unsigned long)(model->received + model->captured));
            canvas_draw_str(canvas, 2, 63, status);
            snprintf(status, sizeof(status), "%s%s", model->logs ? "LOG " : "", model->captures ? "PCAP" : "");
            canvas_draw_str_aligned(canvas, 126, 63, AlignRight, AlignBottom, status);
        }
    }
}

static void refresh_rows(StudioModel* model) {
    if(model->follow) model->top = studio_terminal_tail(&model->terminal);
    if(model->top < model->terminal.first) model->top = model->terminal.first;
    studio_terminal_page(&model->terminal, model->top, model->rows);
}

static bool studio_input(InputEvent* event, void* context) {
    StudioView* studio = context;
    if(event->key == InputKeyBack) return false;
    if(event->type != InputTypeShort && event->type != InputTypeRepeat && event->type != InputTypeLong) return true;
    uint32_t selected = UINT32_MAX;
    with_view_model(studio->view, StudioModel* model, {
        if(model->home && event->type != InputTypeLong) {
            if(event->key == InputKeyUp) model->selected = (model->selected + 4) % 6;
            if(event->key == InputKeyDown) model->selected = (model->selected + 2) % 6;
            if(event->key == InputKeyLeft || event->key == InputKeyRight) model->selected ^= 1;
            if(event->key == InputKeyOk && event->type == InputTypeShort) selected = model->selected;
        } else if(!model->home && event->type == InputTypeLong) {
            if(event->key == InputKeyOk && recovery_available(model)) selected = 6;
        } else if(!model->home) {
            if(event->key == InputKeyOk && event->type == InputTypeShort &&
               studio_command_state(&model->command, model->now) == StudioCommandTimeout &&
               !model->timeout_hidden) {
                model->timeout_hidden = true;
            } else {
                model->held = true;
                uint32_t tail = studio_terminal_tail(&model->terminal);
                if(event->key == InputKeyOk && event->type == InputTypeShort) model->follow = !model->follow;
                if(event->key == InputKeyUp || event->key == InputKeyLeft) {
                    uint32_t step = event->key == InputKeyLeft ? STUDIO_ROWS : 1;
                    model->follow = false;
                    model->top = model->top > model->terminal.first + step ? model->top - step : model->terminal.first;
                }
                if(event->key == InputKeyDown || event->key == InputKeyRight) {
                    uint32_t step = event->key == InputKeyRight ? STUDIO_ROWS : 1;
                    model->follow = false;
                    model->top = model->top + step < tail ? model->top + step : tail;
                }
                refresh_rows(model);
            }
        }
    }, true);
    if(selected != UINT32_MAX) studio->callback(selected, studio->context);
    return true;
}

StudioView* studio_view_alloc(StudioSelectCallback callback, void* context) {
    StudioView* studio = malloc(sizeof(StudioView));
    studio->view = view_alloc();
    studio->callback = callback;
    studio->context = context;
    view_allocate_model(studio->view, ViewModelTypeLocking, sizeof(StudioModel));
    view_set_context(studio->view, studio);
    view_set_draw_callback(studio->view, studio_draw);
    view_set_input_callback(studio->view, studio_input);
    return studio;
}

void studio_view_free(StudioView* studio) {
    view_free(studio->view);
    free(studio);
}

View* studio_view_get_view(StudioView* studio) { return studio->view; }

void studio_view_home(StudioView* studio, uint8_t selected, bool logs, bool captures) {
    with_view_model(studio->view, StudioModel* model, {
        model->home = true;
        model->selected = selected;
        model->logs = logs;
        model->captures = captures;
    }, true);
}

void studio_view_console(StudioView* studio, const char* title, bool follow, bool stop) {
    with_view_model(studio->view, StudioModel* model, {
        memset(model, 0, sizeof(*model));
        studio_terminal_reset(&model->terminal);
        /* Leave room for elapsed time; commands can contain long arguments. */
        snprintf(model->title, sizeof(model->title), "%.14s", title);
        model->follow = follow;
        model->stop = stop;
    }, true);
}

bool studio_view_feed(StudioView* studio, const uint8_t* data, size_t len) {
    bool replied = false;
    with_view_model(studio->view, StudioModel* model, {
        model->now = now_ms();
        replied = studio_command_feed(&model->command, data, len, model->now);
        studio_terminal_feed(&model->terminal, data, len);
        model->received += len;
        if(model->follow || !model->held) refresh_rows(model);
    }, true);
    return replied;
}

void studio_view_status(StudioView* studio, uint32_t seconds, bool logs, bool captures, bool error) {
    with_view_model(studio->view, StudioModel* model, {
        model->seconds = seconds;
        model->logs = logs;
        model->captures = captures;
        model->error = error;
    }, true);
}

void studio_view_report(StudioView* studio, const char* text) {
    with_view_model(studio->view, StudioModel* model, {
        studio_terminal_reset(&model->terminal);
        studio_terminal_feed(&model->terminal, (const uint8_t*)text, strlen(text));
        model->top = 0;
        refresh_rows(model);
    }, true);
}

void studio_view_capture(StudioView* studio, size_t len) {
    with_view_model(studio->view, StudioModel* model, {
        model->captured += len;
        model->now = now_ms();
        if(len && model->command.kind != StudioCommandLink && model->command.kind != StudioCommandQuery &&
           model->command.kind != StudioCommandOff)
            studio_command_reply(&model->command, model->now, false);
    }, true);
}

void studio_view_command(StudioView* studio, const char* command, bool continuous, bool script, uint32_t now) {
    with_view_model(studio->view, StudioModel* model, {
        studio_command_begin(&model->command, command, continuous, script, now);
        model->now = now;
        model->timeout_hidden = false;
    }, true);
}

void studio_view_link_verified(StudioView* studio, uint32_t now) {
    with_view_model(studio->view, StudioModel* model, {
        model->now = now;
        studio_command_reply(&model->command, now, true);
    }, true);
}

void studio_view_reply_age(StudioView* studio, bool seen, uint32_t last_reply) {
    with_view_model(studio->view, StudioModel* model, {
        model->now = now_ms();
        model->reply_seen = seen;
        model->last_reply = last_reply;
    }, true);
}

unsigned studio_view_recovery(StudioView* studio) {
    unsigned action = 0;
    with_view_model(studio->view, StudioModel* model, {
        if(recovery_available(model)) action = studio_command_can_retry(&model->command) ? 1 : 2;
    }, false);
    return action;
}

bool studio_view_link_complete(StudioView* studio) {
    bool complete = false;
    with_view_model(studio->view, StudioModel* model, {
        complete = model->command.kind == StudioCommandLink && model->command.complete;
    }, false);
    return complete;
}
