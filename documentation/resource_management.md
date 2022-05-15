# Resource Management

This section gives a brief overview of how assets are managed in Simulant

## An ID for Everything

When using Simulant you'll inevitably need to create, manipulate and delete many
different kinds of object. These include Cameras, Viewports, Actors, Stages and 
Pipelines.

All of these types of objects have a counterpart ID type, an instance of which
identifies a single object. These IDs can be exchanged with an object manager for
access to an object. It's important to understand that the manager
that created the object, retains ownership of it.

For example, you can create a Mesh (which is an Asset) like this:

    MeshID mid = stage->assets->new_mesh();
    
and later, you can exchange the ID for a MeshPtr like this:

    MeshPtr mesh = stage->mesh(mid);
    
## Assets vs Stage Nodes

Assets and StageNodes both use this ID system, but they behave in different ways:

 - Assets: are reference-counted, you should try not to store pointers to assets, instead store IDs
 - StageNodes: are *not* reference-counted, you can store the pointers to them
 
If you store a pointer to an Asset, the reference count will never hit zero and the asset will never
be freed which is why storing an ID is prefered.

Storing pointers to StageNodes doesn't suffer the same problem, but if you store IDs instead you can
check to see whether or not the StageNode still exists before accessing the pointer:

```
    smlt::ActorID id = stage->new_actor();
    if(stage->has_actor()) {
        // OK, the actor is still there
        smlt::ActorPtr ptr = stage->actor(id);
        ptr->move_to(x, y, z);
        // etc...
    }
```
         
## Release references ASAP
        
It is important that you don't hold on to asset pointers for longer than you need to.

The recommended approach is to wrap accesses to assets in curly braces or to get a reference and use it in a single line, for example:

```
    // ... code ...

    { //Start a scope block
        auto mesh = stage->assets->mesh(mesh_id);
        mesh->new_submesh_as_rectangle(...);
    } //End of scope, releases the lock on the mesh
```

This minimizes the amount of time the lock is held and so reduces the chance of a deadlock.

## Deleting objects
    
Resource managers (e.g. those that manage Materials, Textures, Shaders, Meshes 
and Sounds) are reference counted. This means that you don't need to delete objects
manually, other managers do not free objects automatically and you are required
to delete the objects with a delete call:

    stage->destroy_actor(actor_id);
    
StageNodes like Actors will not be released immediately. They will be destroyed after `late_update()` but before the render queue is constructed.
    
For more information see [Refcounted Managers](refcount_managers.md) and 
[Manual Managers](manual_managers.md)

