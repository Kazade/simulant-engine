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

#ifndef OBJ_LOADER_H
#define OBJ_LOADER_H

#include "../loader.h"

namespace smlt {
namespace loaders {

class OBJLoader : public Loader {
public:
    OBJLoader(const Path& filename, std::shared_ptr<std::istream> data):
        Loader(filename, data) {}

    void into(Loadable& resource, const LoaderOptions& options = LoaderOptions());
};

void parse_face(const std::string& input, int32_t& vertex_index, int32_t& tex_index, int32_t& normal_index);

class OBJLoaderType : public LoaderType {
public:
    OBJLoaderType() {

    }

    virtual ~OBJLoaderType() {}

    const char* name() override { return "obj"; }
    bool supports(const Path& filename) const override {
        return filename.ext() == ".obj";
    }

    Loader::ptr loader_for(const Path& filename, std::shared_ptr<std::istream> data) const override {
        return Loader::ptr(new OBJLoader(filename, data));
    }
};

}
}

#endif // OBJ_LOADER_H
