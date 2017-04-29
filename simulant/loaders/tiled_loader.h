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

#ifndef TILED_LOADER_H
#define TILED_LOADER_H

#include "../loader.h"

namespace smlt {
namespace loaders {

class TiledLoader : public Loader {
public:
    TiledLoader(const unicode& filename, std::shared_ptr<std::stringstream> data):
        Loader(filename, data) {}

    void into(Loadable& resource, const LoaderOptions& options = LoaderOptions());
};

class TiledLoaderType : public LoaderType {
public:
    TiledLoaderType() {

    }

    ~TiledLoaderType() {}

    unicode name() override { return "tiled"; }
    bool supports(const unicode& filename) const override {        
        bool ret = filename.lower().ends_with(".tmx");
        return ret;
    }

    Loader::ptr loader_for(const unicode& filename, std::shared_ptr<std::stringstream> data) const override {
        return Loader::ptr(new TiledLoader(filename, data));
    }
};

}
}

#endif // TILED_LOADER_H
