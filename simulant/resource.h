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

#ifndef RESOURCE_H
#define RESOURCE_H

#include <cassert>
#include <mutex>
#include <string>
#include <chrono>

#include "generic/property.h"
#include "generic/data_carrier.h"
#include "generic/object_manager.h"

namespace smlt {

class AssetManager;

class Resource {
public:
    friend class AssetManager;

    Resource(AssetManager* manager):
        manager_(manager) {
        created_ = std::chrono::system_clock::now();
    }

    virtual ~Resource() {}

    AssetManager& resource_manager() { assert(manager_); return *manager_; }
    const AssetManager& resource_manager() const { assert(manager_); return *manager_; }

    int age() const {
        return std::chrono::duration_cast<std::chrono::seconds>(
            created_ - std::chrono::system_clock::now()
        ).count();
    }

    void set_garbage_collection_method(GarbageCollectMethod method);

    Property<Resource, generic::DataCarrier> data = {this, &Resource::data_};

protected:
    Resource(const Resource& rhs):
        manager_(rhs.manager_),
        created_(std::chrono::system_clock::now()),
        data_(rhs.data_) {

    }
private:
    AssetManager* manager_;

    std::chrono::time_point<std::chrono::system_clock> created_;

    generic::DataCarrier data_;
};

}
#endif // RESOURCE_H
