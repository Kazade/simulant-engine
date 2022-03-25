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

#ifndef PARTICLE_SCRIPT_H
#define PARTICLE_SCRIPT_H

#include "../loader.h"

namespace smlt {
namespace loaders {

class ParticleScriptLoader : public Loader {
public:
    ParticleScriptLoader(const Path& filename, std::shared_ptr<std::istream> data):
        Loader(filename, data) {}

    bool into(Loadable& resource, const LoaderOptions& options = LoaderOptions());
};

class ParticleScriptLoaderType : public LoaderType {
public:
    ParticleScriptLoaderType() {

    }

    virtual ~ParticleScriptLoaderType() {}

    const char* name() override { return "particle"; }
    bool supports(const Path& filename) const override {
        return filename.ext() == ".spart" || filename.ext() == ".kglp";
    }

    Loader::ptr loader_for(const Path& filename, std::shared_ptr<std::istream> data) const override {
        return Loader::ptr(new ParticleScriptLoader(filename, data));
    }
};

}
}

#endif // PARTICLE_SCRIPT_H
