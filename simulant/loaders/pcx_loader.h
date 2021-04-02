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


#include "../loader.h"

namespace smlt {
namespace loaders {

class PCXLoader : public BaseTextureLoader {
public:
    PCXLoader(const Path& filename, std::shared_ptr<std::istream> data):
        BaseTextureLoader(filename, data) {}

private:
    TextureLoadResult do_load(std::shared_ptr<FileIfstream> stream) override;
};

class PCXLoaderType : public LoaderType {
public:
    PCXLoaderType() {
        // Always add the texture hint
        add_hint(LOADER_HINT_TEXTURE);
    }

    virtual ~PCXLoaderType() {}

    const char* name() override { return "pcx_texture"; }
    bool supports(const Path& filename) const override {
        return filename.ext() == ".pcx";
    }

    Loader::ptr loader_for(const Path& filename, std::shared_ptr<std::istream> data) const override {
        return Loader::ptr(new PCXLoader(filename, data));
    }
};

}
}
