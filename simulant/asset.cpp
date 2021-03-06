//
//   Copyright (c) 2011-2017 Luke Benstead https://simulant-engine.appspot.com
//
//     This file is part of Simulant.
//
//     Simulant is free software: you can redistribute it and/or modify
//     it under the terms of the GNU Lesser General Public License as published by
//     the Free Software Foundation, either version 3 of the License, or
//     (at your option) any later version.
//
//     Simulant is distributed in the hope that it will be useful,
//     but WITHOUT ANY WARRANTY; without even the implied warranty of
//     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//     GNU Lesser General Public License for more details.
//
//     You should have received a copy of the GNU Lesser General Public License
//     along with Simulant.  If not, see <http://www.gnu.org/licenses/>.
//

#include "asset.h"
#include "asset_manager.h"

namespace smlt {

Asset::Asset(AssetManager* manager):
    manager_(manager),
    created_(std::chrono::system_clock::now()) {
}

Asset::~Asset() {

}

int Asset::age() const {
    return std::chrono::duration_cast<std::chrono::seconds>(
                created_ - std::chrono::system_clock::now()
    ).count();
}

void Asset::set_garbage_collection_method(GarbageCollectMethod method) {
    manager_->set_garbage_collection_method(this, method);
}

Asset::Asset(const Asset& rhs):
    manager_(rhs.manager_),
    created_(std::chrono::system_clock::now()),
    data_(rhs.data_) {
}

Asset& Asset::operator=(const Asset& rhs) {
    if(&rhs == this) return *this;

    // We intentionally don't copy the created timestamp
    manager_ = rhs.manager_;
    data_ = rhs.data_;

    return *this;
}


}
