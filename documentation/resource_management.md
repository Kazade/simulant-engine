# Resource Management

This section gives a brief overview of how resources are managed in KGLT

## An ID for Everything

When using KGLT you'll inevitably need to create, manipulate and delete many
different kinds of object. These include Cameras, Viewports, Actors, Stages and 
Pipelines.

All of these types of objects have a counterpart ID type, an instance of which
identifies a single object. These IDs can be exchanged with an object manager for
temporary access to an object. It's important to understand that the manager
that created the object, retains ownership of it.

For example, the Stage is a manager of Actors. You can create an Actor with the
new_actor() method:

    ActorID actor_id = stage.new_actor();
    
new_actor() creates a new actor instance, but it doesn't return it. Instead you
are given an ActorID which is your token to get access to the Actor.

When you want to manipulate the Actor, you can use your ActorID to get it:

    auto actor = stage.actor(actor_id);
    actor->move_to(10, 10, 10);
        
## Release references ASAP
        
It is important that you don't hold on to object references for longer than you need to.
The reason is that accessor functions like actor() return ProtectedPtr<T> instances.
ProtectedPtr is a wrapper around both a std::shared_ptr<T> and a std::lock_guard. If
you hold on to the reference for too long, you will deadlock other threads that
need access to the actor.

## Deleting objects
    
Resource managers (e.g. those that manage Materials, Textures, Shaders, Meshes 
and Sounds) are reference counted. This means that you don't need to delete objects
manually, other managers do not free objects automatically and you are required
to delete the objects with a delete call:

    stage.delete_actor(actor_id);
    
For more information see [Ref-counted Managers](refcount_managers.md) and 
[Manual Managers](manual_managers.md)

