#include "cJSON.h"
#include "logger.h"
#include "wpllm_utils.h"
#include <curl/curl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define USAGE                                                                  \
  "usage:\n"                                                                   \
  " wpllm [options]\n"                                                         \
  "options:\n"                                                                 \
  " -u [url]  The wordpress site URL to prepare llms.txt (website examples: "  \
  "https://wordpress.org/showcase)\n"                                          \
  " -p        Boolean flag to get only pages.\n"                               \
  " -b        Boolean flag to get only (blog) posts.\n"                        \
  " -h        Show this help message.\n"                                       \
  " -v        Verbose boolean flag, off (quiet) by default.\n"                 \
  "If p and b are not passed, both pages and posts are considered.\n"

int main(int argc, char *argv[]) {
  int opt;
  char *url = NULL;
  bool pages = false, blogs = false;

  if (argc < 2) {
    logger(LOG_INFO, "MAIN", "No arguments; showing usage.");
    fprintf(stderr, "%s", USAGE);
    return EXIT_SUCCESS;
  }

  // https://www.man7.org/linux/man-pages/man3/getopt.3.html
  while ((opt = getopt(argc, argv, "hvpbu:")) != -1) {
    switch (opt) {
    case 'u':
      url = optarg;
      break;

    case 'p':
      pages = true;
      break;

    case 'b':
      blogs = true;
      break;

    case 'v':
      current_log_level = LOG_INFO;
      break;

    default: // considering h will fall here too.
      logger(LOG_INFO, "MAIN", "Showing usage.");
      fprintf(stderr, "%s", USAGE);
      return EXIT_SUCCESS;
    }
  }

  if ((url == NULL) || (is_url_valid(url) == 0)) {
    logger(LOG_ERROR, "INPUT", "Invalid URL: %s", url ? url : "(null)");
    fprintf(stderr, "%s", USAGE);
    return EXIT_FAILURE;
  }

  logger(LOG_INFO, "MAIN", "Welcome to wpllm.c!");

  logger(LOG_INFO, "INPUT", "URL entered is %s", url);
  bool both = (pages == blogs); /* neither or both => both */
  if (both)
    logger(LOG_INFO, "INPUT", "Parsing both pages and blog posts");
  else if (pages)
    logger(LOG_INFO, "INPUT", "Parsing only pages");
  else
    logger(LOG_INFO, "INPUT", "Parsing only blog posts");

  /*
   * WordPress exposes a fixed REST path on every site: /wp-json/wp/v2/posts
   * (and /wp-json/wp/v2/pages). The base URL is the site; the path is standard.
   */
  cJSON *json = NULL;

  if (pages && !blogs) {
    /* Pages only */
    char *raw = make_curl_request_pages(url);
    if (strcmp(raw, "ERROR") == 0) {
      logger(LOG_ERROR, "CURL",
             "Either invalid URL or not a WordPress site. Request failed.");
      exit(EXIT_FAILURE);
    }
    json = filter_wp_pages(raw);
    free(raw);
  } else if (blogs && !pages) {
    /* Posts only */
    char *raw = make_curl_request_posts(url);
    if (strcmp(raw, "ERROR") == 0) {
      logger(LOG_ERROR, "CURL",
             "Either invalid URL or not a WordPress site. Request failed.");
      exit(EXIT_FAILURE);
    }
    json = filter_wp_posts(raw);
    free(raw);
  } else {
    /* Both: fetch pages and posts, merge */
    char *raw_pages = make_curl_request_pages(url);
    if (strcmp(raw_pages, "ERROR") == 0) {
      logger(LOG_ERROR, "CURL",
             "Either invalid URL or not a WordPress site. Request failed.");
      exit(EXIT_FAILURE);
    }
    cJSON *pages_json = filter_wp_pages(raw_pages);
    free(raw_pages);

    char *raw_posts = make_curl_request_posts(url);
    cJSON *posts_json = NULL;
    if (strcmp(raw_posts, "ERROR") != 0) {
      posts_json = filter_wp_posts(raw_posts);
      free(raw_posts);
    } else {
      logger(LOG_INFO, "CURL", "Posts request failed; including pages only.");
      posts_json = cJSON_CreateArray();
    }

    if (!pages_json || !posts_json) {
      if (pages_json)
        cJSON_Delete(pages_json);
      if (posts_json)
        cJSON_Delete(posts_json);
      logger(LOG_ERROR, "PARSE", "Failed to parse JSON.");
      exit(EXIT_FAILURE);
    }
    json = merge_item_arrays(pages_json, posts_json);
    cJSON_Delete(pages_json);
    cJSON_Delete(posts_json);
    if (!json) {
      logger(LOG_ERROR, "PARSE", "Failed to merge results.");
      exit(EXIT_FAILURE);
    }
  }

  if (!json) {
    logger(LOG_ERROR, "PARSE", "Failed to parse JSON.");
    exit(EXIT_FAILURE);
  }

  write_llm_file("llm.md", json);
  cJSON_Delete(json);
  logger(LOG_INFO, "OUTPUT", "Wrote llm.md");
  return EXIT_SUCCESS;
}
