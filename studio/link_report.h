#pragma once
#include <stdbool.h>
#include <stddef.h>

bool studio_link_report(const char* output, char* report, size_t size);
bool studio_link_report_with_features(
    const char* output, char* report, size_t size, bool* diagnostics);
