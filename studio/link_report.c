#include "link_report.h"
#include "../script/cJSON.h"
#include <stdio.h>
#include <string.h>

bool studio_link_report(const char* output, char* report, size_t size) {
    bool diagnostics;
    return studio_link_report_with_features(output, report, size, &diagnostics);
}

static bool parse_frame(
    const char* output, char* report, size_t size, bool* diagnostics) {
    *diagnostics = false;
    const char* json = strstr(output, "@MARAUDER:");
    if(!json) return false;
    json += strlen("@MARAUDER:");
    const char* end = strchr(json, '\n');
    if(!end || end - json > 1024) return false;
    /* Bound nesting before invoking the recursive JSON parser on UART input. */
    unsigned depth = 0;
    bool quoted = false;
    bool escaped = false;
    for(const char* p = json; p < end; p++) {
        if(escaped) { escaped = false; continue; }
        if(quoted && *p == '\\') { escaped = true; continue; }
        if(*p == '"') { quoted = !quoted; continue; }
        if(quoted) continue;
        if(*p == '{' || *p == '[') { if(++depth > 4) return false; }
        else if(*p == '}' || *p == ']') { if(!depth) return false; depth--; }
    }
    if(depth || quoted) return false;
    cJSON* reply = cJSON_ParseWithLength(json, end - json);
    const cJSON* tx = cJSON_GetObjectItemCaseSensitive(reply, "tx");
    const cJSON* command = cJSON_GetObjectItemCaseSensitive(reply, "command");
    const cJSON* status = cJSON_GetObjectItemCaseSensitive(reply, "status");
    const cJSON* firmware = cJSON_GetObjectItemCaseSensitive(reply, "firmware");
    const cJSON* protocol = cJSON_GetObjectItemCaseSensitive(reply, "protocol");
    bool valid = cJSON_IsString(tx) && strcmp(tx->valuestring, "studio") == 0 &&
        cJSON_IsString(command) && strcmp(command->valuestring, "protocolinfo") == 0 &&
        cJSON_IsString(status) && strcmp(status->valuestring, "success") == 0 &&
        cJSON_IsString(firmware) && firmware->valuestring[0] &&
        cJSON_IsNumber(protocol) && protocol->valuedouble == 1;
    if(valid) snprintf(report, size, "Board replied\nFW %.17s\nProtocol %d / OK\nUART 115200", firmware->valuestring, protocol->valueint);
    const cJSON* profile = cJSON_GetObjectItemCaseSensitive(reply, "profile");
    const cJSON* features = cJSON_GetObjectItemCaseSensitive(reply, "capabilities");
    if(valid && cJSON_IsString(profile) &&
       strcmp(profile->valuestring, "studio-zy-esp32") == 0 && cJSON_IsArray(features)) {
        const cJSON* feature;
        bool well_formed = true;
        cJSON_ArrayForEach(feature, features) {
            if(!cJSON_IsString(feature)) { well_formed = false; break; }
            if(strcmp(feature->valuestring, "diagnostics") == 0) *diagnostics = true;
        }
        if(!well_formed) *diagnostics = false;
        else snprintf(report, size, "Studio ZY-ESP32\nFW %.17s\nProfile confirmed\nUART 115200", firmware->valuestring);
    }
    cJSON_Delete(reply);
    return valid;
}

bool studio_link_report_with_features(
    const char* output, char* report, size_t size, bool* diagnostics) {
    *diagnostics = false;
    const char* frame = output;
    while((frame = strstr(frame, "@MARAUDER:"))) {
        if(parse_frame(frame, report, size, diagnostics)) return true;
        const char* end = strchr(frame, '\n');
        if(!end) break;
        frame = end + 1;
    }
    return false;
}
