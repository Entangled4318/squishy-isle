# Headless emulator tests. Needs libmgba-dev (apt) and Python with Pillow.
HARNESS := build/harness
ROM ?= ../rom/build/squishy_isle.gba
OUT := build/out

UNIT := build/test_collection

$(UNIT): test_collection.c ../rom/src/collection.c ../rom/src/collection.h ../rom/src/save.h
	@mkdir -p build
	$(CC) -O2 -Wall -Wextra -Wno-unused-parameter -Wno-pointer-to-int-cast -o $@ $<

$(HARNESS): harness.c
	@mkdir -p build
	$(CC) -O2 -Wall -o $@ $< -lmgba

run: $(HARNESS) $(UNIT)
	@rm -rf $(OUT) && mkdir -p $(OUT)
	./$(UNIT)
	./$(HARNESS) $(ROM) $(OUT)/test.sav hwcheck_boot1.txt $(OUT) | tee $(OUT)/boot1.log
	./$(HARNESS) $(ROM) $(OUT)/test.sav hwcheck_boot2.txt $(OUT) | tee $(OUT)/boot2.log
	python3 check_hwcheck.py $(OUT)
	./$(HARNESS) $(ROM) $(OUT)/step2.sav step2.txt $(OUT) | tee $(OUT)/step2.log
	python3 check_step2.py $(OUT)
	./$(HARNESS) $(ROM) $(OUT)/game.sav boot_only.txt $(OUT) > $(OUT)/save1.log
	./$(HARNESS) $(ROM) $(OUT)/game.sav boot_only.txt $(OUT) > $(OUT)/save2.log
	python3 make_v1_save.py $(OUT)/v1.sav
	./$(HARNESS) $(ROM) $(OUT)/v1.sav boot_only.txt $(OUT) > $(OUT)/savev1.log
	./$(HARNESS) $(ROM) $(OUT)/v1.sav boot_only.txt $(OUT) > $(OUT)/savev1b.log
	python3 check_save.py $(OUT)
	./$(HARNESS) $(ROM) $(OUT)/boxes.sav step3_boxes.txt $(OUT) > $(OUT)/boxes.log
	./$(HARNESS) $(ROM) $(OUT)/boxes.sav boot_only.txt $(OUT) > $(OUT)/boxes_reboot.log
	python3 check_step3.py $(OUT)

.PHONY: run
