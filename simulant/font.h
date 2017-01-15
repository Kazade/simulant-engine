#pragma once

#include "generic/managed.h"

namespace smlt {

enum CharacterSet {
    CHARACTER_SET_LATIN
};

class Font:
    public Managed<Font> {

};

}
