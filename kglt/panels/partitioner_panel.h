#pragma once

#include <unordered_map>
#include "panel.h"
#include "../types.h"

namespace kglt {

class WindowBase;

class PartitionerPanel : public Panel {
public:
    PartitionerPanel(WindowBase* window);

private:
    WindowBase* window_ = nullptr;

    void do_activate() override;
    void do_deactivate() override;
    void initialize();
    bool initialized_ = false;

    std::unordered_map<StageID, ActorID> debug_actors_;
};

}
