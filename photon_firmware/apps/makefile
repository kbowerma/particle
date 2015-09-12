TOOLCHAIN := /Users/kylebowerman/Documents/particle/particle_firmware/firmware/

PARTICLE_USB := $(shell compgen -f -- "/dev/tty.usbmodem")

make:
	@echo Use make in the app directory

clean:
	-rm -dr out

cleanfirmware:
	cd $(TOOLCHAIN); \
	make clean PARTICLE_DEVELOP=1
	cd $(TOOLCHAIN)/main; \
	make clean PARTICLE_DEVELOP=1

cleanall: clean cleanfirmware

build:
	cd $(TOOLCHAIN)/main; \
	make PLATFORM=photon PARTICLE_DEVELOP=1 APPDIR=$(mkfile_dir) TARGET_DIR=$(mkfile_dir)/out/ TARGET_FILE=/$(APPNAME)
	@echo
	@echo Build complete: $(mkfile_dir)out/$(APPNAME).bin

flash:	dfu
	-stty -f $(PARTICLE_USB) 14400
	particle flash --usb out/$(APPNAME).bin	

dfu:
	-stty -f $(PARTICLE_USB) 14400
