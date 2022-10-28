# output files
TARGET_SERVER ?= jackudpsrv
TARGET_CLIENT ?= jackudpcli

# shared settings
BUILD_DIR ?= ./build
BINARY_DIR ?= ./
INC_DIRS ?= ./

LDLIBS   := -ljack -lm
UNIFLAGS ?= -O0 -Wall -Wextra -fsanitize=undefined -fsanitize=address
CMPFLAGS ?= -MMD -MP -std=gnu11 $(UNIFLAGS) $(INC_FLAGS)
LDCFLAGS ?= $(UNIFLAGS) $(LDLIBS)

# unique settings
SRCS_SERVER := server.c shared.c resample.c
SRCS_CLIENT := client.c shared.c resample.c

# generated
OBJS_SERVER := $(SRCS_SERVER:%=$(BUILD_DIR)/%.o)
DEPS_SERVER := $(OBJS_SERVER:.o=.d)
OBJS_CLIENT := $(SRCS_CLIENT:%=$(BUILD_DIR)/%.o)
DEPS_CLIENT := $(OBJS_CLIENT:.o=.d)
INC_FLAGS := $(addprefix -I,$(shell find $(INC_DIRS) -type d))

# targets
.PHONY: all clean

all : $(TARGET_SERVER) $(TARGET_CLIENT)

clean:
	rm -rf $(BUILD_DIR)

$(BINARY_DIR)/$(TARGET_SERVER): $(OBJS_SERVER)
	$(MKDIR_P) $(dir $@)
	$(CC) $(LDCFLAGS) $^ -o $@ 

$(BINARY_DIR)/$(TARGET_CLIENT): $(OBJS_CLIENT)
	$(MKDIR_P) $(dir $@)
	$(CC) $(LDCFLAGS) $^ -o $@ 

$(BUILD_DIR)/%.c.o: %.c
	$(MKDIR_P) $(dir $@)
	$(CC) $(CMPFLAGS) -c $< -o $@

-include $(DEPS_SERVER)
-include $(DEPS_CLIENT)
MKDIR_P ?= mkdir -p
