#include "link_report.h"
#include "../script/cJSON.h"
#include <stdio.h>
#include <string.h>
#include <stdint.h>

bool studio_link_report(const char* output, char* report, size_t size) {
    bool diagnostics;
    return studio_link_report_with_features(output, report, size, &diagnostics);
}

static cJSON* parse_json(const char* output) {
    const char* json = strstr(output, "@MARAUDER:");
    if(!json) return NULL;
    json += strlen("@MARAUDER:");
    const char* end = strchr(json, '\n');
    if(!end || end - json > 1024) return NULL;
    /* Bound nesting before invoking the recursive JSON parser on UART input. */
    unsigned depth = 0;
    bool quoted = false;
    bool escaped = false;
    for(const char* p = json; p < end; p++) {
        if(escaped) { escaped = false; continue; }
        if(quoted && *p == '\\') { escaped = true; continue; }
        if(*p == '"') { quoted = !quoted; continue; }
        if(quoted) continue;
        if(*p == '{' || *p == '[') { if(++depth > 4) return NULL; }
        else if(*p == '}' || *p == ']') { if(!depth) return NULL; depth--; }
    }
    if(depth || quoted) return NULL;
    return cJSON_ParseWithLength(json, end - json);
}

static bool envelope(const cJSON* reply, const char* expected) {
    const cJSON* tx = cJSON_GetObjectItemCaseSensitive(reply, "tx");
    const cJSON* command = cJSON_GetObjectItemCaseSensitive(reply, "command");
    const cJSON* status = cJSON_GetObjectItemCaseSensitive(reply, "status");
    const cJSON* protocol = cJSON_GetObjectItemCaseSensitive(reply, "protocol");
    return cJSON_IsString(tx) && strcmp(tx->valuestring, "studio") == 0 &&
        cJSON_IsString(command) && strcmp(command->valuestring, expected) == 0 &&
        cJSON_IsString(status) && strcmp(status->valuestring, "success") == 0 &&
        cJSON_IsNumber(protocol) && protocol->valuedouble == 1;
}


static bool integer(const cJSON* value, double low, double high) {
    return cJSON_IsNumber(value) && value->valuedouble >= low && value->valuedouble <= high &&
           value->valuedouble == (double)(int64_t)value->valuedouble;
}

static void radio_summary(const cJSON* radio, char* report, size_t size) {
    const cJSON* target = cJSON_GetObjectItemCaseSensitive(radio, "target");
    const cJSON* country = cJSON_GetObjectItemCaseSensitive(radio, "country");
    const cJSON* first = cJSON_GetObjectItemCaseSensitive(radio, "first");
    const cJSON* count = cJSON_GetObjectItemCaseSensitive(radio, "count");
    const cJSON* manual = cJSON_GetObjectItemCaseSensitive(radio, "manual");
    const cJSON* power = cJSON_GetObjectItemCaseSensitive(radio, "powerQdbm");
    const cJSON* limit = cJSON_GetObjectItemCaseSensitive(radio, "limitQdbm");
    const cJSON* channel = cJSON_GetObjectItemCaseSensitive(radio, "channel");
    const cJSON* active = cJSON_GetObjectItemCaseSensitive(radio, "active");
    const cJSON* verified = cJSON_GetObjectItemCaseSensitive(radio, "verified");
    const cJSON* errors = cJSON_GetObjectItemCaseSensitive(radio, "errors");
    const char* error_names[] = {"countryError", "powerError", "channelError", "applyError", "lastError"};
    int codes[5];
    bool valid = cJSON_IsObject(radio) && cJSON_IsString(target) && !strcmp(target->valuestring,"AU") &&
        integer(limit,80,80) && cJSON_IsBool(active) && cJSON_IsBool(verified) && integer(errors,0,UINT32_MAX);
    for(size_t i=0;i<5;i++) {
        const cJSON* error=cJSON_GetObjectItemCaseSensitive(radio,error_names[i]);
        if(!integer(error,-1,65535)) valid=false;
        codes[i]=integer(error,-1,65535) ? error->valueint : -1;
    }
    bool have_country=cJSON_IsString(country) && strlen(country->valuestring)==2 &&
        country->valuestring[0]>='A' && country->valuestring[0]<='Z' &&
        country->valuestring[1]>='A' && country->valuestring[1]<='Z' &&
        integer(first,1,14) && integer(count,1,14) && first->valueint+count->valueint<=15 && cJSON_IsBool(manual);
    valid &= have_country ? codes[0]==0 :
        cJSON_IsNull(country) && cJSON_IsNull(first) && cJSON_IsNull(count) && cJSON_IsNull(manual) && codes[0]!=0;
    valid &= integer(power,8,84) ? codes[1]==0 : cJSON_IsNull(power) && codes[1]!=0;
    valid &= integer(channel,1,14) ? codes[2]==0 : cJSON_IsNull(channel) && codes[2]!=0;
    if(!valid) { snprintf(report,size,"Radio data invalid\nAU not verified"); return; }
    bool confirmed=cJSON_IsTrue(active) && cJSON_IsTrue(verified) && have_country &&
        !strcmp(country->valuestring,"AU") && first->valueint==1 && count->valueint==13 && cJSON_IsTrue(manual) &&
        integer(power,8,80) && integer(channel,1,13) && codes[3]==0;
    char country_line[40], power_line[32], channel_line[24];
    if(have_country) snprintf(country_line,sizeof(country_line),"Country: %s\nChannels: %d-%d",country->valuestring,first->valueint,first->valueint+count->valueint-1);
    else snprintf(country_line,sizeof(country_line),"Country unavailable\nTarget: AU 1-13");
    if(cJSON_IsNumber(power)) snprintf(power_line,sizeof(power_line),"TX max: %d.%02d dBm",power->valueint/4,(power->valueint%4)*25);
    else snprintf(power_line,sizeof(power_line),"TX max: unavailable");
    if(cJSON_IsNumber(channel)) snprintf(channel_line,sizeof(channel_line),"Current channel: %d",channel->valueint);
    else snprintf(channel_line,sizeof(channel_line),"Channel unavailable");
    snprintf(report,size,"%s\n%s\n%s\n%s\nCap: 20 dBm\nPolicy: %s\nRead errs C/P/CH:\n%d / %d / %d\nApply err: %d\nLast err: %d\nError count: %lu",
        confirmed ? "AU radio confirmed" : (cJSON_IsTrue(active) ? "AU not verified" : "WiFi off / not live"),
        country_line,power_line,channel_line,cJSON_IsTrue(manual) ? "manual" : (cJSON_IsFalse(manual) ? "auto" : "unavailable"),
        codes[0],codes[1],codes[2],codes[3],codes[4],(unsigned long)errors->valuedouble);
}

