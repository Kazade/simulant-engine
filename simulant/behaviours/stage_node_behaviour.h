#pragma once

#include "behaviour.h"
#include "../nodes/stage_node.h"

namespace smlt {

namespace behaviours {

class StageNodeBehaviour:
    public Behaviour {

public:
    StageNodeBehaviour(Organism* controllable):
        owner_(dynamic_cast<StageNode*>(controllable)) {

        if(!owner_) {
            throw std::logic_error("Attempted to use StageNodeBehaviour on an object which wasn't a StageNode");
        }
    }

protected:
    Property<StageNodeBehaviour, StageNode> owner = { this, &StageNodeBehaviour::owner_ };

private:
    StageNode* owner_;
};

}
}
