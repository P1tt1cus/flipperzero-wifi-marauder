#include "command_status.h"
#include <stdio.h>
#include <string.h>

static bool query(const char* command) {
    /* Exact allowlist: never replay mutations, arbitrary input or scripts. */
    static const char* const queries[] = {
        "help", "info", "diagnostics", "settings", "channel",
        "recon status", "csimotion status", "remoteid list",
        "list -a", "list -s", "list -c", "list -t", "list -i", "list -p",
        "list -b", "list -f", "list -x", "list -m",
    };
    for(size_t i = 0; i < sizeof(queries) / sizeof(*queries); i++) {
        if(strcmp(command, queries[i]) == 0) return true;
    }
    return false;
}

void studio_command_begin(StudioCommandStatus* status, const char* command,
                          bool continuous, bool script, uint32_t now) {
    memset(status, 0, sizeof(*status));
    status->started = now;
    snprintf(status->command, sizeof(status->command), "%s", command ? command : "");
    if(script) status->kind = StudioCommandScript;
    else if(!command || !*command) status->kind = StudioCommandOff;
    else if(strcmp(command, "protocolinfo --machine studio") == 0) status->kind = StudioCommandLink;
    else if(query(command)) status->kind = StudioCommandQuery;
    else if(strcmp(command, "pingscan") == 0 || strcmp(command, "arpscan") == 0 ||
            strncmp(command, "portscan ", 9) == 0) status->kind = StudioCommandNetworkScan;
    else if(continuous) status->kind = StudioCommandStream;
    else status->kind = StudioCommandOther;
}

void studio_command_reply(StudioCommandStatus* status, uint32_t now, bool complete) {
    status->replied = true;
    status->last_reply = now;
    status->complete |= complete;
}

static bool line_received(StudioCommandStatus* status, uint32_t now) {
    char* line = status->line;
    while(*line == ' ' || *line == '>') line++;
    size_t len = strlen(line);
    while(len && line[len - 1] == ' ') line[--len] = '\0';
    if(!len) return false;
    if(*line == '#') {
        if(!status->overflow && strcmp(line + 1, status->command) == 0) status->echo = true;
        return false;
    }
    if(!strcmp(line, status->command)) return false;
    if(status->kind == StudioCommandLink || status->kind == StudioCommandOff) return false;
    studio_command_reply(status, now, false);
    return true;
}

bool studio_command_feed(StudioCommandStatus* status, const uint8_t* data, size_t len, uint32_t now) {
    bool reply = false;
    for(size_t i = 0; i < len; i++) {
        uint8_t c = data[i];
        if(status->escape == 1) { status->escape = c == '[' ? 2 : (c == ']' ? 3 : 0); continue; }
        if(status->escape == 2) { if(c >= 0x40 && c <= 0x7e) status->escape = 0; continue; }
        if(status->escape == 3) { if(c == 7) status->escape = 0; else if(c == 27) status->escape = 4; continue; }
        if(status->escape == 4) { status->escape = c == '\\' ? 0 : 3; continue; }
        if(c == 27) { status->escape = 1; continue; }
        if(c == '\r') continue;
        if(c == '\n') {
            reply |= line_received(status, now);
            status->used = 0;
            status->line[0] = '\0';
            status->overflow = false;
        } else if(c >= 32 && c < 127) {
            /* The prompt has no newline. Only synchronous allowlisted queries
             * can finish at a prompt following their exact echoed command. */
            if(c == ' ' && !strcmp(status->line, ">") && status->echo &&
               status->kind == StudioCommandQuery) {
                status->complete = true;
            }
            if(status->used + 1 < sizeof(status->line)) {
                status->line[status->used++] = c;
                status->line[status->used] = '\0';
            } else status->overflow = true;
        }
    }
    return reply;
}

StudioCommandState studio_command_state(const StudioCommandStatus* status, uint32_t now) {
    if(status->complete) return status->replied ? StudioCommandDone : StudioCommandEmpty;
    if(status->kind == StudioCommandOff) return StudioCommandReply;
    /* Scripts may intentionally be silent during delays. Never time them out. */
    if(status->kind == StudioCommandScript) return StudioCommandReply;
    uint32_t elapsed = now - status->started;
    if(!status->replied) {
        uint32_t limit = status->kind == StudioCommandNetworkScan ? 60000 :
                         status->kind == StudioCommandStream ? 15000 :
                         status->kind == StudioCommandOther ? 30000 : 10000;
        return elapsed >= limit ? StudioCommandTimeout : StudioCommandWaiting;
    }
    uint32_t quiet = now - status->last_reply;
    if(status->kind == StudioCommandQuery && quiet >= 10000) return StudioCommandTimeout;
    /* Quiet asynchronous output cannot prove either completion or disconnect. */
    if(quiet >= (status->kind == StudioCommandNetworkScan ? 60000u : 30000u))
        return StudioCommandQuiet;
    return StudioCommandReply;
}

bool studio_command_can_retry(const StudioCommandStatus* status) {
    return status->kind == StudioCommandLink || status->kind == StudioCommandQuery;
}

const char* studio_command_timeout_title(const StudioCommandStatus* status) {
    switch(status->kind) {
    case StudioCommandLink: return "Link timed out (10s)";
    case StudioCommandQuery: return "Query timed out";
    case StudioCommandNetworkScan: return "No scan reply (60s)";
    case StudioCommandStream: return "No start reply (15s)";
    default: return "No reply (30s)";
    }
}
