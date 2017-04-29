
# Refcounted Managers

Refcounted managers are the second kind of management class in Simulant (the other being [Manual managers](manual_managers.md)). All resources
in Simulant use a refcounted manager, these resources include:

 - Textures
 - Materials
 - Sounds
 - Shaders
 - Meshes
 
In normal usage, these materials are linked just after creation to another resource (e.g. Texture -> Material) or
to an object that has a manual manager (e.g. an Actor). A resource can obviously be linked to more than one object
but when all linked objects/resources are destroyed, then the refcounted manager will delete the resource.

# The GC grace period

When you create a resource, you have a 10 second grace period to grab a reference to it, after that time the resource
will be destroyed and a warning will be logged.

For example, say you load a mesh, like so:

    MeshID my_mesh = stage()->new_mesh_from_file("test.obj");
    
Once the new_mesh_from_file call returns, you have 10 seconds to do this:

    auto mesh = stage()->mesh(my_mesh); //Marks resource as accessed
    // when 'mesh' goes out of scope, the mesh will be destroyed!
    
or this:

    stage()->new_actor_with_mesh(my_mesh); //Links the mesh to an actor, preventing its destruction    
    
Failure to access a resource within the 10 second grace period will result in its destruction,
and any further attempts to access it will result in an ObjectDoesNotExist<T> exception.

# Disabling collection

Sometimes you want to load a resource once, but there will be periods of time where it won't be attached to anything.

Imagine, for example, you have a Mesh for a torpedo. When the player fires a torpedo, you'd associate the Mesh to a new Actor, 
and when the torpedo detonates, the Actor will be destroyed. If that was the only torpedo in play, this would mean the mesh was
no longer linked to any Actor, and so would be garbage collected meaning next time the player fires, you'll need to reload the Mesh! 
Not ideal!

Fortunately, when you create a mesh, you can disable its garbage collection on a case-by-case basis:

    MeshID torpedo_mesh = stage()->new_mesh_from_file("torpedo.obj", /*garbage_collect=*/false);
    
Now the torpedo_mesh will never be garbage collected. When you are absolutely finished with it, you can call:

    stage()->mesh(topedo_mesh)->enable_gc();
    
The torpedo will then be cleaned up at the next run of the garbage collector.



