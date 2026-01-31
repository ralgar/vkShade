BUILD_DIR := build
BUILD32_DIR := build32

DISTRO_ID := $(shell if [ -e /etc/os-release ] ; then source /etc/os-release; echo $$ID; fi)

ifeq ($(DISTRO_ID),debian)
	LIB32_DIR := lib/i386-linux-gnu
else
	LIB32_DIR := lib32
endif

ifdef XDG_CONFIG_HOME
	CONFIG_DIR := $(XDG_CONFIG_HOME)/vkShade
else
	CONFIG_DIR := $(HOME)/.config/vkShade
endif

ifdef XDG_DATA_HOME
    VK_LAYER_DIR := $(XDG_DATA_HOME)/vulkan/implicit_layer.d
else
    VK_LAYER_DIR := $(HOME)/.local/share/vulkan/implicit_layer.d
endif

.PHONY: build
build: config
	ninja -C $(BUILD_DIR)

.PHONY: build-lib32
build-lib32: config-lib32
	ninja -C $(BUILD32_DIR)

.PHONY: config
config:
	meson setup --buildtype=release $(BUILD_DIR)

.PHONY: config-lib32
config-lib32:
	ASFLAGS=--32 CFLAGS=-m32 CXXFLAGS=-m32 PKG_CONFIG_PATH=/usr/$(LIB32_DIR)/pkgconfig \
		meson setup --buildtype=release --libdir=$(LIB32_DIR) $(BUILD32_DIR)

.PHONY: install
install: build
	mkdir -p "$(VK_LAYER_DIR)" "$(HOME)/.local/lib"
	cp $(BUILD_DIR)/config/vkShade.json "$(VK_LAYER_DIR)"
	sed -i "s|libvkshade.so|$(HOME)/.local/lib/libvkshade.so|" "$(VK_LAYER_DIR)/vkShade.json"
	cp $(BUILD_DIR)/src/libvkshade.so "$(HOME)/.local/lib"

.PHONY: install-lib32
install-lib32: build-lib32
	mkdir -p "$(VK_LAYER_DIR)" "$(HOME)/.local/lib32"
	cp $(BUILD32_DIR)/config/vkShade.json "$(VK_LAYER_DIR)/vkShade.x86.json"
	sed -i "s|libvkshade.so|$(HOME)/.local/lib32/libvkshade.so|" \
		"$(VK_LAYER_DIR)/vkShade.x86.json"
	cp $(BUILD32_DIR)/src/libvkshade.so "$(HOME)/.local/lib32"

.PHONY: uninstall
uninstall:
	rm -f "$(VK_LAYER_DIR)/vkShade.json"
	rm -f "$(HOME)/.local/lib/libvkshade.so"

.PHONY: uninstall-lib32
uninstall-lib32:
	rm -f "$(VK_LAYER_DIR)/vkShade.x86.json"
	rm -f "$(HOME)/.local/lib32/libvkshade.so"

.PHONY: test
test: install
	ENABLE_VKSHADE=1 VKSHADE_LOG_LEVEL=trace vkcube

.PHONY: clean
clean:
	rm -rf $(BUILD_DIR)
	rm -rf $(BUILD32_DIR)
