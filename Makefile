################################################################################

CC=gcc # define the compiler to use
LD=ld # define the linker to use
SRCS := \
			src/common/debug.c              \
			src/common/dictionary.c         \
			src/common/heap.c               \
			src/common/strsub.c             \
			src/nvram/nvol3.c               \
			src/nvram/nvram.c               \
			src/registry/registry.c         \
			src/registry/registrycmd.c      \
			src/shell/corshell.c            \
			src/shell/corshellcmd.c         \
			test/main.c
CFLAGS=-Os
LDFLAGS=-lpthread --static -Xlinker -Map=output.map -T corshell.ld

TARGET_EXEC ?= nvol2

BUILD_DIR ?= ./build
SRC_DIRS ?= ./src

OBJS := $(SRCS:%=$(BUILD_DIR)/%.o) 
DEPS := $(OBJS:.o=.d)
LDS := 

INC_DIRS := $(shell find $(SRC_DIRS) -type d)
INC_FLAGS := $(addprefix -I,$(INC_DIRS))

CPPFLAGS ?= $(INC_FLAGS) -MMD -MP

$(BUILD_DIR)/$(TARGET_EXEC): $(OBJS)
	$(CC) $(OBJS) $(LDS) -o $@ $(LDFLAGS)

# assembly
$(BUILD_DIR)/%.s.o: %.s
	$(MKDIR_P) $(dir $@)
	$(AS) $(ASFLAGS) -c $< -o $@

# c source
$(BUILD_DIR)/%.c.o: %.c
	$(MKDIR_P) $(dir $@)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

# c++ source
$(BUILD_DIR)/%.cpp.o: %.cpp
	$(MKDIR_P) $(dir $@)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@


.PHONY: clean

clean:
	$(RM) -r $(BUILD_DIR)

-include $(DEPS)

MKDIR_P ?= mkdir -p
