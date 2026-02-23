// 22.02.2026 08:11PM Nikhil Kapila
// C file for curl fetch utils

#ifndef WPLLM_UTILS_H
#define WPLLM_UTILS_H

#include "cJSON.h"
#include <stdlib.h>

char *make_curl_request_pages(const char *url);
char *make_curl_request_posts(const char *url);
char *make_curl_request_endpoint(const char *url, const char *endpoint);

int is_url_valid(const char *url);

cJSON *filter_wp_pages(const char *raw_json);
cJSON *filter_wp_posts(const char *raw_json);
cJSON *merge_item_arrays(cJSON *a, cJSON *b);

char *html_to_markdown(const char *html);
void write_llm_file(const char *outpath, cJSON *items);

#endif
