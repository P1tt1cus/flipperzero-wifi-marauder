#pragma once
#include <gui/view.h>

typedef struct StudioView StudioView;
typedef void (*StudioSelectCallback)(uint32_t index, void* context);
StudioView* studio_view_alloc(StudioSelectCallback callback, void* context);
void studio_view_free(StudioView* studio);
View* studio_view_get_view(StudioView* studio);
void studio_view_home(StudioView* studio, uint8_t selected, bool logs, bool captures);
void studio_view_console(StudioView* studio, const char* title, bool follow, bool stop);
bool studio_view_feed(StudioView* studio, const uint8_t* data, size_t len);
void studio_view_status(StudioView* studio, uint32_t seconds, bool logs, bool captures, bool error);

void studio_view_report(StudioView* studio, const char* text);
void studio_view_capture(StudioView* studio, size_t len);

/* Input callback index 6 requests timeout recovery; home uses indices 0..5. */
void studio_view_command(StudioView* studio, const char* command, bool continuous, bool script, uint32_t now);
void studio_view_link_verified(StudioView* studio, uint32_t now);
void studio_view_reply_age(StudioView* studio, bool seen, uint32_t last_reply);
/* 0: no recovery, 1: retry read-only query, 2: check link. */
unsigned studio_view_recovery(StudioView* studio);

bool studio_view_link_complete(StudioView* studio);
