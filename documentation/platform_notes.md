# Platforms

Simulant is currently supported in some form on the following platforms:

 - Fedora (and probably other Linux distributions)
 - Windows
 - OSX
 - Dreamcast (room for improvement, performance and sound need work)
 - PSP (compiles, missing features)
 - Android (untested, needs work)

The following platforms will be targetted when possible (or when someone submits patches):

 - iOS
 - Web (via WASM)


## Dreamcast

Dreamcast support is currently a work in progress. The following things aren't supported yet:

 - Mouse input
 - Sound
 - Any kind of VMU support
 - Romdisk loading

There are also the following problems:

 - Unsupported operations: Point particles
 

# The Platform API

Simulant provides a `Platform` property on the `Window` that provides access to platform-specific functionality, for example, what the native resolution is, or how much memory is available.

## Memory functions

 - `Platform::total_ram_in_bytes()` - Return the total amount of physical ram, returns `MEMORY_VALUE_UNAVAILABLE` if unsupported
 - `Platform::available_ram_in_bytes()` - Return the total amount of available ram, returns `MEMORY_VALUE_UNAVAILABLE` if unsupported
 - `Platform::process_ram_usage_in_bytes(process_id)` - Returns the ram usage of the specified process (used by `Application::ram_usage_in_bytes`), returns `MEMORY_VALUE_UNAVAILABLE` if unsupported
