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
