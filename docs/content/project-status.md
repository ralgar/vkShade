# Project Status

--8<--
README.md:project-status
--8<--

## Current Features

See also [Limitations](#limitations).

- [x] ReShade FX shader support
- [x] Configuration system with ReShade-like presets and hot-reloading
- [x] Interactive GUI overlay with keyboard and mouse support
- [x] Cross-platform input (Wayland, Xlib, XCB)
- [x] Modern C++ RAII wrappers around the Vulkan C API

## Limitations

Some ReShade effects and preset features do not work properly yet. For example:

- Effects relying on the depth buffer
- Effects with multiple techniques
- ReShade's `Techniques` and `TechniqueSorting` preset parameters
- Possibly more...

## Roadmap

### Near-term (v0.1.x)

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

### Mid-term (v0.2.x+)

- [ ] Support for effects with multiple techniques
- [ ] Full support for ReShade's input-based runtime uniforms
- [ ] Full support for ReShade's overlay-based runtime uniforms
- [ ] Hot-reloadable effects
- [ ] Depth buffer access

### Long-term

- [ ] Community effect repository
- [ ] Performance profiling tools
