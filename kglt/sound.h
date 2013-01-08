#ifndef SOUND_H
#define SOUND_H

#include "generic/managed.h"
#include "generic/identifiable.h"
#include "resource.h"
#include "loadable.h"

#include "types.h"

namespace kglt {

class Sound :
    public Managed<Sound>,
    public generic::Identifiable<SoundID>,
    public Resource,
    public Loadable {

public:
    Sound(ResourceManager* resource_manager, SoundID id);
};

class Source {
public:
    Source(SubScene& subscene) {}
    virtual ~Source() {}

    void play(SoundID sound) {}
    bool is_playing_sound() const { return false; }
};

}
#endif // SOUND_H
