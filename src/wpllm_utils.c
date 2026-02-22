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

char *make_curl_request(const char *url) {
  CURL *curl = curl_easy_init();

  struct Response chunk = {.data = malloc(1), .size = 0};
  if (!curl) {
    logger(LOG_ERROR, "CURL", "init failed.");
    return "ERROR";
  }

  // url/wp-json/wp/v2/pages
  char wp_url[100];
  sprintf(wp_url, "%s/wp-json/wp/v2/pages", url);
  logger(LOG_INFO, "CURL", "The url is %s", wp_url);
  curl_easy_setopt(curl, CURLOPT_URL, wp_url);
  curl_easy_setopt(curl, CURLOPT_USERAGENT, "wpllm/0.0.1");

  // call back
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writer);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);

  logger(LOG_INFO, "CURL", "Fetching %s", wp_url);
  CURLcode res = curl_easy_perform(curl);
  curl_easy_cleanup(curl);

  if (res != CURLE_OK) {
    logger(LOG_ERROR, "CURL", "Request failed: %s", curl_easy_strerror(res));
    free(chunk.data);
    return "ERROR";
  }

  if (res == CURLE_OK) {
    logger(LOG_INFO, "CURL", "Response success.");
    return chunk.data;
  }

  logger(LOG_INFO, "CURL", "Received %lu bytes", (unsigned long)chunk.size);
}

cJSON *filter_wp_pages(const char *raw_json) {
  cJSON *root = cJSON_Parse(raw_json);
  if (!root)
    return NULL;

  cJSON *new = cJSON_CreateArray();

  cJSON *page = NULL;
  cJSON_ArrayForEach(page, root) {
    cJSON *obj = cJSON_CreateObject();

    // we need id, link, title['rendered'], content['rendered']

    // id
    cJSON_AddItemToObject(obj, "id", cJSON_GetObjectItem(page, "id"));

    // link
    cJSON_AddItemToObject(obj, "link", cJSON_GetObjectItem(page, "link"));

    // title['rendered'], content['rendered']
    cJSON *title = cJSON_GetObjectItem(page, "title");
    cJSON_AddItemToObject(obj, "title", cJSON_GetObjectItem(title, "rendered"));

    cJSON *content = cJSON_GetObjectItem(page, "content");
    cJSON_AddItemToObject(obj, "content",
                          cJSON_GetObjectItem(content, "rendered"));

    cJSON_AddItemToArray(new, obj);
    cJSON_Delete(obj);
  }

  cJSON_Delete(root);

  return new;
}
