// 22.02.2026 08:11PM Nikhil Kapila
// C file for curl fetch utils

#include "wpllm_utils.h"
#include "cJSON.h"
#include "logger.h"
#include <curl/curl.h>
#include <stdlib.h>
#include <string.h>

struct Response {
  char *data;
  size_t size;
};

size_t writer(void *contents, size_t size, size_t nmemb, void *userp) {
  size_t realsize = size * nmemb;

  // cast back to struct
  struct Response *resp = (struct Response *)userp;
  char *ptr = realloc(resp->data, resp->size + realsize + 1);
  if (!ptr)
    return 0;

  resp->data = ptr;
  memcpy(&(resp->data[resp->size]), contents, realsize);

  resp->size += realsize;
  resp->data[resp->size] = '\0';

  return realsize;
}

int is_url_valid(const char *url) {
  CURLU *u = curl_url();
  CURLUcode code = curl_url_set(u, CURLUPART_URL, url, 0);
  curl_url_cleanup(u);
  return (code == CURLUE_OK);
}

char *make_curl_request_endpoint(const char *url, const char *endpoint) {
  CURL *curl = curl_easy_init();
  struct Response chunk = {.data = malloc(1), .size = 0};
  if (!curl) {
    logger(LOG_ERROR, "CURL", "init failed.");
    free(chunk.data);
    return "ERROR";
  }

  char wp_url[256];
  int written = snprintf(wp_url, sizeof(wp_url), "%s/wp-json/wp/v2/%s", url, endpoint);
  if (written < 0 || (size_t)written >= sizeof(wp_url)) {
    logger(LOG_ERROR, "CURL", "URL too long.");
    free(chunk.data);
    curl_easy_cleanup(curl);
    return "ERROR";
  }

  logger(LOG_INFO, "CURL", "Fetching %s", wp_url);
  curl_easy_setopt(curl, CURLOPT_URL, wp_url);
  curl_easy_setopt(curl, CURLOPT_USERAGENT, "wpllm/0.0.1");
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writer);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);

  CURLcode res = curl_easy_perform(curl);
  curl_easy_cleanup(curl);

  if (res != CURLE_OK) {
    logger(LOG_ERROR, "CURL", "Request failed: %s", curl_easy_strerror(res));
    free(chunk.data);
    return "ERROR";
  }

  logger(LOG_INFO, "CURL", "Response success, %lu bytes", (unsigned long)chunk.size);
  return chunk.data;
}

char *make_curl_request_pages(const char *url) {
  return make_curl_request_endpoint(url, "pages");
}

char *make_curl_request_posts(const char *url) {
  return make_curl_request_endpoint(url, "posts");
}

cJSON *filter_wp_pages(const char *raw_json) {
  cJSON *root = cJSON_Parse(raw_json);
  if (!root)
    return NULL;

  cJSON *new = cJSON_CreateArray();

  cJSON *page = NULL;
  cJSON_ArrayForEach(page, root) {
    cJSON *obj = cJSON_CreateObject();
    if (!obj) continue;

    // we need id, link, title['rendered'], content['rendered']

    // id
    cJSON *id = cJSON_GetObjectItem(page, "id");
    if (id) cJSON_AddItemToObject(obj, "id", cJSON_Duplicate(id, 1));

    // link
    cJSON *link = cJSON_GetObjectItem(page, "link");
    if (link) cJSON_AddItemToObject(obj, "link", cJSON_Duplicate(link, 1));

    // title['rendered'], content['rendered']
    // title
    cJSON *title = cJSON_GetObjectItem(page, "title");
    cJSON *title_rendered = title ? cJSON_GetObjectItem(title, "rendered") : NULL;
    if (title_rendered)
      cJSON_AddItemToObject(obj, "title", cJSON_Duplicate(title_rendered, 1));
    else
      cJSON_AddItemToObject(obj, "title", cJSON_CreateString(""));

    // content
    cJSON *content = cJSON_GetObjectItem(page, "content");
    cJSON *content_rendered = content ? cJSON_GetObjectItem(content, "rendered") : NULL;
    if (content_rendered)
      cJSON_AddItemToObject(obj, "content", cJSON_Duplicate(content_rendered, 1));
    else
      cJSON_AddItemToObject(obj, "content", cJSON_CreateString(""));

    cJSON_AddItemToArray(new, obj);
  }

  cJSON_Delete(root);
  return new;
}
