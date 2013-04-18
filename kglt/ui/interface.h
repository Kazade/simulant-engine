#ifndef INTERFACE_H
#define INTERFACE_H

#include "../generic/managed.h"
#include"../types.h"
#include "../loadable.h"

namespace Rocket {
namespace Core {
    class Context;
    class ElementDocument;
}
}

namespace kglt {

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

    Rocket::Core::ElementDocument* _document() { return document_; }
    void _set_document(Rocket::Core::ElementDocument* doc) { document_ = doc; }

    Rocket::Core::Context* _context() { return context_; }

    bool init();
    void update(float dt);
    void render();

private:    
    std::string locate_font(const std::string& filename);

    WindowBase& window_;

    uint32_t width_;
    uint32_t height_;

    Rocket::Core::Context* context_;
    Rocket::Core::ElementDocument* document_;
};

}
}

#endif // INTERFACE_H
