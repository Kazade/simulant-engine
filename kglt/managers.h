#ifndef MANAGERS_H
#define MANAGERS_H

#include "generic/generic_tree.h"
#include "generic/manager.h"
#include "types.h"
#include "interfaces.h"

namespace kglt {


class BackgroundManager:
    public generic::TemplatedManager<WindowBase, Background, BackgroundID>,
    public virtual Updateable {

public:
    BackgroundManager(WindowBase* window);

    BackgroundID new_background();
    BackgroundID new_background_from_file(const unicode& filename, float scroll_x=0.0, float scroll_y=0.0);
    BackgroundPtr background(BackgroundID bid);
    bool has_background(BackgroundID bid) const;
    void delete_background(BackgroundID bid);
    uint32_t background_count() const;

    void update(double dt) override;

private:
    WindowBase* window_;
};

class StageManager:
    public generic::TemplatedManager<WindowBase, Stage, StageID>,
    public virtual Updateable {

public:
    StageManager(WindowBase* window);

    StageID new_stage(AvailablePartitioner partitioner=PARTITIONER_OCTREE);
    StagePtr stage();
    StagePtr stage(StageID s);
    void delete_stage(StageID s);
    uint32_t stage_count() const;

    void print_tree();

    StageID default_stage_id() const { return default_stage_id_; }

    void update(double dt) override;

private:
    void print_tree(GenericTreeNode* node, uint32_t& level);

    WindowBase* window_;

    StageID default_stage_id_;
};

}

#endif // MANAGERS_H
