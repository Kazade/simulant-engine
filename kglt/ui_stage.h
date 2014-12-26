#ifndef UI_STAGE_H
#define UI_STAGE_H

#include <kazbase/signals.h>
#include "generic/managed.h"
#include "generic/identifiable.h"
#include "interfaces.h"
#include "ui/interface.h"
#include "resource.h"

namespace kglt {

class UIStage:
    public Managed<UIStage>,
    public generic::Identifiable<UIStageID>,
    public Resource,
    public RenderableStage {

public:
    /*
     *  Like Stage, this can be added to a pipeline with a camera
     *  and viewport, but is intended for building user-interfaces
     *  and not 2D/3D scenes
     */

    UIStage(WindowBase *parent, UIStageID id);
    ~UIStage();

    ui::ElementList append(const unicode& tag);
    ui::ElementList $(const unicode& selector);
    void set_styles(const std::string& styles);
    void load_rml(const unicode& path);
    void load_rml_from_string(const unicode& data);
    void register_font_globally(const unicode& ttf_file);

    //Internal functions
    //Called when added to a pipeline, and also before rendering
    void __resize(uint32_t width, uint32_t height);
    void __render(const Mat4& projection_matrix);
    void __update(double dt);

    void __handle_mouse_move(int x, int y);
    void __handle_mouse_down(int button);
    void __handle_mouse_up(int button);

    void __handle_touch_up(int finger_id, int x, int y);
    void __handle_touch_motion(int finger_id, int x, int y);
    void __handle_touch_down(int finger_id, int x, int y);

    // RenderableStage
    void on_render_started() {}
    void on_render_stopped() {}
private:
    WindowBase& window_;

    std::shared_ptr<ui::Interface> interface_;
    sig::connection update_conn_;
};

}

#endif // UI_STAGE_H
