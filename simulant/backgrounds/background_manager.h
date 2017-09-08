#pragma once

#include "../generic/property.h"
#include "../interfaces.h"
#include "../utils/unicode.h"
#include "background.h"

namespace smlt {

class Window;

class BackgroundManager:
    public generic::TemplatedManager<Background, BackgroundID>,
    public virtual Updateable {

public:
    BackgroundManager(Window* window);
    ~BackgroundManager();

    BackgroundID new_background(BackgroundType type);
    BackgroundID new_background_as_scrollable_from_file(const unicode& filename, float scroll_x=0.0, float scroll_y=0.0);
    BackgroundID new_background_as_animated_from_file(const unicode& filename);

    BackgroundPtr background(BackgroundID bid);
    bool has_background(BackgroundID bid) const;
    void delete_background(BackgroundID bid);
    uint32_t background_count() const;

    void update(float dt) override;

    Property<BackgroundManager, Window> window = { this, &BackgroundManager::window_ };
private:
    Window* window_;
};

}
