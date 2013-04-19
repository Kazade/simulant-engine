#ifndef INTERFACE_H
#define INTERFACE_H

#include <tr1/memory>

#include "../generic/managed.h"
#include"../types.h"
#include "../loadable.h"

namespace kglt {

class RocketImpl;
class WindowBase;

namespace ui {

class Interface;

class Interface :
    public Managed<Interface>,
    public Loadable {

public:
    Interface(WindowBase& window, uint32_t width_in_pixels, uint32_t height_in_pixels);
    ~Interface();

    uint16_t width_in_pixels() const { return width_; }
    uint16_t height_in_pixels() const { return height_; }

    RocketImpl* impl() { return impl_.get(); }

    bool init();
    void update(float dt);
    void render();

private:    
    std::string locate_font(const std::string& filename);

    WindowBase& window_;

    uint32_t width_;
    uint32_t height_;

    std::unique_ptr<RocketImpl> impl_;
};

}
}

#endif // INTERFACE_H
