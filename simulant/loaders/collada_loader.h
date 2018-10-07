#pragma once

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

#pragma once

#include <memory>
#include "../loader.h"

class TiXmlElement;

namespace smlt {

class Stage;
class StageNode;
class Mesh;

namespace loaders {

class ColladaLoader : public Loader {
public:
    ColladaLoader(const unicode& filename, std::shared_ptr<std::istream> data):
        Loader(filename, data) {}

    void into(Loadable& resource, const LoaderOptions& options=LoaderOptions());

private:
    void load_into_stage(Stage* scene, const LoaderOptions& options);
    void load_into_mesh(Mesh* mesh, const LoaderOptions& options);

    void handle_node(TiXmlElement* pnode, Stage* stage, smlt::StageNode *parent);
    void handle_geometry(TiXmlElement* geometry, Stage *stage);

    std::unordered_map<std::string, MeshID> meshes_;
};

class ColladaLoaderType : public LoaderType {
public:
    ColladaLoaderType() {

    }

    ~ColladaLoaderType() {}

    unicode name() override {
        return "collada";
    }

    bool supports(const unicode& filename) const override {
        return filename.lower().ends_with(".dae");
    }

    Loader::ptr loader_for(const unicode& filename, std::shared_ptr<std::istream> data) const override {
        return Loader::ptr(new ColladaLoader(filename, data));
    }
};


}
}
