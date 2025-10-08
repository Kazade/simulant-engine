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

#ifndef IDENTIFIABLE_H
#define IDENTIFIABLE_H

#include <cassert>

namespace smlt {
namespace generic {

template<typename IDType>
class Identifiable {
public:
    typedef IDType id_type;

    Identifiable(IDType id):
        id_(id) {

        assert(id > 0);
    }

    virtual ~Identifiable() {}

    IDType id() const { return id_; }

    virtual bool operator==(const Identifiable<IDType>& rhs) const {
        return id() == rhs.id();
    }

    virtual bool operator<(const Identifiable<IDType>& rhs) const {
        return id() < rhs.id();
    }

private:
    IDType id_ = 0;
};

}
}

#endif // IDENTIFIABLE_H
