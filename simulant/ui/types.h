/* *   Copyright (c) 2011-2017 Luke Benstead https://simulant-engine.appspot.com
 *
 *     This file is part of Simulant.
 *
 *     Simulant is free software: you can redistribute it and/or modify
 *     it under the terms of the GNU General Public License as published by
 *     the Free Software Foundation, either version 3 of the License, or
 *     (at your option) any later version.
 *
 *     Simulant is distributed in the hope that it will be useful,
 *     but WITHOUT ANY WARRANTY; without even the implied warranty of
 *     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *     GNU General Public License for more details.
 *
 *     You should have received a copy of the GNU General Public License
 *     along with Simulant.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef UI_TYPES_H
#define UI_TYPES_H

#include "smlt/generic/unique_id.h"

namespace smlt {
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
