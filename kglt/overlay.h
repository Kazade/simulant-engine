#ifndef UI_STAGE_H
#define UI_STAGE_H

#include "deps/kazsignal/kazsignal.h"
#include "generic/managed.h"
#include "generic/identifiable.h"
#include "generic/property.h"
#include "interfaces.h"
#include "ui/interface.h"
#include "resource.h"
#include "loadable.h"
#include "window_base.h"

namespace kglt {

class Overlay:
    public Managed<Overlay>,
    public generic::Identifiable<OverlayID>,
    public Resource,
    public RenderableStage,
    public Loadable {

public:
    /*
     *  Like Stage, this can be added to a pipeline with a camera
     *  and viewport, but is intended for building user-interfaces
     *  and not 2D/3D scenes
     */

    Overlay(OverlayID id, WindowBase *parent);
    ~Overlay();

    ui::ElementList append(const unicode& tag);
    ui::ElementList $(const unicode& selector);
    ui::ElementList find(const unicode& selector) { return this->$(selector); }

    void set_styles(const std::string& styles);
    void load_rml(const unicode& path);
    void load_rml_from_string(const unicode& data);
    void register_font_globally(const unicode& ttf_file);

    //Internal functions
    //Called when added to a pipeline, and also before rendering
    void render(CameraPtr camera, Viewport viewport);
    void update(double dt);

    void __handle_mouse_move(int x, int y);
    void __handle_mouse_down(int button);
    void __handle_mouse_up(int button, bool check_rendered=true);

    void __handle_touch_up(int finger_id, int x, int y, bool check_rendered=true);
    void __handle_touch_motion(int finger_id, int x, int y);
    void __handle_touch_down(int finger_id, int x, int y);

    ui::Interface* __interface() const { return interface_.get(); }

    // RenderableStage
    void on_render_started() {}
    void on_render_stopped();

    Property<Overlay, WindowBase> window = { this, &Overlay::window_ };
    Property<Overlay, ResourceManager> assets = { this, &Overlay::resource_manager_ };

private:
    WindowBase* window_ = nullptr;

    std::shared_ptr<ui::Interface> interface_;
    std::shared_ptr<ResourceManager> resource_manager_;

    sig::connection update_conn_;

    std::set<int> mouse_buttons_down_;
    std::set<int> fingers_down_;
};

}

#endif // UI_STAGE_H
