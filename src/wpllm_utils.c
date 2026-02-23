// 22.02.2026 08:11PM Nikhil Kapila
// C file for curl fetch utils

#include "wpllm_utils.h"
#include "cJSON.h"
#include "logger.h"
#include <ctype.h>
#include <curl/curl.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>

/* Returns 1 if haystack contains needle case-insensitively. */
static int contains_ignore_case(const char *haystack, const char *needle) {
  size_t nlen = strlen(needle);
  if (nlen == 0)
    return 1;
  for (; *haystack; haystack++) {
    size_t i = 0;
    for (; i < nlen && haystack[i]; i++)
      if (tolower((unsigned char)haystack[i]) !=
          tolower((unsigned char)needle[i]))
        break;
    if (i == nlen)
      return 1;
  }
  return 0;
}

#define TMP_TEMPLATE "/tmp/wpllm_markitdown_XXXXXX"
#define MARKITDOWN_CMD "markitdown"
#define READ_CHUNK 4096

struct Response {
  char *data;
  size_t size;
};

size_t writer(void *contents, size_t size, size_t nmemb, void *userp) {
  size_t realsize = size * nmemb;

  // cast back to struct
  struct Response *resp = (struct Response *)userp;
  char *ptr = realloc(resp->data, resp->size + realsize + 1);
  if (!ptr) {
    logger(LOG_ERROR, "CURL", "realloc failed in response writer.");
    return 0;
  }

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

  // strip trailing slash so we don't get double slash in path
  size_t url_len = strlen(url);
  if (url_len > 0 && url[url_len - 1] == '/')
    url_len--;

  char wp_url[256];
  int written = snprintf(wp_url, sizeof(wp_url), "%.*s/wp-json/wp/v2/%s",
                         (int)url_len, url, endpoint);
  if (written < 0 || (size_t)written >= sizeof(wp_url)) {
    logger(LOG_ERROR, "CURL", "URL too long.");
    free(chunk.data);
    curl_easy_cleanup(curl);
    return "ERROR";
  }

  logger(LOG_INFO, "CURL", "Fetching %s", wp_url);
  curl_easy_setopt(curl, CURLOPT_URL, wp_url);
  curl_easy_setopt(
      curl, CURLOPT_USERAGENT,
      "Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_7) "
      "AppleWebKit/537.36 (KHTML, like Gecko) Chrome/131.0.0.0 Safari/537.36");
  struct curl_slist *headers = curl_slist_append(NULL, "Accept: application/json");
  curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
  curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
  curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writer);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);

  CURLcode res = curl_easy_perform(curl);

  long http_code = 0;
  curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
  curl_easy_cleanup(curl);
  curl_slist_free_all(headers);

  if (res != CURLE_OK) {
    logger(LOG_ERROR, "CURL", "Request failed: %s", curl_easy_strerror(res));
    free(chunk.data);
    return "ERROR";
  }

  if (http_code < 200 || http_code >= 300) {
    logger(LOG_ERROR, "CURL", "HTTP %ld (expected 2xx); response may not be JSON.",
           http_code);
    free(chunk.data);
    return "ERROR";
  }

  logger(LOG_INFO, "CURL", "Response success, %lu bytes",
         (unsigned long)chunk.size);
  return chunk.data;
}

char *make_curl_request_pages(const char *url) {
  return make_curl_request_endpoint(url, "pages");
}

char *make_curl_request_posts(const char *url) {
  return make_curl_request_endpoint(url, "posts");
}

/* WP REST API uses the same schema for pages and posts (id, link,
 * title.rendered, content.rendered). */
