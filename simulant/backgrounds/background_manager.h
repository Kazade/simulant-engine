#pragma once

#include "../generic/property.h"
#include "../utils/unicode.h"
#include "background.h"

namespace smlt {

template<typename T, typename IDType, typename ...Subtypes>
class ManualManager;

class Window;

class BackgroundManager {
public:
    BackgroundManager(Window* window);
    virtual ~BackgroundManager();

    BackgroundPtr new_background(BackgroundType type);
    BackgroundPtr new_background_as_scrollable_from_file(const unicode& filename, float scroll_x=0.0, float scroll_y=0.0);
    BackgroundPtr new_background_as_animated_from_file(const unicode& filename);

    BackgroundPtr background(BackgroundID bid);
    bool has_background(BackgroundID bid) const;
    BackgroundPtr destroy_background(BackgroundID bid);
    uint32_t background_count() const;

    void destroy_all_backgrounds();

    void update(float dt);

    Property<BackgroundManager, Window> window = { this, &BackgroundManager::window_ };

    void clean_up();
private:
    Window* window_ = nullptr;

    typedef ManualManager<Background, BackgroundID> Manager;
    std::shared_ptr<Manager> backgrounds_;
};

}
