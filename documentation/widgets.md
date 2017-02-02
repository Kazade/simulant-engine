
# Widgets (Experimental)

Widgets are a class of StageNode which represent UI elements (e.g. Button, Label etc.). Widgets are managed by a stage's UI manager which can be accessed via the `ui` property.

Currently the following widgets are (partially) implemented:

 - Label - a text string
 - Button - a rectangular widget with optional text
 - Progress Bar

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

It's important to remember that padding doesn't affect the total widget size, and the border size extends outside the boundaries of the widget.

