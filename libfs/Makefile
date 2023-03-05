# Target library
lib := libfs.a

# Application objects to compile
objs := \
	disk.o \
	fs.o 

# Don't print the commands unless explicitly requested with `make V=1`
ifneq ($(V),1)
Q = @
endif

# Define compilation toolchain
CC := gcc
## Markdown specific compilation toolchain
PANDOC += pandoc

# General gcc options
CFLAGS := -Wall -Wextra -Werror
## Debug flag
ifneq ($(D),1)
CFLAGS	+= -O2
else
CFLAGS	+= -g
endif
## Dependency generation
CFLAGS	+= -MMD

# Default rule
all: $(lib)

# Dependency Tracking
deps := $(patsubst %.o, %.d, $(objs))
-include $(deps)

# Rule for libfs.a
libfs.a: $(objs)
	@echo "MAKE $@"
	$(Q)ar rcs $@ $^

# Generic rule for compiling objects
%.o: %.c 
	@echo "CC $@"
	$(Q)$(CC) $(CFLAGS) -c -o $@ $<

# Generic rule for converting markdown files
%.html: %.md 
	@echo "MD $@"
	$(Q)$(PANDOC) -o $@ $<

# Cleaning rule
clean:
	@echo "CLEAN	$(CUR_PWD)"
	$(Q)rm -f $(objs) $(deps) $(lib)
