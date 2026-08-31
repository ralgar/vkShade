# Installation

## Packaging Status

vkShade has packages available in the following repositories:

[![Packaging status](https://repology.org/badge/vertical-allrepos/vkshade.svg)](https://repology.org/project/vkshade/versions)

If vkShade is not packaged for your distro, you can
 [build it from source](#building-from-source).

## Building from Source

vkShade provides a full-featured and easy to use build system, which will
 build and install the layer in your user's home directory. It can also
 uninstall the layer, should you decide that you don't want to use it anymore.

**NOTE:** You should prefer using distro-provided packages, if available.

### Dependencies

Before building, you will need:

- GCC >= 12
- X11 development files (`libx11`, `libxcb`)
- Wayland development files (`wayland-client`)
- xkbcommon development files (`libxkbcommon`)
- SPIR-V Headers
- Vulkan Headers
- Vulkan Utility Libraries
- GNU Make
- Meson
- Ninja

### Building

1. Clone the repository, and change directory into it.

   ```
   git clone https://github.com/ralgar/vkShade.git
   cd vkShade
   ```

1. Checkout the latest tagged release.

   ```
   git checkout <version>
   ```

1. Using the included `Makefile`, build and install vkShade.

   ```
   make install
   ```

1. (Optional): Build and install the 32 bit version.

   ```
   make install-lib32
   ```

### Uninstallation

To uninstall, simply run `make uninstall`, or `make uninstall-lib32`.
