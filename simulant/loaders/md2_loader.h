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

#include <memory>
#include "../loader.h"

namespace smlt {
namespace loaders {

class MD2Loader : public Loader {
public:
    /* This is the max number of expanded frames to keep in memory */
    static uint16_t MAX_RESIDENT_FRAMES;

    MD2Loader(const Path& filename, std::shared_ptr<std::istream> data):
        Loader(filename, data) {}

    void into(Loadable& resource, const LoaderOptions& options=LoaderOptions());
};

class MD2LoaderType : public LoaderType {
public:
    MD2LoaderType() {

    }

    virtual ~MD2LoaderType() {}

    const char* name() override {
        return "md2";
    }

    bool supports(const Path& filename) const override {
        return smlt::lower_case(filename.ext()) == ".md2";
    }

    Loader::ptr loader_for(const Path& filename, std::shared_ptr<std::istream> data) const override {
        return Loader::ptr(new MD2Loader(filename, data));
    }
};


}
}
