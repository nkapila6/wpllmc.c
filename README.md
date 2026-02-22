# wpllm.c

A C program that generates llm.txt or llm.md for any Wordpress site. Purely human written. :)
Meant to be used as an extension for my `wp-content-engine` agent that will invoke this tool via a ctypes wrapper.

Written and built on a M1 Pro.

## Overall flow

The C binary will take command line arguments of the URL, posts or pages, output as llms.txt or llms.md.

## Dependencies

- curl and libcurl: <https://ec.haxx.se/install/index.html>
- glibc (of course...)
- cJSON: <https://github.com/DaveGamble/cJSON>
