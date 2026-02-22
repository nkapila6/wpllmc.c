all: clean wpllm wpllm-test

wpllm: main.c
	gcc main.c -o wpllm -lcurl

wpllm-test: wpllm
	./wpllm -h

.PHONY: clean

clean:
	rm -f wpllm
