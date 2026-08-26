<p align="center">
  <img src=".github/svg/logo-with-text.svg" width="512">
  <br><br>
  <img src="https://img.shields.io/github/v/tag/ralgar/vkShade?color=blue&label=Release">
  <img src="https://img.shields.io/github/issues/ralgar/vkShade?label=Issues&color=indigo">
  <img src="https://img.shields.io/github/issues-pr-closed/ralgar/vkShade?label=Pull%20Requests&color=indigo">
  <img src="https://img.shields.io/badge/License-BSD_2--clause-red?logo=freebsd&logoColor=red">
  <img src="https://img.shields.io/github/stars/ralgar/vkShade?style=flat&logo=github&color=gold&label=Stars">
</p>

## Overview

vkShade is a ReShade-compatible Vulkan post-processing layer, written from the
 ground up to be a powerful, modern, and maintainable solution for improving
 visuals in games and other Vulkan applications on Linux.

> [!CAUTION]
> This software may trigger bans from anti-cheat. Use it with multiplayer
> games at your own risk.

## Project Status

This project is currently in the **PRE-ALPHA** phase of development. The
 core shader runtime and preset system are functional and increasingly
 feature-complete, while the GUI and other user-facing components are still
 under active development. There may be breaking changes as the project
 continues to mature.

### Current Features

> [!NOTE]
> See also [Limitations](#limitations).

- [x] ReShade FX shader support
- [x] Configuration system with ReShade-like presets and hot-reloading
- [x] Interactive GUI overlay with keyboard and mouse support
- [x] Cross-platform input (Wayland, Xlib, XCB)
- [x] Modern C++ RAII wrappers around the Vulkan C API

<p align="center">
  <a href="https://raw.githubusercontent.com/ralgar/vkShade/assets/screenshots/v0.0.6-demo.png">
    <img src="https://raw.githubusercontent.com/ralgar/vkShade/assets/screenshots/v0.0.6-demo.png" width="800" alt="vkShade in action">
  </a>
  <br>
  <em>vkShade overlay running in-game (click to enlarge)</em>
</p>

### Limitations

Some ReShade effects and preset features do not work properly yet. For example:

- Effects relying on the depth buffer
- Effects with multiple techniques
- ReShade's `Techniques` and `TechniqueSorting` preset parameters
- Possibly more...

### Roadmap

#### Near-term (v0.1.x)

- [x] Chain together multiple effects
- [x] Configuration system
    - [x] Application-level config file
    - [x] Per-game effect presets using ReShade's INI format
    - [x] Hot-reload config/preset files on changes
- [x] GUI overlay
    - [x] Browse, apply, and re-order effects
    - [x] Save and reload preset
    - [ ] Uniform enumeration and adjustment (in-progress)
- [x] ReShade FX integration
    - [x] Basic ReShade FX support
    - [x] Full support for ReShade's time-based runtime uniforms
    - [x] Stub support for ReShade's other runtime uniforms
    - [x] Image and sampler reflection
    - [x] Support for effects with multiple passes
    - [x] Pipeline state reflection (stencil and blending)
- [ ] Various fixes and improvements

#### Mid-term (v0.2.x+)

- [ ] Support for effects with multiple techniques
- [ ] Full support for ReShade's input-based runtime uniforms
- [ ] Full support for ReShade's overlay-based runtime uniforms
- [ ] Hot-reloadable effects
- [ ] Depth buffer access

#### Long-term

- [ ] Community effect repository
- [ ] Performance profiling tools

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

## Configuration

The `vkShade.ini` configuration file is searched for in the following
 locations, in order of precedence:

- A file set with the environment variable
  `VKSHADE_CONFIG_FILE=/path/to/vkShade.ini`
- A file named `vkShade.ini` in the working directory of the game
- `$XDG_CONFIG_HOME/vkShade/vkShade.ini` or `~/.config/vkShade/vkShade.ini`
- `$XDG_DATA_HOME/vkShade/vkShade.ini` or `~/.local/share/vkShade/vkShade.ini`
- `/etc/vkShade.ini`
- `/etc/vkShade/vkShade.ini`
- `/usr/share/vkShade/vkShade.ini`

> [!TIP]
> The default/example configuration file can be found in this repo at
> `config/vkShade.ini` and will be installed at either
> `$XDG_DATA_HOME/vkShade/vkShade.ini` for a user-level installation or at
> `/usr/share/vkShade/vkShade.ini` for a system-wide installation.

### ReShade FX

To use ReShade effects, you need to configure the search paths in
 `vkShade.ini`. For example:

```ini
[ReShade]
EffectSearchPaths = /opt/reshade/shaders,/opt/reshade/shaders/SweetFX
TextureSearchPaths = /opt/reshade/textures,/opt/reshade/textures/SweetFX
```

### ReShade Presets

vkShade supports ReShade-like preset files (`ReShade.ini`).

The preset file is searched for in the same locations as `vkShade.ini`, or a
custom file can be specified with the `VKSHADE_PRESET_FILE` environment
variable.

Uniform values are loaded from the preset and applied to effects at runtime.

> [!TIP]
> The ReShade preset and vkShade config files both support hot-reloading.
>
> If you overwrite the current config/preset file while vkShade is running, it
> will immediately pick up the change and reload the file. This is currently
> the supported way for tweaking uniforms at runtime.

> [!NOTE]
> ReShade preset support is functional, but currently partial.
>
> Uniforms are fully supported, but the `Techniques` and `TechniqueSorting`
> parameters are not supported yet. Instead, vkShade uses it's own `Effects`
> parameter and semantics.

### Input

Keybinds are now configurable under the `[Input]` section in `vkShade.ini`
 (single keys only for now). You can set custom keybinds by using the
 action name and the enum name of the key (`src/input/key_codes.hpp`):

```ini
[Input]
ToggleEffects = KEY_INSERT
ToggleGui = KEY_HOME
```

> [!TIP]
> If you don't set any keybinds, vkShade will use the defaults:
>
> - `INSERT` to toggle the effects
> - `HOME` to toggle the in-game GUI overlay

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

- [ReShade] - for the ReShade FX ecosystem
- [vkBasalt] - for the original inspiration

## License

Copyright (c) 2026 Ryan Algar
 ([ralgar/vkShade](https://github.com/ralgar/vkShade))

BSD 2-clause License (see [LICENSE](LICENSE) or
 [BSD 2-clause](https://choosealicense.com/licenses/bsd-2-clause/))

<!----------------------------------------------------------------------------->

[ReShade]: https://github.com/crosire/reshade
[vkBasalt]: https://github.com/DadSchoorse/vkBasalt
