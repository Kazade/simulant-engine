# Spatial Hash Partitioner

Simulant's default partitioner uses a technique called "Spatial Hashing" to determine whether objects are within the frustum (or, at least, almost within the frustum). The implementation of Spatial Hashing in Simulant may well be unique. For a quick overview of how Spatial Hashing works, I recommend [this article](http://www.gamedev.net/page/resources/_/technical/game-programming/spatial-hashing-r2697).

## Overview

A traditional spatial hashing algorithm has the following problems:

1. The cell-size must be tuned to the dataset. If objects are too big for the cell-size, they will span multiple cells wastefully, on the other hand if objects are much smaller than the cell-size, then the spatial hash can be inefficient.
1. Using a spatial hash for frustum culling can be extraordinarily slow if the cell size is much smaller than the viewing frustum.

Because of these drawbacks spatial hashing doesn't seem to be commonly used for frustum culling (it's apparently more commonly used in physics engines where objects are likely similar sizes).

The algorithm we use in Simulant uses 16 different spatial hashes, each with a cell-size double that of the last. The cell sizes are powers of two: 1, 2, 4, 8, 16, 32, 64, 128, 256, 512, 1024, 2048, 4096, 8192, 16384 and 32768.

When an object is inserted into the hash, the cell-size chosen is the first one which will fit the object's AABB. So, a 3x3x3 object would be inserted into the spatial hash with a cell-size of 4. 

The clever part is, the implementation doesn't have multiple hashes - it has a single ordered std::map which maps a key to one or more objects.

## Keys, Ancestors and Descendents

When an object is inserted, a multi-level Key is generated. Each Key is made up of up to 16, 3-dimensional hash keys to form a path in a tree of spatial hashes, starting from the largest cell-size to the smallest in which the object can fit.

Theoretically, these keys form heirarchies. If Key A has 16 elements in its path, and Key B has 15 elements and those 15 are the same as the first 15 of Key A, then Key B is an ancestor of Key A, and Key A is a descendent of Key B. This gives us a powerful lookup system as we know that if objects with Key B are visible, then objects of Key A are also visible. If this is difficult to visualise, consider that these multiple spatial hashes are essentially overlapping grids. If you know a grid cell is visible, you know that smaller grid cells within it are also visible.

## Gathering Objects

All object gathering (at the moment) happens with axis-aligned cubes. The process is as follows:

1. When passed an AABB, the max_dimension is used to build a cube around its central point. Keys are generated for the 8 corners of the cube at the cell-size which would fit the max_dimension.
2. The keys are deduplicated and then each key is looked up in the spatial hash index. The index is iterated in order until a key is found which is not a descendent of the passed in key
3. When objects for all iterated keys are added, the search looks up the tree by popping the last element of the path of the key, and then looking for exact matches. This is an anccestor search. If any are found then objects at this key are added to the set. These objects are larger than the box max_dimension but overlap the same space and should be returned also.

# Improvements

 - Frustum culling is currently performed by a single box around the frustum, this returns many objects outside the frustum when using a perspective projection, and particularly with a large far-distance. It would make more sense to find the hash keys which are within the frustum at a particular cell-size (possible determined by the size of the frustum) by performing a kind of 3D scanline rendering (iterating the bounds of the frustum AABB). Then the lookup could be performed for each of those keys. 



