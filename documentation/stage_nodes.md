# Stage Nodes

Each `Stage` is the root of a heirachy of `StageNodes`. Classes that inherit `StageNode` are:

 - `Stage`
 - `Actor`
 - `ParticleSystem`
 - `Geom`
 - `Camera`
 - `Light`
 
These are the basic building blocks of your scene.

## Traversal

There are a number of ways to traverse your stage heirarchy but they are all exposed through
standard C++ begin/end pairs designed to be used with C++ range for-loops. The below code
shows a number of examples of traversing a stage's tree:

```
for(auto node: stage->each_descendent()) {}  // Root-to-leaf iteration

for(auto node: stage->each_descendent_and_self()) {}  // Include the stage itself

for(auto node: stage->each_descendent_lf()) {}  // Leaf-first iteration

for(auto node: actor->each_ancestor()) {} // Traverse up the tree

for(auto node: actor->each_sibling()) {} // Iterate the siblings of the actor

for(auto node: stage->each_child()) {} // Iterate direct-children only
```

# Updating of StageNode and their Behaviours

`StageNodes` are only updated if the owning `Stage` is attached to an active `Pipeline`. You can check this by checking `Stage::is_part_of_active_pipeline()`. 

It is highly recommended that if you are manipulating a `Stage` in a background thread (e.g. in the `load()` method of a `Scene`) that you do not attach the `Stage` to a pipeline until you are finished manipulating. This should prevent threading problems.
