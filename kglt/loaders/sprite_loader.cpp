#include "sprite_loader.h"
#include "kglt/sprite.h"

namespace kglt {
namespace loaders {

void SpriteLoader::into(Loadable& resource, std::initializer_list<std::string> options) {
    Loadable* res_ptr = &resource;
    Sprite* sprite = dynamic_cast<Sprite*>(res_ptr);
    assert(sprite && "You passed a Resource that is not a sprite to the Sprite loader");


}

}
}
