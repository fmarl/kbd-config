CONVERT_TO ?= liatris
QMK_HOME   ?= $(HOME)/qmk_firmware

KEYBOARD   := splitkb/aurora/lily58/rev1
KEYMAP     := emacs58
KEYMAP_DIR := keyboards/splitkb/aurora/lily58/keymaps/$(KEYMAP)
FIRMWARE   := $(subst /,_,$(KEYBOARD))_$(KEYMAP)_$(CONVERT_TO).uf2
SYMBOLS    := $(patsubst images/%.png,$(KEYMAP_DIR)/%_symbol.h,$(wildcard images/*.png))
MOUNT_DIR  := /tmp/rpi-rp2

export QMK_HOME
export QMK_USERSPACE := $(CURDIR)

.PHONY: all setup preview flash clean $(FIRMWARE)

all: $(FIRMWARE)

setup:
	test -d $(QMK_HOME) || \
	    git clone --depth 1 --recurse-submodules --shallow-submodules \
	        https://github.com/qmk/qmk_firmware $(QMK_HOME)

$(KEYMAP_DIR)/%_symbol.h: images/%.png scripts/oled-image.sh scripts/pbm-to-oled.awk
	sh scripts/oled-image.sh $*_symbol $< > $@.tmp
	mv $@.tmp $@

$(FIRMWARE): $(SYMBOLS)
	qmk compile -kb $(KEYBOARD) -km $(KEYMAP) -e CONVERT_TO=$(CONVERT_TO)

flash: $(FIRMWARE)
	@echo "Waiting for RPI-RP2 (double-tap reset on the half to flash)..."
	@until [ -e /dev/disk/by-label/RPI-RP2 ]; do sleep 0.5; done
	sudo mkdir -p $(MOUNT_DIR)
	sudo mount /dev/disk/by-label/RPI-RP2 $(MOUNT_DIR)
	sudo cp $(FIRMWARE) $(MOUNT_DIR)/
	sync
	-sudo umount $(MOUNT_DIR)
	@echo "Done. Repeat for the other half."

clean:
	qmk clean
	rm -rf $(FIRMWARE) preview
