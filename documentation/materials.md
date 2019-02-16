# Materials

Materials are one of the core objects that power Simulant's rendering pipeline. They are used to control the appearance of a mesh, but also aspects of how and when that mesh gets rendered.

Conceptually, a `Material` is made up of different properties, and these properties control things like the diffuse colour, the blending mode, the texture maps used etc. Material properties have a name, a type, and a default value.

In renderers that support shaders, the name of the property is also the shader variable name that will be populated by the value. 

The type of the property can be one of the following:

 - `bool`
 - `int`
 - `float`
 - `Vec2`
 - `Vec3`
 - `Vec4`
 - `Mat3`
 - `Mat4`
 - `TextureUnit`

Once a property is defined on the `Material`, you can set its value at either the `Material` level or the `MaterialPass` level. A `Material` can have a number of passes and a submesh will be rendered once-per-material-pass. It's useful to be able to control which property values apply per-rendering of the submesh.

For example, say you wanted to render two-passes, with different diffuse textures. You could construct a material to do that as follows:

```
auto material = stage->assets->new_material();
material->set_pass_count(2);
material->pass(0)->set_diffuse_map(texture1);
material->pass(1)->set_diffuse_map(texture2);
```

Alternatively, if you want both passes to share the same diffuse map, you could set the diffuse map property at the `Material` level:

```
auto material = stage->assets->new_material();
material->set_pass_count(2);
material->set_diffuse_map(shared_texture);  // Applies to all passes
```

## Built-in Properties

Material properties are defined at runtime and you can define your own, but all `Materials` have a large number of pre-defined properties that you can rely on. All built-in properties are named with an `s_` prefix. If you are defining custom properties, avoid using the `s_` prefix for those.

Here is a list of all the available built-in properties:

 - `s_material_ambient`
 - `s_material_diffuse`
 - `s_material_specular`
 - `s_material_shininess`
 - `s_diffuse_map`
 - `s_light_map`
 - `s_normal_map`
 - `s_specular_map`
 - `s_blend_func`
 - `s_cull_mode`
 - `s_colour_material`
 - `s_depth_test_enabled`
 - `s_depth_write_enabled`
 - `s_shade_model`
 - `s_polygon_mode`
 - `s_lighting_enabled`
 - `s_texturing_enabled`
 - `s_point_size`
 - `s_light_position` *
 - `s_light_ambient` *
 - `s_light_diffuse` *
 - `s_light_specular` *
 - `s_light_constant_attenuation` *
 - `s_light_linear_attenuation` *
 - `s_light_quadratic_attenuation` *
 - `s_modelview` *
 - `s_modelview_projection` *
 - `s_projection` *
 - `s_view` *
 - `s_inverse_transpose_modelview_projection` *

Note that the properties marked with an asterisk automatically have their values set by the renderer, they exist as properties so that they are accessible as variables in shaders.

## Defining and Setting Custom Properties

Defining custom properties is straightforward:

```
auto material = stage->assets->new_material();
material->define_property(MATERIAL_PROPERTY_TYPE_INT, "my_property", 999);
```

This would define an integer property with a default value of 999. Be aware that property names must be valid GLSL shader names. 

To set a value for this property, you can use the `set_property_value` method:

```
material->set_property_value<int>("my_property", 5);
```

## Loading Materials from File

Although you can define `Materials` entirely in code, the more usual thing to do is to define them in `.smat` files which are in Simulant Material format.

Simulant Material format is a JSON document with a defined structure. The minimal material file is this:

```
{
    "passes": [{}]
}
```

This is a single-pass material with all built in properties set to default.

If you want to set property values material-wide, you can put them at the top level:

```
{
    "property_values": {
        "s_material_diffuse": "1 0 0 1"
    },
    "passes": [{}]
}
```

Or, you can do it at the pass-level:


```
{
    "passes": [
        {
            "property_values": {
                "s_material_diffuse": "1 0 0 1"
            }
        }
    ]
}
```

Finally, if you want to define custom properties, you can do that too:

```
{
    "custom_properties": [
        {"name": "my_property", "type": "int", "default": 0}
    ],
    "passes": [{}]
}
```

When using the GL 2.x renderer, you'll likely want to specify a `vertex_shader` and `fragment_shader` in the pass dictionary. These should be paths (relative to the file, or on the asset search path) to `.vert` and `.frag` files. Failure to specify these shaders will default to a shader that doesn't really do anything!

