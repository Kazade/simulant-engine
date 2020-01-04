#pragma once

#include "behaviour.h"
#include "../nodes/stage_node.h"
#include "../macros.h"

namespace smlt {

namespace behaviours {

class StageNodeBehaviour:
    public Behaviour {

public:
    StageNodeBehaviour() = default;
    Property<StageNodeBehaviour, StageNode> stage_node = { this, &StageNodeBehaviour::stage_node_ };

protected:
    void on_behaviour_added(Organism* controllable) override {
        stage_node_ = dynamic_cast<StageNode*>(controllable);
    }

    void on_behaviour_removed(Organism *controllable) override {
        _S_UNUSED(controllable);

        stage_node_ = nullptr;
    }

private:
    StageNode* stage_node_ = nullptr;
};

}
}
