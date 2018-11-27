/* *   Copyright (c) 2011-2017 Luke Benstead https://simulant-engine.appspot.com
 *
 *     This file is part of Simulant.
 *
 *     Simulant is free software: you can redistribute it and/or modify
 *     it under the terms of the GNU Lesser General Public License as published by
 *     the Free Software Foundation, either version 3 of the License, or
 *     (at your option) any later version.
 *
 *     Simulant is distributed in the hope that it will be useful,
 *     but WITHOUT ANY WARRANTY; without even the implied warranty of
 *     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *     GNU Lesser General Public License for more details.
 *
 *     You should have received a copy of the GNU Lesser General Public License
 *     along with Simulant.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef CREATOR_H
#define CREATOR_H

#include <memory>

namespace smlt {
namespace generic {

template<typename T>
class Creator {
public:
    static std::shared_ptr<T> create() {
        return std::shared_ptr<T>(new T());
    }

    template<typename U>
    static std::shared_ptr<T> create(U& u) {
        return std::shared_ptr<T>(new T(u));
    }
};

}

}

#endif // CREATOR_H