static cJSON *filter_wp_items(const char *raw_json) {
  const char *parse_start = raw_json;
  /* Skip UTF-8 BOM if present (some servers send it). */
  if (raw_json && (unsigned char)raw_json[0] == 0xEF &&
      (unsigned char)raw_json[1] == 0xBB && (unsigned char)raw_json[2] == 0xBF)
    parse_start = raw_json + 3;

  cJSON *root = cJSON_Parse(parse_start);
  if (!root) {
    const char *err = cJSON_GetErrorPtr();
    if (err && *err)
      logger(LOG_ERROR, "PARSE", "Failed to parse JSON (near: \"%.40s...\").",
             err);
    else
      logger(LOG_ERROR, "PARSE", "Failed to parse JSON.");
    return NULL;
  }

  if (!cJSON_IsArray(root)) {
    logger(LOG_ERROR, "PARSE",
           "Expected JSON array (e.g. API error or disabled REST).");
    cJSON_Delete(root);
    return NULL;
  }

  cJSON *new = cJSON_CreateArray();

  cJSON *item = NULL;
  cJSON_ArrayForEach(item, root) {
    /* Skip items whose title contains "copy" or "sample" (case-insensitive). */
    cJSON *title = cJSON_GetObjectItem(item, "title");
    cJSON *title_rendered =
        title ? cJSON_GetObjectItem(title, "rendered") : NULL;
    const char *title_str = (title_rendered && cJSON_IsString(title_rendered))
                                ? title_rendered->valuestring
                                : "";
    if (contains_ignore_case(title_str, "copy") ||
        contains_ignore_case(title_str, "sample"))
      continue;

    cJSON *obj = cJSON_CreateObject();
    if (!obj)
      continue;

    cJSON *id = cJSON_GetObjectItem(item, "id");
    if (id)
      cJSON_AddItemToObject(obj, "id", cJSON_Duplicate(id, 1));

    cJSON *link = cJSON_GetObjectItem(item, "link");
    if (link)
      cJSON_AddItemToObject(obj, "link", cJSON_Duplicate(link, 1));

    if (title_rendered)
      cJSON_AddItemToObject(obj, "title", cJSON_Duplicate(title_rendered, 1));
    else
      cJSON_AddItemToObject(obj, "title", cJSON_CreateString(""));

    cJSON *content = cJSON_GetObjectItem(item, "content");
    cJSON *content_rendered =
        content ? cJSON_GetObjectItem(content, "rendered") : NULL;
    if (content_rendered)
      cJSON_AddItemToObject(obj, "content",
                            cJSON_Duplicate(content_rendered, 1));
    else
      cJSON_AddItemToObject(obj, "content", cJSON_CreateString(""));

    cJSON_AddItemToArray(new, obj);
  }

  cJSON_Delete(root);
  return new;
}

cJSON *filter_wp_pages(const char *raw_json) {
  return filter_wp_items(raw_json);
}

cJSON *filter_wp_posts(const char *raw_json) {
  return filter_wp_items(raw_json);
}

/* Merges two arrays of WP items into one (duplicates items; caller frees a and
 * b). */
cJSON *merge_item_arrays(cJSON *a, cJSON *b) {
  cJSON *merged = cJSON_CreateArray();
  if (!merged) {
    logger(LOG_ERROR, "PARSE", "Failed to allocate merged array.");
    return NULL;
  }
  cJSON *item = NULL;
  cJSON_ArrayForEach(item, a) {
    cJSON_AddItemToArray(merged, cJSON_Duplicate(item, 1));
  }
  cJSON_ArrayForEach(item, b) {
    cJSON_AddItemToArray(merged, cJSON_Duplicate(item, 1));
  }
  return merged;
}

 // strip <script>, <div>, and <p> (unwrap; keep inner content)
static char *strip_divs_and_scripts(const char *html) {
  size_t len = strlen(html);
  char *out = malloc(len + 1);
  if (!out)
    return NULL;
  size_t i = 0, o = 0;
  while (i < len) {
    if (i + 7 <= len && strncasecmp(html + i, "<script", 7) == 0) {
      const char *end = strstr(html + i + 7, "</script>");
      if (end) {
        i = (size_t)(end - html) + 9;
        continue;
      }
    }
    if (i + 4 <= len && strncasecmp(html + i, "<div", 4) == 0) {
      size_t j = i + 4;
      while (j < len && html[j] != '>') {
        if (html[j] == '"' || html[j] == '\'') {
          char q = html[j++];
          while (j < len && html[j] != q) j++;
          if (j < len) j++;
        } else
          j++;
      }
      if (j < len) {
        i = j + 1;
        continue;
      }
    }
    if (i + 6 <= len && strncasecmp(html + i, "</div>", 6) == 0) {
      i += 6;
      continue;
    }
    /* <p> or <p ...> */
    if (i + 2 <= len && strncasecmp(html + i, "<p", 2) == 0 &&
        (html[i + 2] == '>' || html[i + 2] == ' ' || html[i + 2] == '\t')) {
      size_t j = i + 2;
      while (j < len && html[j] != '>') {
        if (html[j] == '"' || html[j] == '\'') {
          char q = html[j++];
          while (j < len && html[j] != q) j++;
          if (j < len) j++;
        } else
          j++;
      }
      if (j < len) {
        i = j + 1;
        continue;
      }
    }
    if (i + 4 <= len && strncasecmp(html + i, "</p>", 4) == 0) {
      i += 4;
      continue;
    }
    out[o++] = html[i++];
  }
  out[o] = '\0';
  return out;
}

