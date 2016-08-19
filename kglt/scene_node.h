#ifndef SCENE_NODE_H
#define SCENE_NODE_H

#include "generic/generic_tree.h"
#include "interfaces.h"

namespace kglt {

//SceneNode base class. Not Transformable (because not all nodes are - the Stage for example)
class SceneNode:
    public Locateable,
    public Updateable,
    public GenericTreeNode,
    public Ownable,
    public Printable,
    public Nameable {

public:
};

}

#endif // SCENE_NODE_H
