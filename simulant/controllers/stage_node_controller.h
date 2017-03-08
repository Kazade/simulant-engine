#include "controller.h"
#include "../nodes/stage_node.h"

namespace smlt {

namespace controllers {

class StageNodeController:
    public Controller {

public:
    StageNodeController(Controllable* controllable):
        owner_(dynamic_cast<StageNode*>(controllable)) {

        if(!owner_) {
            throw std::logic_error("Attempted to use StageNodeController on an object which wasn't a StageNode");
        }
    }

protected:
    Property<StageNodeController, StageNode> owner = { this, &StageNodeController::owner_ };

private:
    StageNode* owner_;
};

}
}
