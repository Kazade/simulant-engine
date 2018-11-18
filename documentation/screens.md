# Screens

Screens are a representation of external display devices like the Dreamcast VMU. They have a width, height, colour format and refresh rate. They also have a pre-designated name.

In the case of the Dreamcast VMU, the name is defined by the controller port (A, B, C or D) and then a number (1 or 2), so the VMU screen plugged into port one of the first controller would be "A1".

You can access the screen using `Window::screen(name)` which returns a `Screen*`. You can find out how many screens are connected using `Window::screen_count()`.

Sometimes you'll want to perform an operation on all screens, and there is an iterator function that let's you do that: `Window::each_screen(std::function<void (string, Screen*)> callback)`. 

To render to an external screen you use `Screen::render(data, format)`. Currently `format`
must be `SCREEN_FORMAT_G1` which is 1-bit greyscale.

Calling render too quickly might result in some frames being dropped if the screen can't display them fast enough, you can judge how quickly to render by querying the `refresh_rate()` attribute.

## The Virtual Screen

Sometimes it's useful to test your Screen rendering while developing on a PC. Simulant provides
applications the ability to enable a single virtual screen for testing.

You can enable the screen by setting `desktop.enable_virtual_screen = true` in your `AppConfig`. There are other related settings too:

 - `desktop.virtual_screen_width`
 - `desktop.virtual_screen_height`
 - `desktop.virtual_screen_format`
 - `desktop.virtual_screen_integer_scale`
 
`virtual_screen_integer_scale` increases the size of the test window, which is useful on Hi-dpi screens when you're testing 48x32 VMU images!

