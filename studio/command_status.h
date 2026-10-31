#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef enum {
    StudioCommandOff,
    StudioCommandLink,
    StudioCommandQuery,
    StudioCommandStream,
    StudioCommandNetworkScan,
    StudioCommandOther,
    StudioCommandScript,
} StudioCommandKind;

typedef enum {
    StudioCommandWaiting,
    StudioCommandReply,
    StudioCommandDone,
    StudioCommandEmpty,
    StudioCommandQuiet,
    StudioCommandTimeout,
} StudioCommandState;

typedef struct {
    StudioCommandKind kind;
    char command[513];
    char line[256];
    size_t used;
    uint8_t escape;
    uint32_t started;
    uint32_t last_reply;
    bool overflow;
    bool echo;
    bool replied;
    bool complete;
} StudioCommandStatus;

void studio_command_begin(StudioCommandStatus* status, const char* command,
                          bool continuous, bool script, uint32_t now);
/* True only for response content, never a command echo or bare prompt. */
bool studio_command_feed(StudioCommandStatus* status, const uint8_t* data, size_t len, uint32_t now);
void studio_command_reply(StudioCommandStatus* status, uint32_t now, bool complete);
StudioCommandState studio_command_state(const StudioCommandStatus* status, uint32_t now);
bool studio_command_can_retry(const StudioCommandStatus* status);
const char* studio_command_timeout_title(const StudioCommandStatus* status);
