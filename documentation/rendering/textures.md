
# Textures

Textures represent images which can either be applied as a mesh, or less commonly to a framebuffer.

The `Texture` class in Simulant represents a texture, and has a counterpart `TextureID` identifer. Textures are reference-counted and so you should try not to hold on to their `TexturePtr` reference
directly as this will prevent garbage collection.

Textures are created via a `ResourceManager`; either `stage->assets` or `window->shared_assets`.

## Image data

A `Texture` instance may hold raw image data loaded from disk, but by default it only does this transiently. The default behaviour is that a Textures data attribute is cleared once it has been uploaded to the GPU by the renderer. You can affect this behaviour by setting the `free_data_mode` of a texture:

```
texture->set_free_data_mode(TEXTURE_FREE_DATA_NEVER);
```

The default setting is `TEXTURE_FREE_DATA_AFTER_UPLOAD`. 


## Filter modes

Simulant supports so-called Point, Bilinear and Trilinear filtering. The default is Point filtering. You can change the filter mode using `Texture::set_texture_filter()` and the GPU will reflect the change in the next frame.

## Mipmap generation

Mipmaps can optionally be generated automatically each time a `Texture` is uploaded to the GPU. This is the default behaviour. If you want to disable this you should call `Texture::set_mipmap_generation(MIPMAP_GENERATE_NONE)`. 

## Disabling uploads

Sometimes you might want to just leverage the texture loading functions of Simulant to get access to the image data (e.g. for generating heightmaps). In this situation it would be wasteful to
upload the texture data to the GPU unnecessarily. 

For this reason you can disable uploads by calling `Texture::set_auto_upload(false)`
 
## Format conversion

There are times when the texture loaded from disk isn't in the format required during rendering. For example, the .fnt font loader loads single-channel textures from disk, but requires RGBA
textures during rendering. `Texture::convert(format, channels)` allows you to perform conversions
between different formats, or even just swizzle channels.

The `format` parameter is the destination `TextureFormat`, the `channels` parameter is a 4-element array (RGBA) of constants choosing which source channel will be used in place of the destination channels. For example, if you wanted to swap red and green, you could pass:

```
{TEXTURE_CHANNEL_GREEN, TEXTURE_CHANNEL_RED, TEXTURE_CHANNEL_BLUE, TEXTURE_CHANNEL_ALPHA}
```

There are two additional channel constants, `TEXTURE_CHANNEL_ONE` and `TEXTURE_CHANNEL_ZERO`. These allow you to set channels to full or zero intensity.


