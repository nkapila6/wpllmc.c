CC = gcc
CFLAGS = -Wall -Wextra -I$(SRC_DIR) -DCJSON_NESTING_LIMIT=10000
LIBS = -lcurl

# dirs
SRC_DIR = src
BUILD_DIR = build
TARGET = wpllm

# srcs
SRCS = $(wildcard $(SRC_DIR)/*.c)

# .o files
OBJS = $(patsubst $(SRC_DIR)/%.c, $(BUILD_DIR)/%.o, $(SRCS))

all: clean $(TARGET) wpllm-test

$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $(TARGET) $(LIBS)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

wpllm-test: $(TARGET)
	rm -rf $(BUILD_DIR)
	./$(TARGET) -h

clean:
	rm -rf $(BUILD_DIR) $(TARGET)

.PHONY: all clean wpllm-test
