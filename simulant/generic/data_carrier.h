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

#ifndef USER_DATA_CARRIER_H
#define USER_DATA_CARRIER_H

#include <unordered_map>
#include "any/any.h"

namespace smlt {
namespace generic {

class NoSuchData : public std::runtime_error {
public:
    NoSuchData(const std::string& what):
        std::runtime_error(what) {}
};

class DataCarrier {
public:
    virtual ~DataCarrier() {}

    template<typename T>
    void stash(T thing, const std::string& identifier) {
        things_[identifier] = smlt::any(thing);
    }

    bool exists(const std::string& identifier) const {
        return things_.count(identifier);
    }

    template<typename T>
    T get(const std::string& identifier) const {
        if(!exists(identifier)) {
            throw NoSuchData(identifier);
        }
        return smlt::any_cast<T>(things_.at(identifier));
    }

    void unstash(const std::string& identifier) {
        if(!exists(identifier)) return;

        things_.erase(identifier);
    }

private:
    std::unordered_map<std::string, smlt::any> things_;
};

}
}

#endif // USER_DATA_CARRIER_H
