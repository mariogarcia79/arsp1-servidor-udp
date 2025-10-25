CC      := gcc
CFLAGS  := -Wall -Wextra -Iinclude
LDFLAGS := 

SRC_DIR := src
OBJ_DIR := obj
BIN_DIR := bin

TARGET  := $(BIN_DIR)/qotd-udp-server-g201

SRC := $(wildcard $(SRC_DIR)/**/*.c $(SRC_DIR)/*.c)
OBJ := $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(SRC))

all: $(TARGET)

$(TARGET): $(OBJ) | $(BIN_DIR)
	@echo "Linking $@"
	$(CC) $(OBJ) -o $@ $(LDFLAGS)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	@echo "Compiling $<"
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(BIN_DIR) $(OBJ_DIR):
	@mkdir -p $@

clean:
	@echo "Cleaning build files..."
	rm -rf $(OBJ_DIR) $(BIN_DIR)

.PHONY: all clean