static bool parse_frame(const char* output, char* report, size_t size, bool* diagnostics, bool* radio) {
    *diagnostics = false;
    *radio = false;
    cJSON* reply = parse_json(output);
    const cJSON* firmware = cJSON_GetObjectItemCaseSensitive(reply,"firmware");
    if(!envelope(reply,"protocolinfo") || !cJSON_IsString(firmware) || !firmware->valuestring[0]) {
        cJSON_Delete(reply); return false;
    }
    snprintf(report, size, "Board replied\nFW %.17s\nProtocol 1 / OK\nUART 115200", firmware->valuestring);
    const cJSON* profile = cJSON_GetObjectItemCaseSensitive(reply, "profile");
    const cJSON* features = cJSON_GetObjectItemCaseSensitive(reply, "capabilities");
    if(cJSON_IsString(profile) &&
       strcmp(profile->valuestring, "studio-zy-esp32") == 0 && cJSON_IsArray(features)) {
        const cJSON* feature;
        bool well_formed = true;
        cJSON_ArrayForEach(feature, features) {
            if(!cJSON_IsString(feature)) { well_formed = false; break; }
            if(strcmp(feature->valuestring, "diagnostics") == 0) *diagnostics = true;
            if(strcmp(feature->valuestring, "radio-au") == 0) *radio = true;
        }
        if(!well_formed) { *diagnostics = false; *radio = false; }
        else snprintf(report, size, "Studio ZY-ESP32\nFW %.17s\nProfile confirmed\nUART 115200", firmware->valuestring);
    }
    if(*radio) {
        char region[384];
        radio_summary(cJSON_GetObjectItemCaseSensitive(reply, "radio"), region, sizeof(region));
        snprintf(report, size, "Board replied\n%s\nFW %.17s\nUART 115200", region, firmware->valuestring);
    }
    cJSON_Delete(reply);
    return true;
}


bool studio_link_report_with_features(const char* output, char* report, size_t size, bool* diagnostics) {
    bool radio;
    return studio_link_report_full(output, report, size, diagnostics, &radio);
}

bool studio_link_report_full(const char* output, char* report, size_t size, bool* diagnostics, bool* radio) {
    *diagnostics = false;
    *radio = false;
    const char* frame = output;
    while((frame = strstr(frame, "@MARAUDER:"))) {
        if(parse_frame(frame, report, size, diagnostics, radio)) return true;
        const char* end = strchr(frame, '\n');
        if(!end) break;
        frame = end + 1;
    }
    return false;
}

bool studio_radio_report(const char* output, char* report, size_t size) {
    const char* frame=output;
    while((frame=strstr(frame,"@MARAUDER:"))) {
        cJSON* reply=parse_json(frame);
        const cJSON* profile=cJSON_GetObjectItemCaseSensitive(reply,"profile");
        bool valid=envelope(reply,"radiostatus") && cJSON_IsString(profile) && !strcmp(profile->valuestring,"studio-zy-esp32");
        if(valid) radio_summary(cJSON_GetObjectItemCaseSensitive(reply,"radio"),report,size);
        cJSON_Delete(reply);
        if(valid) return true;
        const char* end=strchr(frame,'\n');
        if(!end) break;
        frame=end+1;
    }
    return false;
}
