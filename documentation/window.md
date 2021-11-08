# The Window

The Window class controls access to the Compositor, and Input manager.

## Platform Information

You can get access to platform-specific information and functionality by using the global `get_platform()` function which will return a `Platform` subclass.

 - `Platform::name()` - Returns the name of the platform. One of "linux", "windows", "osx", "android", "psp", or "dreamcast"
 - `Platform::sleep_for_us(int32_t)` - Sleeps for the specified number of microseconds. This is mainly because the Dreamcast compiler doesn't support `std::this_thread::sleep_for(...)`.
