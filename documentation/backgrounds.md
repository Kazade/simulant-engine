
# Backgrounds

It is quite common to want to be able to set a background on your scene, either static, 
scrollable, or animated. 

Simulant has built-in functionality for adding one or more backgrounds to the Window.

## Scrollable vs Animated

Under-the-hood, Simulant's Background class is implemented by creating a Stage with a single Sprite instance and then rendering with an orthographic Camera. 

For technical reasons it's not currently possible to scroll an animated background so when you 
create a background you must choose what kind of background you need. 

Scrollable backgrounds consist of a texture which is scrolled either vertically, horizontally, or in both directions.

Animated backgrounds consist of a number of frames and potentially different animations.

## Creating backgrounds

You can create a background using one of the following methods on the Window.

 - `Window::new_background(type)`
 - `Window::new_background_as_scrollable_from_file(filename, scrollx, scrolly)`
 - `Window::new_background_as_animated_from_file(filename, frame_with, frame_height, attrs)`
 
Each of these functions will return a `BackgroundID`, and as with any other ID it provides a
`fetch()` method for returning a pointer to the `Background` (`BackgroundPtr`)

As a background is a composite object, you can access the underlying objects using the following properties:

 - `Background::sprite`
 - `Background::stage`
