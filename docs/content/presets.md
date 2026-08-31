# ReShade Presets

vkShade supports ReShade-like preset files (`ReShade.ini`).

The preset file is searched for in the same locations as `vkShade.ini`, or a
custom file can be specified with the `VKSHADE_PRESET_FILE` environment
variable.

Uniform values are loaded from the preset and applied to effects at runtime.

!!! TIP
    The ReShade preset and vkShade config files both support hot-reloading.

    If you overwrite the current config/preset file while vkShade is running, it
    will immediately pick up the change and reload the file. This is currently
    the supported way for tweaking uniforms at runtime.

!!! NOTE
    ReShade preset support is functional, but currently partial.

    Uniforms are fully supported, but the `Techniques` and `TechniqueSorting`
    parameters are not supported yet. Instead, vkShade uses it's own `Effects`
    parameter and semantics.
