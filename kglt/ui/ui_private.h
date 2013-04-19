#ifndef UI_PRIVATE_H
#define UI_PRIVATE_H

namespace Rocket {
namespace Core {

class Context;
class ElementDocument;

}
}

namespace kglt {
struct RocketImpl {
    RocketImpl():
        context_(nullptr),
        document_(nullptr) {}

    Rocket::Core::Context* context_;
    Rocket::Core::ElementDocument* document_;
};
}

#endif // UI_PRIVATE_H
