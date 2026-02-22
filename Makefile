all: clean wpllm wpllm-test

wpllm: main.c
	gcc main.c -o wpllm

wpllm-test: wpllm
	./wpllm -h

.PHONY: clean

clean:
	rm -f wpllm
