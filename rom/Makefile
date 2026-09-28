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
OBJS    := $(BUILD)/crt0.o $(SRCS:src/%.c=$(BUILD)/%.o) $(BUILD)/hw_assets.o $(BUILD)/font_data.o
ART     := $(wildcard $(TOOLS)/*.py)

all: $(BUILD)/$(NAME).gba

$(BUILD)/hw_assets.c $(BUILD)/hw_assets.h: $(ART)
	@mkdir -p $(BUILD)
	cd $(TOOLS) && $(PYTHON) export_hwcheck.py $(abspath $(BUILD))

$(BUILD)/font_data.c $(BUILD)/font_data.h: $(TOOLS)/font.py $(TOOLS)/export_font.py
	@mkdir -p $(BUILD)
	cd $(TOOLS) && $(PYTHON) export_font.py $(abspath $(BUILD))

$(BUILD)/%.o: src/%.c $(BUILD)/hw_assets.h $(BUILD)/font_data.h $(wildcard src/*.h)
	@mkdir -p $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/hw_assets.o: $(BUILD)/hw_assets.c
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/font_data.o: $(BUILD)/font_data.c
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
