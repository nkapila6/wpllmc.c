CC = gcc
CFLAGS = -Wall -Wextra
LIBS = -lcurl
SRCS = wpllm.c logger.c
TARGET = wpllm

all: clean $(TARGET) wpllm-test

$(TARGET): $(SRCS)
	$(CC) $(CFLAGS) $(SRCS) -o $(TARGET) $(LIBS)

wpllm-test: $(TARGET)
	./wpllm -h

clean:
	rm -f $(TARGET)
