BUILD_DIR := build
BUILD32_DIR := build32

DISTRO_ID := $(shell if [ -e /etc/os-release ] ; then source /etc/os-release; echo $$ID; fi)

ifeq ($(DISTRO_ID),debian)
	LIB32_DIR := lib/i386-linux-gnu
else
	LIB32_DIR := lib32
endif

.PHONY: build
build: config
	ninja -C $(BUILD_DIR)

.PHONY: build-lib32
build-lib32: config-lib32
	ninja -C $(BUILD32_DIR)

.PHONY: config
config:
	meson setup --prefix "$(HOME)/.local" --buildtype=release $(BUILD_DIR)

.PHONY: config-lib32
config-lib32:
	ASFLAGS=--32 CFLAGS=-m32 CXXFLAGS=-m32 PKG_CONFIG_PATH=/usr/$(LIB32_DIR)/pkgconfig \
		meson setup --prefix "$(HOME)/.local" --buildtype=release libdir=$(LIB32_DIR) $(BUILD32_DIR)

.PHONY: install
install: build
	meson install -C "$(BUILD_DIR)" --skip-subprojects

.PHONY: install-lib32
install-lib32: build-lib32
	meson install -C "$(BUILD32_DIR)" --skip-subprojects

.PHONY: tests
tests: config
	meson test -C build --suite vkShade

.PHONY: uninstall
uninstall:
	ninja uninstall -C "$(BUILD_DIR)"

.PHONY: uninstall-lib32
uninstall-lib32:
	ninja uninstall -C "$(BUILD32_DIR)"

.PHONY: test
test: install
	ENABLE_VKSHADE=1 VKSHADE_LOG_LEVEL=trace vkcube

.PHONY: clean
clean:
	rm -rf $(BUILD_DIR)
	rm -rf $(BUILD32_DIR)
