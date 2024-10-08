CXX := g++
CC := gcc
LD := g++
TARGET_NAME := main

BUILD_DIR := build
BIN_DIR := bin
SRC_DIR := src
LIB_DIR :=

TARGET_EXEC = $(BIN_DIR)/$(TARGET_NAME)

SRCS := $(shell find $(SRC_DIR) -name '*.cc' -or -name '*.c')
OBJS := $(SRCS:%=$(BUILD_DIR)/%.o)
DEPS := $(OBJS:.o=.d)

CXXFLAGS := -Wall -Wpedantic -Wsuggest-override -Werror -MMD -MP -g
CCFLAGS := -Wall -Wpedantic -Werror $(INC_FLAGS) -MMD -MP -g
LDFLAGS :=
LIBFLAGS :=

.PHONY: clean

$(TARGET_EXEC): $(OBJS)
	@mkdir -p $(dir $@)
	$(LD) $(LDFLAGS) -o $@ $^ $(LIBFLAGS)

$(BUILD_DIR)/%.cc.o: %.cc
	@mkdir -p $(dir $@)
	$(CXX) -c $(CXXFLAGS) -o $@ $<

$(BUILD_DIR)/%.c.o: %.c
	@mkdir -p $(dir $@)
	$(CC) -c $(CCFLAGS) -o $@ $<

clean:
	rm -rf $(BUILD_DIR) $(BIN_DIR)

-include $(DEPS)
