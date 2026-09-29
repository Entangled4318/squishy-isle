# Squishy Isle ROM build.
#   make          build build/squishy_isle.gba
#   make test     run the headless mGBA test scripts in ../test
#   make clean

NAME    := squishy_isle
PREFIX  := arm-none-eabi-
CC      := $(PREFIX)gcc
OBJCOPY := $(PREFIX)objcopy
PYTHON  := python3
TOOLS   := ../tools
BUILD   := build

ARCH    := -mcpu=arm7tdmi -mtune=arm7tdmi -mthumb -mthumb-interwork
CFLAGS  := $(ARCH) -O2 -std=gnu11 -Wall -Wextra -Wno-unused-parameter \
           -ffunction-sections -fdata-sections -fno-strict-aliasing \
           -Isrc -I$(BUILD)
ASFLAGS := -mcpu=arm7tdmi -mthumb-interwork
LDFLAGS := $(ARCH) -nostartfiles -T gba.ld -Wl,--gc-sections \
           -Wl,-Map,$(BUILD)/$(NAME).map --specs=nano.specs --specs=nosys.specs

SRCS    := $(wildcard src/*.c)
GEN     := hw_assets font_data game_assets music_data
OBJS    := $(BUILD)/crt0.o $(SRCS:src/%.c=$(BUILD)/%.o) $(GEN:%=$(BUILD)/%.o)
ART     := $(wildcard $(TOOLS)/*.py)

ifeq ($(filter grouped-target,$(.FEATURES)),)
$(error GNU Make 4.3 or newer is needed (grouped targets, &:))
endif

all: $(BUILD)/$(NAME).gba

# Each generator writes its .c and .h in one run. "&:" (grouped targets)
# tells make so: with a plain rule, make -j ran every generator twice at
# the same time, both writing the same files.

$(BUILD)/hw_assets.c $(BUILD)/hw_assets.h &: $(ART)
	@mkdir -p $(BUILD)
	cd $(TOOLS) && $(PYTHON) export_hwcheck.py $(abspath $(BUILD))

$(BUILD)/font_data.c $(BUILD)/font_data.h &: $(TOOLS)/font.py $(TOOLS)/export_font.py
	@mkdir -p $(BUILD)
	cd $(TOOLS) && $(PYTHON) export_font.py $(abspath $(BUILD))

$(BUILD)/game_assets.c $(BUILD)/game_assets.h &: $(ART)
	@mkdir -p $(BUILD)
	cd $(TOOLS) && $(PYTHON) export_game.py $(abspath $(BUILD))

$(BUILD)/music_data.c $(BUILD)/music_data.h &: $(TOOLS)/music.py
	@mkdir -p $(BUILD)
	cd $(TOOLS) && $(PYTHON) music.py $(abspath $(BUILD))

$(BUILD)/%.o: src/%.c $(GEN:%=$(BUILD)/%.h) $(wildcard src/*.h)
	@mkdir -p $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

$(GEN:%=$(BUILD)/%.o): $(BUILD)/%.o: $(BUILD)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/crt0.o: src/crt0.s
	@mkdir -p $(BUILD)
	$(CC) $(ASFLAGS) -c $< -o $@

$(BUILD)/$(NAME).elf: $(OBJS) gba.ld
	$(CC) $(LDFLAGS) $(OBJS) -o $@

$(BUILD)/$(NAME).gba: $(BUILD)/$(NAME).elf $(TOOLS)/gbafix.py
	$(OBJCOPY) -O binary $< $@
	$(PYTHON) $(TOOLS)/gbafix.py $@

test: $(BUILD)/$(NAME).gba
	$(MAKE) -C ../test run ROM=$(abspath $(BUILD)/$(NAME).gba)

clean:
	rm -rf $(BUILD)

.PHONY: all test clean
