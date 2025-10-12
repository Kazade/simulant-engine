/* *   Copyright (c) 2011-2017 Luke Benstead https://simulant-engine.appspot.com
 *
 *     This file is part of Simulant.
 *
 *     Simulant is free software: you can redistribute it and/or modify
 *     it under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 *     Simulant is distributed in the hope that it will be useful,
 *     but WITHOUT ANY WARRANTY; without even the implied warranty of
 *     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *     GNU Lesser General Public License for more details.
 *
 *     You should have received a copy of the GNU Lesser General Public License
 *     along with Simulant.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "../loader.h"
#include "../threads/mutex.h"

namespace smlt {
namespace loaders {

class PNGLoader: public BaseTextureLoader {
public:
    PNGLoader(const Path& filename, std::shared_ptr<std::istream> data) :
        BaseTextureLoader(filename, data) {}

private:
    bool do_load(std::shared_ptr<FileIfstream> stream,
                 Texture* result) override;

    bool do_load(const std::vector<uint8_t>& data, Texture* result) override;

    thread::Mutex lock_;
};

class PNGLoaderType: public LoaderType {
public:
    PNGLoaderType() {
        // Always add the texture hint
        add_hint(LOADER_HINT_TEXTURE);
    }

    virtual ~PNGLoaderType() {}

    const char* name() override {
        return "texture";
    }
    bool supports(const Path& filename) const override {
        return filename.ext() == ".png";
    }

    Loader::ptr loader_for(const Path& filename,
                           std::shared_ptr<std::istream> data) const override {
        return Loader::ptr(new PNGLoader(filename, data));
    }
};

} // namespace loaders
} // namespace smlt
