# Headless emulator tests. Needs libmgba-dev (apt) and Python with Pillow.
HARNESS := build/harness
ROM ?= ../rom/build/squishy_isle.gba
OUT := build/out

$(HARNESS): harness.c
	@mkdir -p build
	$(CC) -O2 -Wall -o $@ $< -lmgba

run: $(HARNESS)
	@rm -rf $(OUT) && mkdir -p $(OUT)
	./$(HARNESS) $(ROM) $(OUT)/test.sav hwcheck_boot1.txt $(OUT) | tee $(OUT)/boot1.log
	./$(HARNESS) $(ROM) $(OUT)/test.sav hwcheck_boot2.txt $(OUT) | tee $(OUT)/boot2.log
	python3 check_hwcheck.py $(OUT)
	./$(HARNESS) $(ROM) $(OUT)/step2.sav step2.txt $(OUT) | tee $(OUT)/step2.log
	python3 check_step2.py $(OUT)

.PHONY: run
