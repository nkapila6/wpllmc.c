#include "logger.h"
#include <curl/curl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
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

int is_url_valid(const char *url) {
  CURLU *u = curl_url();
  CURLUcode code = curl_url_set(u, CURLUPART_URL, url, 0);
  curl_url_cleanup(u);
  return (code == CURLUE_OK);
}

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
  // logger();
  printf("\n entered: %s", url);
  printf("\nvalue of pages: %d", pages);
  printf("\nvalue of blog posts: %d", blogs);
}
