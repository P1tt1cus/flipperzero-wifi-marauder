#pragma once
#include <stdbool.h>
#include <stddef.h>

bool studio_link_report(const char* output, char* report, size_t size);
bool studio_link_report_with_features(
    const char* output, char* report, size_t size, bool* diagnostics);

bool studio_link_report_full(const char* output, char* report, size_t size, bool* diagnostics, bool* radio);
bool studio_radio_report(const char* output, char* report, size_t size);
