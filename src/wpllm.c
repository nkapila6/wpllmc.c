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
      fprintf(stderr, "%s", USAGE);
      return EXIT_SUCCESS;
    }
  }

  if ((url == NULL) || (is_url_valid(url) == 0)) {
    fprintf(stderr, "The URL %s is invalid, kindly follow usage below:\n%s",
            url, USAGE);
    return EXIT_FAILURE;
  }

  printf("Welcome to wpllm.c!\n");

  logger(LOG_INFO, "INPUT", "URL entered is %s", url);
  if ((pages == 1 && blogs == 1) || (pages == 0 && blogs == 0))
    logger(LOG_INFO, "INPUT", "Parsing both pages and blog posts");
  else if (pages == 1)
    logger(LOG_INFO, "INPUT", "Parsing only pages");
  else if (blogs == 1)
    logger(LOG_INFO, "INPUT", "Parsing only blog posts");

  char *raw_response = make_curl_request(url);
  if (strcmp(raw_response, "ERROR") == 0) {
    logger(LOG_ERROR, "CURL",
           "Either invalid URL or not a Wordpress blog. Cannot perform a "
           "request using CURL.");
    exit(EXIT_FAILURE);
  }

  // parse json
  cJSON *json = filter_wp_pages(raw_response);
  free(raw_response); // freeing chunk.data
}
