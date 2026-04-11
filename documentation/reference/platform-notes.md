# Platforms

Simulant is currently supported in some form on the following platforms:

 - Fedora (and probably other Linux distributions)
 - Windows
 - OSX (should work, not regularly tested)
 - Dreamcast (performance work needed)
 - PSP (runs, missing audio)
 - Android
 - Evercade (compiles, untested)

The following platforms will be targetted when possible (or when someone submits patches):

 - OG XBox (WIP)
 - PS2
 - GameCube
 

# The Platform API

Simulant provides a `Platform` property on the `Window` that provides access to platform-specific functionality, for example, what the native resolution is, or how much memory is available.

## Memory functions

 - `Platform::total_ram_in_bytes()` - Return the total amount of physical ram, returns `MEMORY_VALUE_UNAVAILABLE` if unsupported
 - `Platform::available_ram_in_bytes()` - Return the total amount of available ram, returns `MEMORY_VALUE_UNAVAILABLE` if unsupported
 - `Platform::process_ram_usage_in_bytes(process_id)` - Returns the ram usage of the specified process (used by `Application::ram_usage_in_bytes`), returns `MEMORY_VALUE_UNAVAILABLE` if unsupported
