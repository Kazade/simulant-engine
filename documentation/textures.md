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

## Locking textures

Every frame the active renderer will check to see if any textures need to be (re-)uploaded to the GPU. If you are manipulating texture data in another thread this could cause issues.

To protect against this you can lock a texture while you manipulate it. While a `Texture` is locked the renderer will not upload it to the GPU, update any filters, or generate any mipmaps.

To lock a texture you call the `Texture::lock()` method and store the resulting `TextureLock` object while you manipulate the data.

For example:

```
{   // Start of scope
    auto lock = texture->lock();
    
    // ... manipulate the texture
}   // End of scope, texture will be unlocked
```

## Manipulating data

When you manipulate the `Texture` data, you will need to notify the renderer that the data changed. You do this by calling `Texture::mark_data_changed()`, and the texture data will be uploaded to the GPU during the following frame. You should call `mark_data_changed()` while the `Texture` is locked.

## Filter modes

Simulant supports so-called Point, Bilinear and Trilinear filtering. The default is Point filtering. You can change the filter mode using `Texture::set_texture_filter()` and the GPU will reflect the change in the next frame.

## Mipmap generation

Mipmaps can optionally be generated automatically each time a `Texture` is uploaded to the GPU. This is the default behaviour. If you want to disable this you should call `Texture::set_mipmap_generation(MIPMAP_GENERATE_NONE)`. 

## Disabling uploads

Sometimes you might want to just leverage the texture loading functions of Simulant to get access to the image data (e.g. for generating heightmaps). In this situation it would be wasteful to
upload the texture data to the GPU unnecessarily. 

For this reason you can disable uploads by calling `Texture::set_auto_upload(false)`
 




