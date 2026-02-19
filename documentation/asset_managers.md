
# Asset Manager

Asset managers control the lifecycle of all assets in Simulant. There are two levels of asset manager:

 - Scene Asset Manager - a property of the Scene class, this is what you use for most asset loading
 - Shared Asset Manager - a property of the Application class. This is used for assets that must live across multiple scenes

All assets in Simulant are reference-counted and will disappear when all references are destroyed. 

>> Note: be careful that you store the result of `load_X()` calls, otherwise the asset will be destroyed immediately. If you want to keep the asset around without storing a reference, you can pass GARBAGE_COLLECT_NEVER as the garbage collection method and later find the asset by giving it a name.

The following are examples of asset types:

 - Textures
 - Materials
 - Sounds
 - Shaders
 - Meshes
 - Fonts
 - Prefabs
 - Particle scripts
 
 Assets can sometimes be linked together, for example a `Mesh` might reference a `Material` which might reference multiple `Textures`.

# Garbage Collection Methods

There are (currently) two different kinds of `GarbageCollectMethod` that you can set on a object, these are:

 - `GARBAGE_COLLECT_NEVER` - Never delete the object, keep it around indefinitely until the GC method is changed
 - `GARBAGE_COLLECT_PERIODIC` - The object will be deleted at some point after the final reference is released.

# Period Collection

When you create a asset, you need to maintain a reference to it otherwise the asset
will be destroyed.

For example, say you load a mesh, like so:

    MeshPtr my_mesh = assets->load_mesh("test.obj");
    
The mesh will stay loaded while `my_mesh` is in scope, but then it will be released unless you either store the
MeshPtr somewhere persistent, or attach the mesh to a stage node. e.g.

    stage->create_child<Actor>(my_mesh); //Links the mesh to an actor, preventing its destruction    
    
# Disabling collection

Sometimes you want to load a asset once, but there will be periods of time where it won't be attached to anything.

Imagine, for example, you have a Mesh for a torpedo. When the player fires a torpedo, you'd associate the Mesh to a new Actor, 
and when the torpedo detonates, the Actor will be destroyed. If that was the only torpedo in play, this would mean the mesh was
no longer linked to any Actor, and so would be garbage collected meaning next time the player fires, you'll need to reload the Mesh! 
Not ideal!

Fortunately, when you create a mesh, you can disable its garbage collection on a case-by-case basis:

    MeshPtr torpedo_mesh = assets->load_mesh(
        "torpedo.obj", 
        MeshLoadOptions(),
        GARBAGE_COLLECT_NEVER
    );
    
Now the torpedo_mesh will never be garbage collected. When you are absolutely finished with it, you can call:

    topedo_mesh->set_garbage_collection_method(GARBAGE_COLLECT_PERIODIC);
    
The torpedo will then be cleaned up at the next run of the garbage collector.

# Default materials and fonts

Simulant needs at minimum a default material, and a default font file. These are normally set to `Material::BuiltIns::DEFAULT` and Orbitron respectively but you can set them per manager by calling `AssetManager::set_default_material_filename()` and `AssetManager::set_default_font_filename()`
