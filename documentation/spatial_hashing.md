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





