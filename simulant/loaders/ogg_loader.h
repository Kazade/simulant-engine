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

#ifndef OGG_LOADER_H
#define OGG_LOADER_H

#include <map>
#include <vector>
#include "../types.h"
#include "../loader.h"

namespace smlt {
namespace loaders {

class OGGLoader : public Loader {
public:
    OGGLoader(const Path& filename, std::shared_ptr<std::istream> data):
        Loader(filename, data) {}

    bool into(Loadable& resource, const LoaderOptions &options=LoaderOptions());

};

class OGGLoaderType : public LoaderType {
public:
    const char* name() { return "ogg"; }
    bool supports(const Path& filename) const {
        //FIXME: check magic
        return filename.ext() == ".ogg";
    }

    Loader::ptr loader_for(const Path& filename, std::shared_ptr<std::istream> data) const {
        return Loader::ptr(new OGGLoader(filename, data));
    }
};


}
}

#endif // OGG_LOADER_H
