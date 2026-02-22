#include <curl/curl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define USAGE                                                                  \
  "usage:\n"                                                                   \
  " wpllm [options]\n"                                                         \
  "options:\n"                                                                 \
  " -u [url]  The wordpress site URL to prepare llms.txt (website examples: "  \
  "https://wordpress.org/showcase)\n"                                          \
  " -h        Show this help message\n"

int is_url_valid(const char *url) {
  CURLU *u = curl_url();
  CURLUcode code = curl_url_set(u, CURLUPART_URL, url, 0);
  curl_url_cleanup(u);
  return (code == CURLUE_OK);
}

int main(int argc, char *argv[]) {
  int opt;
  char *url = NULL;

  if (argc < 2) {
    fprintf(stderr, "%s", USAGE);
    return EXIT_SUCCESS;
  }

  printf("Welcome to wpllm.c!\n");
  while ((opt = getopt(argc, argv, "hu:")) != -1) {
    switch (opt) {
    case 'u':
      url = optarg;
      break;
    default:
      fprintf(stderr, "%s", USAGE);
      return EXIT_SUCCESS;
    }
  }

  if ((url == NULL) || (is_url_valid(url) == 0)) {

    fprintf(stderr, "The URL %s is invalid, kindly follow usage below:\n%s",
            url, USAGE);
    return EXIT_FAILURE;
  }

  printf("\n entered: %s", url);
}
