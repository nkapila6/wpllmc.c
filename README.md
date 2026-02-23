# wpllm.c

A C program that generates llm.md for any Wordpress site. Purely human written. :)
Meant to be used as an extension for my `wp-content-engine` agent that will invoke this tool via a ctypes wrapper (or CLI..?)

Written and built on a M1 Pro.

## Overall flow

The C binary will take command line arguments of the URL, posts or pages, output as llm.md.

## Dependencies

- curl and libcurl: <https://ec.haxx.se/install/index.html>
- glibc (of course...)
- cJSON: <https://github.com/DaveGamble/cJSON>
- **HTML → Markdown:** [markitdown](https://github.com/microsoft/markitdown) — install the CLI (e.g. `uv tool install 'markitdown[all]'`) so it’s on your `PATH`.