char *html_to_markdown(const char *html) {
  if (!html || !*html)
    return strdup("");

  char *cleaned = strip_divs_and_scripts(html);
  if (!cleaned)
    return strdup("");
  html = cleaned;

  char path[sizeof(TMP_TEMPLATE)];
  strncpy(path, TMP_TEMPLATE, sizeof(path) - 1);
  path[sizeof(path) - 1] = '\0';

  int fd = mkstemp(path);
  if (fd < 0) {
    logger(LOG_ERROR, "MARKDOWN", "mkstemp failed for temp file.");
    free(cleaned);
    return NULL;
  }

  size_t len = strlen(html);
  size_t written = 0;
  while (written < len) {
    ssize_t n = write(fd, html + written, len - written);
    if (n <= 0)
      break;
    written += (size_t)n;
  }
  close(fd);
  free(cleaned);
  if (written != len) {
    logger(LOG_ERROR, "MARKDOWN", "Failed to write temp file.");
    unlink(path);
    return NULL;
  }

  /* Rename to .html so markitdown uses HTML converter (and UTF-8). */
  char path_html[64];
  int nh = snprintf(path_html, sizeof(path_html), "%s.html", path);
  if (nh < 0 || nh >= (int)sizeof(path_html) || rename(path, path_html) != 0) {
    logger(LOG_ERROR, "MARKDOWN", "Failed to rename temp file to .html.");
    unlink(path);
    return NULL;
  }

  char cmd[320];
  int nc = snprintf(cmd, sizeof(cmd), "%s \"%s\"", MARKITDOWN_CMD, path_html);
  if (nc < 0 || (size_t)nc >= sizeof(cmd)) {
    logger(LOG_ERROR, "MARKDOWN", "Temp path too long for markitdown command.");
    unlink(path_html);
    return NULL;
  }

  /* Ensure markitdown (Python) reads the temp file as UTF-8 */
  setenv("PYTHONUTF8", "1", 1); /* Python 3.7+ UTF-8 mode */
  setenv("LC_ALL", "en_US.UTF-8", 1);
  setenv("LANG", "en_US.UTF-8", 1);

  FILE *p = popen(cmd, "r");
  if (!p) {
    logger(LOG_ERROR, "MARKDOWN", "popen(%s) failed.", MARKITDOWN_CMD);
    unlink(path_html);
    return NULL;
  }

  char *out = NULL;
  size_t outcap = 0, outlen = 0;
  char buf[READ_CHUNK];

  while (fgets(buf, sizeof(buf), p)) {
    size_t n = strlen(buf);
    if (outlen + n + 1 > outcap) {
      size_t want = outcap ? outcap * 2 : (size_t)READ_CHUNK;
      if (want < outlen + n + 1)
        want = outlen + n + 1;
      char *tmp = realloc(out, want);
      if (!tmp)
        break;
      out = tmp;
      outcap = want;
    }
    memcpy(out + outlen, buf, n + 1);
    outlen += n;
  }
  pclose(p);
  unlink(path_html);

  if (!out)
    out = strdup("");
  return out;
}

void write_llm_file(const char *outpath, cJSON *items) {
  FILE *f = fopen(outpath, "w");
  if (!f) {
    logger(LOG_ERROR, "OUTPUT", "Cannot open %s for writing.", outpath);
    return;
  }
  size_t count = cJSON_GetArraySize(items);
  logger(LOG_INFO, "OUTPUT", "Writing %zu items to %s", count, outpath);
  cJSON *item = NULL;
  cJSON_ArrayForEach(item, items) {
    cJSON *title = cJSON_GetObjectItem(item, "title");
    cJSON *link = cJSON_GetObjectItem(item, "link");
    cJSON *content = cJSON_GetObjectItem(item, "content");
    const char *t = (title && cJSON_IsString(title)) ? title->valuestring : "";
    const char *l = (link && cJSON_IsString(link)) ? link->valuestring : "";
    const char *c =
        (content && cJSON_IsString(content)) ? content->valuestring : "";

    char *md = html_to_markdown(c);
    if (!md)
      md = strdup(c);

    fprintf(f, "## %s\n\n", t);
    fprintf(f, "Link: %s\n\n", l);
    fprintf(f, "%s\n\n", md);

    free(md);
  }
  fclose(f);
}
