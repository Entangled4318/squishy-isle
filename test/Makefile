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
	python3 check_map.py $(OUT)
	python3 make_save.py $(OUT)/step2.sav $$(seq -s, 20 39)
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
	./$(HARNESS) $(ROM) $(OUT)/title.sav title_new.txt $(OUT) > $(OUT)/title_new.log
	cp $(OUT)/boxes.sav $(OUT)/reset.sav
	./$(HARNESS) $(ROM) $(OUT)/reset.sav title_reset.txt $(OUT) > $(OUT)/title_reset.log
	./$(HARNESS) $(ROM) $(OUT)/reset.sav boot_only.txt $(OUT) > $(OUT)/reset_reboot.log
	python3 check_step3.py $(OUT)
	python3 check_title.py $(OUT)
	python3 make_save.py $(OUT)/pen.sav 0,1,3,6,8,11,12,15,17,19 17,6,12
	./$(HARNESS) $(ROM) $(OUT)/pen.sav pen.txt $(OUT) > $(OUT)/pen.log
	python3 check_pen.py $(OUT)
	python3 make_save.py $(OUT)/pick.sav 0,1,3,6,8,11,12,15,17,19 17,6,12
	./$(HARNESS) $(ROM) $(OUT)/pick.sav pick.txt $(OUT) > $(OUT)/pick.log
	python3 make_save.py $(OUT)/top.sav 0,1,3,6,8,11,12,15,17,19 17,6,12
	./$(HARNESS) $(ROM) $(OUT)/top.sav sign_top.txt $(OUT) > $(OUT)/sign_top.log
	python3 check_pick.py $(OUT)
	python3 make_save.py $(OUT)/pen20.sav $$(seq -s, 0 19) 17,6,12
	./$(HARNESS) $(ROM) $(OUT)/pen20.sav pen20.txt $(OUT) > $(OUT)/pen20.log
	python3 check_pen20.py $(OUT) $(ROM:.gba=.elf)
	rm -f $(OUT)/loop.sav
	./$(HARNESS) $(ROM) $(OUT)/loop.sav loop_a.txt $(OUT) > $(OUT)/loop_a.log
	./$(HARNESS) $(ROM) $(OUT)/loop.sav loop_b.txt $(OUT) > $(OUT)/loop_b.log
	./$(HARNESS) $(ROM) $(OUT)/loop.sav loop_c.txt $(OUT) > $(OUT)/loop_c.log
	./$(HARNESS) $(ROM) $(OUT)/loop.sav loop_d.txt $(OUT) > $(OUT)/loop_d.log
	python3 check_loop.py $(OUT) $(ROM:.gba=.elf)
	python3 make_save.py $(OUT)/w9.sav $$(seq -s, 0 8)
	./$(HARNESS) $(ROM) $(OUT)/w9.sav woods_shut.txt $(OUT) > $(OUT)/woods_shut.log
	python3 make_save.py $(OUT)/w10.sav $$(seq -s, 0 9) 0,1,2
	./$(HARNESS) $(ROM) $(OUT)/w10.sav woods.txt $(OUT) > $(OUT)/woods.log
	./$(HARNESS) $(ROM) $(OUT)/w10.sav woods_continue.txt $(OUT) > $(OUT)/woods_continue.log
	python3 check_woods.py $(OUT)
	python3 check_music.py $(OUT) $(ROM)

.PHONY: run
