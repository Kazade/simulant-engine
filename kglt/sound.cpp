
#include "sound.h"

namespace kglt {

Sound::Sound(ResourceManager *resource_manager, SoundID id):
    generic::Identifiable<SoundID>(id),
    Resource(resource_manager) {

}

}
