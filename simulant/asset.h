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

#pragma once

#include <cassert>
#include <string>
#include <chrono>
#include <memory>

#include "generic/property.h"
#include "generic/data_carrier.h"
#include "generic/object_manager.h"
#include "interfaces/nameable.h"
#include "threads/mutex.h"

namespace smlt {

class AssetManager;

class Asset:
    public virtual Nameable {

public:
    friend class AssetManager;

    Asset(AssetManager* manager);

    virtual ~Asset();

    AssetManager& asset_manager() { assert(manager_); return *manager_; }
    const AssetManager& asset_manager() const { assert(manager_); return *manager_; }

    int age() const;

    void set_garbage_collection_method(GarbageCollectMethod method);

    Property<generic::DataCarrier Asset::*> data = {this, &Asset::data_};

protected:
    Asset(const Asset& rhs);
    Asset& operator=(const Asset& rhs);
private:
    AssetManager* manager_;

    std::chrono::time_point<std::chrono::system_clock> created_;

    generic::DataCarrier data_;
};

}

