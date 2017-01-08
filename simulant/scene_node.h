/* *   Copyright (c) 2011-2017 Luke Benstead https://simulant-engine.appspot.com
 *
 *     This file is part of Simulant.
 *
 *     Simulant is free software: you can redistribute it and/or modify
 *     it under the terms of the GNU General Public License as published by
 *     the Free Software Foundation, either version 3 of the License, or
 *     (at your option) any later version.
 *
 *     Simulant is distributed in the hope that it will be useful,
 *     but WITHOUT ANY WARRANTY; without even the implied warranty of
 *     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *     GNU General Public License for more details.
 *
 *     You should have received a copy of the GNU General Public License
 *     along with Simulant.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef SCENE_NODE_H
#define SCENE_NODE_H

#include "generic/generic_tree.h"
#include "interfaces.h"

namespace smlt {

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
