//
//   Copyright (c) 2011-2017 Luke Benstead https://simulant-engine.appspot.com
//
//     This file is part of Simulant.
//
//     Simulant is free software: you can redistribute it and/or modify
//     it under the terms of the GNU Lesser General Public License as published
//     by the Free Software Foundation, either version 3 of the License, or (at
//     your option) any later version.
//
//     Simulant is distributed in the hope that it will be useful,
//     but WITHOUT ANY WARRANTY; without even the implied warranty of
//     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//     GNU Lesser General Public License for more details.
//
//     You should have received a copy of the GNU Lesser General Public License
//     along with Simulant.  If not, see <http://www.gnu.org/licenses/>.
//

#include "joint.h"
#include "armature.h"
#include "stage_node_iterators.h"

namespace smlt {

bool Joint::on_create(Params params) {
    if(!clean_params<Joint>(params)) {
        return false;
    }

    joint_index_ = params.get<int>("joint_index").value_or(-1);

    /* Nodes are parented after creation, so there's usually no armature to
     * find yet - on_parent_set picks it up when there is */
    resolve_armature();

    return StageNode::on_create(params);
}

bool Joint::on_destroy() {
    if(armature_) {
        armature_->mark_joints_dirty();
        armature_ = nullptr;
    }

    return StageNode::on_destroy();
}

void Joint::on_parent_set(const StageNode* oldp, const StageNode* newp) {
    StageNode::on_parent_set(oldp, newp);

    auto previous = armature_;
    resolve_armature();

    /* Both skeletons changed shape - the one we left, and the one we
     * joined */
    if(previous && previous != armature_) {
        previous->mark_joints_dirty();
    }

    if(armature_) {
        armature_->mark_joints_dirty();
    }
}

void Joint::on_transformation_changed() {
    StageNode::on_transformation_changed();

    if(armature_) {
        armature_->mark_skinning_dirty();
    }
}

void Joint::resolve_armature() {
    armature_ = nullptr;

    for(auto& node: each_ancestor()) {
        if(node.node_type() == Armature::Meta::node_type) {
            armature_ = static_cast<Armature*>(&node);
            return;
        }
    }
}

} // namespace smlt
