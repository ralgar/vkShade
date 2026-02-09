<p align="center">
  <img src=".github/svg/logo-with-text.svg" width="512">
  <br><br>
  <img src="https://img.shields.io/github/v/tag/ralgar/vkShade?color=blue&label=Release">
  <img src="https://img.shields.io/github/issues/ralgar/vkShade?label=Issues&color=indigo">
  <img src="https://img.shields.io/github/issues-pr-closed/ralgar/vkShade?label=Pull%20Requests&color=indigo">
  <img src="https://img.shields.io/badge/License-BSD_2--clause-red?logo=freebsd&logoColor=red">
  <img src="https://img.shields.io/github/stars/ralgar/vkShade?style=flat&logo=github&color=gold&label=Stars">
</p>

<p align="center">
<b>VKSHADE IS BACK IN A BIG WAY!</b>
</p>

**What happened to the original vkShade project?**

When I first started the project, I quickly realized that I was in over my
 head trying to learn C++ and Vulkan at the same time. I also came to the
 conclusion that it would be easier to start from scratch rather than attempt
 to refactor the vkBasalt codebase. Over the past 2 years I've grown much more
 comfortable with both C++ and Vulkan, enough that I'm able to revive this
 project and deliver on my original vision!

## Overview

vkShade is a Vulkan post-processing layer for improving the visuals of games.
 It is being written from the ground up, with the goal of being a powerful,
 modern, and maintainable post-processing layer for Linux gaming.

**DISCLAIMER:** As with any software that hooks into the graphics pipeline,
 this project may trigger bans from anti-cheat software. Use it with
 multiplayer games at your own risk.

## Project Status

This project is still in the **ALPHA** phase of development - meaning
 it works, but it's really just a proof-of-concept right now. Over time, I
 intend to bring it up to speed as the de facto post-processing layer for
 Vulkan applications on Linux.

### Current Features

- [x] Post-processing pipeline (single shader pass)
- [x] Interactive ImGui overlay with mouse support
- [x] Cross-platform input (Wayland, Xlib, XCB)
- [x] Modern C++ RAII wrappers around the Vulkan C API
- [x] Demo shader effect (grid overlay)

<p align="center">
  <a href="https://raw.githubusercontent.com/ralgar/vkShade/assets/screenshots/v0.0.1-demo.png">
    <img src="https://raw.githubusercontent.com/ralgar/vkShade/assets/screenshots/v0.0.1-demo.png" width="800" alt="vkShade in action">
  </a>
  <br>
  <em>vkShade overlay running in-game (click to enlarge)</em>
</p>

### Roadmap

#### Near-term (v0.1.x)

- [x] Ping-pong rendering for multiple effects
- [ ] Multi-pass effect support (shader chaining)
- [ ] Shader reflection for auto-generating UI controls
- [ ] Configuration system (save/load settings)
- [ ] Hot-reload shaders and configuration

#### Mid-term (v0.2.x+)

- [ ] ReShade FX compiler and configuration support
- [ ] libshaderc integration (GLSL support)
- [ ] Depth buffer access
- [ ] Effect presets/profiles
- [ ] Per-game configurations

#### Long-term

- [ ] Community effect repository
- [ ] Performance profiling tools

## Packaging Status

No distro packages are available currently, but this will probably change in
 the future.

For now, you can build it from source very easily.

## Building from Source

vkShade provides a full-featured and easy to use build system, which will
 build and install the layer in your user's home directory. It can also
 uninstall the layer, should you decide that you don't want to use it anymore.

**NOTE:** You should prefer using distro-provided packages, if available.

### Dependencies

Before building, you will need:

- GCC >= 9
- X11 headers (`libx11`, `libxcb`)
- Wayland headers (`wayland-client`)
- xkbcommon headers (`libxkbcommon`)
- glslc
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

## Usage

Enable vkShade by setting the environment variable.

### Standard

When using the terminal or an application (.desktop) file, execute:

```
ENABLE_VKSHADE=1 yourgame
```

### Lutris

With Lutris, follow these steps below:

1. Right click on a game, and press `configure`.
1. Go to the `System options` tab and scroll down to `Environment variables`.
1. Press on `Add`, and add `ENABLE_VKSHADE` under `Key`, and add `1` under `Value`.

### Steam

With Steam, edit your game's launch options and add:
```
ENABLE_VKSHADE=1 %command%
```

### Controls

**NOTE:** Keybinds are currently hardcoded.

- Press `F2` to toggle the GUI overlay
- Click to interact with GUI windows when visible
- Press `HOME` to toggle the effects

Customizable keybinds will be added in a future release.

### Log Output

The log output level can be set with the `VKSHADE_LOG_LEVEL` environment var,
 e.g. `VKSHADE_LOG_LEVEL=debug`. Possible values are:
 `trace`, `debug`, `info`, `warn`, `error`, `critical`, and `off`.

By default the logger outputs to stderr, a file can be set as the output using
 the `VKSHADE_LOG_FILE` env var, e.g. `VKSHADE_LOG_FILE="vkShade.log"`.

## FAQ

**Does vkShade work with DXVK and VKD3D?**

Yes. It should work with anything that uses the Vulkan API. If you find that
 vkShade doesn't work with a Vulkan application, please open an issue.

**Will vkShade get me banned?**

Quite possibly. Other software with similar functionality have been known to
 trigger bans, so it is likely that vkShade will as well. It is strongly
 recommended that you only use vkShade with single-player games.

**Will there be an OpenGL version?**

No, however you can probably use Zink (OpenGL to Vulkan translation) and use
 vkShade that way.

## Acknowledgments

A big thank you to these amazing projects, without which vkShade wouldn't be
 possible:

- [vkBasalt](https://github.com/DadSchoorse/vkBasalt)
- [ReShade](https://github.com/crosire/reshade)

## License

Copyright (c) 2026 Ryan Algar
 ([ralgar/vkShade](https://github.com/ralgar/vkShade))

BSD 2-clause License (see [LICENSE](LICENSE) or
 [BSD 2-clause](https://choosealicense.com/licenses/bsd-2-clause/))
