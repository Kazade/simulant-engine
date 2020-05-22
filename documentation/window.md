# The Window

The Window is the root of all things. The Window class controls access to Stages, the Compositor, and asset loading.

## Signals

The following signals are available for the Window class:

 - signal<void ()> signal_frame_started();
 - signal<void ()> signal_frame_finished();
 - signal<void ()> signal_pre_swap();
 - signal<void (double)> signal_step();
 - signal<void ()> signal_shutdown();

## Platform Information

The Window class has a `platform` property which is for platform-specific functionality and information. It has the following attributes:

 - `Platform::name()` - Returns the name of the platform. One of "linux", "windows", "darwin", "android" or "dreamcast"
 - `Platform::sleep_for_us(int32_t)` - Sleeps for the specified number of microseconds. This is mainly because the Dreamcast compiler doesn't support `std::this_thread::sleep_for(...)`.
