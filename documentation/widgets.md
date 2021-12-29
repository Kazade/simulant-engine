
# Widgets (Experimental)

Widgets are a class of StageNode which represent UI elements (e.g. Button, Label etc.). Widgets are managed by a stage's UI manager which can be accessed via the `ui` property.

Currently the following widgets are (partially) implemented:

 - Label - a text string
 - Button - a rectangular widget with optional text
 - Image - a widget that displays a texture
 - Progress Bar
 - Frame - a vertical or horizontal container of widgets that can optionally have a title bar

# Understanding Widgets

You can create new widgets using the UI manager. Widgets are composite objects which are made up of a mesh, and a child actor which uses that mesh. The mesh is made up of a series of rectangular layers which are stacked in the following order:

1. Border
2. Background
3. Foreground
4. Text

You can set the colours of each of these layers independently. You can also set the width of the border, and the amount of padding the widget has between the text and the bounds of the widget. 

The foreground is a special layer whose use varies depending on the widget type. The progress bar for example uses the foreground layer as the indicator bar itself, a checkbox might make the foreground layer visible when checked etc.

# Widget Sizing

There are a number of different methods of controlling widget sizes. These are:

1. `RESIZE_MODE_FIXED` - The widget is sized to whatever size you set and the text content tries to adjus accordingly
2. `RESIZE_MODE_FIXED_WIDTH` - The width you set is respected, but the height you set is treated as the minimum height. The height will expand to fit the text content.
3. `RESIZE_MODE_FIXED_HEIGHT` - The height you set is respected but the width is the minimum width and will expand to fit the text content.
4. `RESIZE_MODE_FIT_CONTENT` - The width and height you specify are the minimum values, the widget will expand horizontally to fit the text, and vertically if the text contains newlines.

If a widget dimension is dynamic (not fixed) then it will be calculated from the content width, padding, and border. If a widget dimension is fixed, then padding and border will fall inside the requested
widget size.

# Widget Types

## Image

An Image widget allows you display an image (e.g. a HUD icon) on the screen. Under the hood these
are implemented as widgets with a background image and an enforced `RESIZE_MODE_FIXED` resize mode.

Sample usage:

```
    auto simulant_logo = stage_->assets->new_texture_from_file("simulant/textures/simulant-icon.png");
    auto icon = stage_->ui->new_widget_as_image(simulant_logo);
```

You can set the source area of the Image widget (the region of the texture that is displayed) using the `set_source_rect` method:

 - `Image::set_source_rect(Vec2 bottom_left, Vec2 size)`

## Frame

A frame is a container widget, that can be a parent to other widgets. Frames organise their children into rows or columns depending on the layout direction. Frames can optionally have a titlebar (if text is provided).


