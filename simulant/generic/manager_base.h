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

#ifndef MANAGER_BASE_H
#define MANAGER_BASE_H

#include <memory>
#include <unordered_map>
#include <stdexcept>
#include <thread>
#include <mutex>

namespace smlt {
namespace generic  {

template<typename T>
class IncrementalGetNextID {
public:
    uint32_t operator()() {
        static uint32_t counter = 0;
        return ++counter;
    }
};

class ObjectLookupError : public std::runtime_error {
public:
    ObjectLookupError(const std::string& what):
        std::runtime_error(what) {
    }
};

}
}

#endif // MANAGER_BASE_H
