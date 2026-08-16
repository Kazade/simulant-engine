/* *   Copyright (c) 2011-2017 Luke Benstead https://simulant-engine.appspot.com
 *
 *     This file is part of Simulant.
 *
 *     Simulant is free software: you can redistribute it and/or modify
 *     it under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 *     Simulant is distributed in the hope that it will be useful,
 *     but WITHOUT ANY WARRANTY; without even the implied warranty of
 *     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *     GNU Lesser General Public License for more details.
 *
 *     You should have received a copy of the GNU Lesser General Public License
 *     along with Simulant.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include "stage_node.h"

namespace smlt {

class Armature;

/** A single bone of a skeleton.
 *
 *  Joints are ordinary stage nodes - they're posed by moving their
 *  transform, either by hand or (more usually) by an `AnimationController`
 *  driving them from keyframe data. Anything parented to a Joint follows it
 *  around, so attaching props to a skeleton is just a matter of
 *  `set_parent`.
 *
 *  A Joint must be a descendent of an `Armature`, which is what actually
 *  consumes the pose. `joint_index` identifies which of the armature's
 *  inverse bind matrices belongs to this joint.
 */
class Joint: public StageNode {
public:
    S_DEFINE_STAGE_NODE_META("joint");

    S_DEFINE_STAGE_NODE_PARAM(
        Joint, "joint_index", int, -1,
        "This joint's index within its armature's skeleton");

    Joint(Scene* owner) :
        StageNode(owner, Meta::node_type) {}

    int joint_index() const {
        return joint_index_;
    }

    /** The armature posing this joint, or nullptr if the joint isn't
     *  currently below one */
    Armature* armature() const {
        return armature_;
    }

private:
    bool on_create(Params params) override;
    bool on_destroy() override;
    void on_parent_set(const StageNode* oldp, const StageNode* newp) override;
    void on_transformation_changed() override;

    /* Walks up the tree looking for the closest Armature ancestor */
    void resolve_armature();

    int joint_index_ = -1;
    Armature* armature_ = nullptr;
};

typedef Joint* JointPtr;

} // namespace smlt
