// 22.02.2026 08:11PM Nikhil Kapila
// C file for curl fetch utils

#ifndef WPLLM_UTILS_H
#define WPLLM_UTILS_H

#include "cJSON.h"
#include <stdlib.h>

char *make_curl_request(const char *url);
char *make_curl_request_endpoint(const char *url, const char *endpoint);

int is_url_valid(const char *url);

cJSON *filter_wp_pages(const char *raw_json);

#endif
