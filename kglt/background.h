#ifndef BACKGROUND_H
#define BACKGROUND_H

#include "generic/managed.h"
#include "generic/identifiable.h"
#include "utils/parent_setter_mixin.h"
#include "generic/protected_ptr.h"
#include "object.h"

namespace kglt {

enum BackgroundResizeStyle {
    BACKGROUND_RESIZE_ZOOM,
    BACKGROUND_RESIZE_SCALE
};

class Background:
    public Managed<Background>,
    public generic::Identifiable<BackgroundID>,
    public Protectable,
    public Updateable,
    public Nameable,
    public Printable {

public:
    Background(WindowBase *window, BackgroundID background_id);

    bool init() override;
    void cleanup() override;
    void update(double dt) override;

    void set_horizontal_scroll_rate(float x_rate);
    void set_vertical_scroll_rate(float y_rate);
    void set_texture(TextureID texture_id);
    void set_resize_style(BackgroundResizeStyle style);

    //Ownable interface
    void ask_owner_for_destruction();

    //Printable interface
    unicode __unicode__() const;

    //Nameable interface
    const bool has_name() const;
    void set_name(const unicode &name);
    const unicode name() const;

    WindowBase& window() { return *window_; }
private:
    WindowBase* window_;
    unicode name_;

    void update_camera();

    StageID stage_id_;
    CameraID camera_id_;
    PipelineID pipeline_id_;
    MaterialID material_id_;
    ActorID actor_id_;

    BackgroundResizeStyle style_ = BACKGROUND_RESIZE_ZOOM;
    float x_rate_ = 0.0;
    float y_rate_ = 0.0;
};

}

#endif // BACKGROUND_H
