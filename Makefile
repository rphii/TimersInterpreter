#### Start of system configuration section. ####

GIT_VERSION := "$(shell git describe --abbrev=4 --dirty --always --tags)"
CC      := gcc
CFLAGS  := -Wall -Wextra -Wimplicit \
		   -Wno-unused-parameter \
		   -Wconversion \
		   -O3 -march=native \
		   ##############\
		   -rdynamic -Og -ggdb3 \
		   -fsanitize=address -rdynamic \
		   -DCOLORPRINT_DISABLE \
		   -pg -rdynamic \
		   -O2 \
		   -O0 -g \
		   -DCOLORPRINT_DISABLE \
		   -Werror \
		   -Warith-conversion \
		   -pg -O2 \
		   -DNDEBUG \
		   -march=native \
		   -Og -rdynamic \
		   -march=native \
		   -Wall \

LDFLAGS := \
		   -lm \
		   ##############\
		   -fsanitize=address -rdynamic \
		   -rdynamic -ggdb3 \
		   -pg -rdynamic \
		   -fsanitize=address \
		   -pg \
		   -rdynamic -Og -ggdb3
CSUFFIX	:= .c
HSUFFIX	:= .h
PREFIX ?= /usr

#### End of system configuration section. ####

APPNAME = timers

# Path for important files
# .c and .h files
SRC_DIR = src
# .o files
OBJ_DIR = obj
# target directory
TRG_DIR = .

.PHONY: all clean


# Files to compile
TARGET  := $(addprefix $(TRG_DIR)/,$(APPNAME))
C_FILES := $(wildcard $(SRC_DIR)/*$(CSUFFIX))
O_FILES := $(addprefix $(OBJ_DIR)/,$(notdir $(C_FILES:$(CSUFFIX)=.o)))
D_FILES := $(addprefix $(OBJ_DIR)/,$(notdir $(C_FILES:$(CSUFFIX)=.d)))

all: $(TARGET)

install: $(TARGET)
	@mkdir -p $(DESTDIR)$(PREFIX)/bin
	@cp -p $(TARGET) $(DESTDIR)$(PREFIX)/bin/${APPNAME}

uninstall:
	@rm -rf $(DESTDIR)$(PREFIX)/bin/${APPNAME}

# link all .o files
$(TARGET): $(O_FILES) | $(TRG_DIR)
	@echo link    : $@ #$^
	@$(CC) $(LDFLAGS) -o $@ $^

# depend include files
-include $(D_FILES)

# compile all .c Files
$(OBJ_DIR)/%.o: $(SRC_DIR)/%$(CSUFFIX) Makefile | $(OBJ_DIR)
	@echo compile : $<
	@$(CC) $(CFLAGS) -c -DVERSION=\"$(GIT_VERSION)\" -MMD -o $@ $<

# create directories if they don't exist
# .o dir
$(OBJ_DIR):
	@mkdir $@
# target dir
$(TRG_DIR):
	@mkdir $@

#### CLEANING ####

ifeq ($(OS),Windows_NT)
# Cleaning rules for Windows OS
clean:
	@del /q $(OBJ_DIR), $(TRG_DIR)
	@rmdir $(OBJ_DIR)
	@rmdir $(TRG_DIR)
else
# Cleaning rules for Unix-based OS (no clue if this works)
clean:
	@rm -rf $(OBJ_DIR) $(TRG_DIR) $(TARGET)
endif

