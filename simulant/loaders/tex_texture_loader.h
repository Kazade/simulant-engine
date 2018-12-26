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


/* .tex is a texure file format designed for the Dreamcast and it's generated via the texconv utility:
 *  https://github.com/tvspelsfreak/texconv
 */

#pragma once

#include <memory>
#include <istream>
#include <vector>

#include "texture_loader.h"
#include "../unicode.h"

namespace smlt {
namespace loaders {

class TexTextureLoader : public BaseTextureLoader {
public:
    TexTextureLoader(const unicode& filename, std::shared_ptr<std::istream> data):
        BaseTextureLoader(filename, data) {}

private:
    TextureLoadResult do_load(const std::vector<uint8_t> &buffer) override;
};

class TexTextureLoaderType : public LoaderType {
public:
    TexTextureLoaderType() {
        // Always add the texture hint
        add_hint(LOADER_HINT_TEXTURE);
    }

    ~TexTextureLoaderType() {}

    unicode name() override { return "tex_texture"; }
    bool supports(const unicode& filename) const override {
        return filename.lower().contains(".tex");
    }

    Loader::ptr loader_for(const unicode& filename, std::shared_ptr<std::istream> data) const override {
        return Loader::ptr(new TexTextureLoader(filename, data));
    }
};

}
}
