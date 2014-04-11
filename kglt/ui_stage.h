#ifndef UI_STAGE_H
#define UI_STAGE_H

#include "generic/managed.h"
#include "generic/identifiable.h"

#include "ui/interface.h"
#include "resource.h"

namespace kglt {

class UIStage:
    public Managed<UIStage>,
    public generic::Identifiable<UIStageID>,
    public Resource {
public:
    /*
     *  Like Stage, this can be added to a pipeline with a camera
     *  and viewport, but is intended for building user-interfaces
     *  and not 2D/3D scenes
     */

    UIStage(WindowBase *parent, UIStageID id);

    ui::Element append(const std::string& tag);
    ui::ElementList $(const std::string& selector);
    void set_styles(const std::string& styles);
    void load_rml(const unicode& path);

    //Internal functions
    //Called when added to a pipeline, and also before rendering
    void __resize(uint32_t width, uint32_t height);
    void __render(const Mat4& projection_matrix);
    void __update(double dt);

private:
    WindowBase& window_;

    std::shared_ptr<ui::Interface> interface_;
};

}

#endif // UI_STAGE_H
