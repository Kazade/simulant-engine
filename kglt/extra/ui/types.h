#ifndef TYPES_H
#define TYPES_H

#include "../../generic/unique_id.h"

namespace kglt {
namespace extra {
namespace ui {

typedef UniqueID<100> LabelID;
typedef UniqueID<101> ContainerID;
typedef UniqueID<102> TextInputID;
typedef UniqueID<103> ButtonID;

class Label;
class Container;
class TextInput;
class Button;

}
}
}
#endif // TYPES_H
