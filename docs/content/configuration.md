# Configuration

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

!!! TIP
    The default/example configuration file can be found in this repo at
    `config/vkShade.ini` and will be installed at either
    `$XDG_DATA_HOME/vkShade/vkShade.ini` for a user-level installation or at
    `/usr/share/vkShade/vkShade.ini` for a system-wide installation.

## ReShade FX

To use ReShade effects, you need to configure the search paths in
 `vkShade.ini`. For example:

```ini title="vkShade.ini"
[ReShade]
EffectSearchPaths = /opt/reshade/shaders,/opt/reshade/shaders/SweetFX
TextureSearchPaths = /opt/reshade/textures,/opt/reshade/textures/SweetFX
```

## Input

Keybinds are now configurable under the `[Input]` section in `vkShade.ini`
 (single keys only for now). You can set custom keybinds by using the
 action name and the enum name of the key (`src/input/key_codes.hpp`):

```ini title="vkShade.ini"
[Input]
ToggleEffects = KEY_INSERT
ToggleGui = KEY_HOME
```

!!! TIP
    If you don't set any keybinds, vkShade will use the defaults:

    - `INSERT` to toggle the effects
    - `HOME` to toggle the in-game GUI overlay

## Log Output

The log output level can be set with the `VKSHADE_LOG_LEVEL` environment var,
 e.g. `VKSHADE_LOG_LEVEL=debug`. Possible values are:
 `trace`, `debug`, `info`, `warn`, `error`, `critical`, and `off`.

By default the logger outputs to stderr, a file can be set as the output using
 the `VKSHADE_LOG_FILE` env var, e.g. `VKSHADE_LOG_FILE="vkShade.log"`.
