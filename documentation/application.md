# Application

The `Application` is the entry point to your game. Normally your `Application` subclass would live in your `main.cpp` file, and your `main()` method would look something like:

```
    smlt::AppConfig config;  // AppConfig structure allows you to control system settings
    MyApplication app(config); // Instantiate your Application subclass
    return app.run(argc, argv);  // Run your application, returns 0 on success
```

## Signals

The following signals are available for the Application class:

 - `signal<void ()> signal_frame_started();`
 - `signal<void ()> signal_frame_finished();`
 - `signal<void ()> signal_pre_swap();`
 - `signal<void ()> signal_post_coroutines(); `
 - `signal<void (float)> signal_update();`
 - `signal<void (float)> signal_fixed_update();`
 - `signal<void (float)> signal_late_update();`
 - `signal<void (float)> signal_post_late_update();`
 - `signal<void ()> signal_shutdown();`

# AppConfig

The AppConfig structure allows you to control a number of settings, in particular you can set the following:

 - `AppConfig::title` - The title of your game (appears in the window title)
 - `AppConfig::fullscreen` - Whether or not to launch fullscreen
 - `AppConfig::width` and `AppConfig::height` - The width and height of the window
 - `AppConfig::development::force_renderer` - Overrides the default renderer from the system default. 
 - `AppConfig::development::force_sound_driver` - Overrides the default sound driver from the system default
 - `AppConfig::developnent::log_file` - If specified, a text file will be created at this location and logs will be output there


