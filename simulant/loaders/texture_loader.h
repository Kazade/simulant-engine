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

#ifndef SIMULANT_TGA_LOADER_H
#define SIMULANT_TGA_LOADER_H

#include "../loader.h"

namespace smlt {
namespace loaders {

class TextureLoader : public BaseTextureLoader {
public:
    TextureLoader(const unicode& filename, std::shared_ptr<std::istream> data):
        BaseTextureLoader(filename, data) {}

private:
    TextureLoadResult do_load(const std::vector<uint8_t> &buffer) override;
};

class TextureLoaderType : public LoaderType {
public:
    TextureLoaderType() {
        // Always add the texture hint
        add_hint(LOADER_HINT_TEXTURE);
    }

    ~TextureLoaderType() {}

    unicode name() override { return "texture"; }
    bool supports(const unicode& filename) const override {
        return filename.lower().contains(".tga") || filename.lower().contains(".png") || filename.lower().contains(".jpg");
    }

    Loader::ptr loader_for(const unicode& filename, std::shared_ptr<std::istream> data) const override {
        return Loader::ptr(new TextureLoader(filename, data));
    }
};

}
}

#endif
