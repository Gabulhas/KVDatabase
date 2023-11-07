TARGET_EXEC ?= a.out

BUILD_DIR ?= ./build
SRC_DIRS ?= ./src

SRCS := $(shell find $(SRC_DIRS) -name *.c)
OBJS := $(SRCS:%=$(BUILD_DIR)/%.o)
DEPS := $(OBJS:.o=.d)

INC_DIRS := $(shell find $(SRC_DIRS) -type d)
INC_FLAGS := $(addprefix -I,$(INC_DIRS))

CPPFLAGS ?= $(INC_FLAGS) -MMD -MP

# CFLAGS ?=  -Wall -O3 -g
CFLAGS ?=  -Wall -g

CC := clang

$(BUILD_DIR)/$(TARGET_EXEC): $(OBJS)
	$(CC) $(OBJS) -o $@ $(LDFLAGS)

# c source
$(BUILD_DIR)/%.c.o: %.c
	$(MKDIR_P) $(dir $@)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

.PHONY: all clean tests lldb_test valgrind_test

all: $(BUILD_DIR)/$(TARGET_EXEC)

clean:
	$(RM) -r $(BUILD_DIR)

# New testing targets
tests: lldb_test valgrind_test

lldb_test: clean all
	@echo "Testing with LLDB..."
	@lldb ./$(BUILD_DIR)/$(TARGET_EXEC) -o "run"
	@echo "LLDB testing completed."

valgrind_test: clean all
	@echo "Testing with Valgrind..."
	@valgrind --leak-check=full ./$(BUILD_DIR)/$(TARGET_EXEC)
	@echo "Valgrind testing completed."

-include $(DEPS)

MKDIR_P ?= mkdir -p
