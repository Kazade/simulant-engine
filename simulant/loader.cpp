//
//   Copyright (c) 2011-2017 Luke Benstead https://simulant-engine.appspot.com
//
//     This file is part of Simulant.
//
//     Simulant is free software: you can redistribute it and/or modify
//     it under the terms of the GNU General Public License as published by
//     the Free Software Foundation, either version 3 of the License, or
//     (at your option) any later version.
//
//     Simulant is distributed in the hope that it will be useful,
//     but WITHOUT ANY WARRANTY; without even the implied warranty of
//     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//     GNU General Public License for more details.
//
//     You should have received a copy of the GNU General Public License
//     along with Simulant.  If not, see <http://www.gnu.org/licenses/>.
//

#include "deps/kazlog/kazlog.h"
#include "loader.h"
#include "texture.h"

namespace smlt {

Loader::~Loader() {

}

namespace loaders {

void BaseTextureLoader::into(Loadable& resource, const LoaderOptions& options) {
    Loadable* res_ptr = &resource;
    Texture* tex = dynamic_cast<Texture*>(res_ptr);
    assert(tex && "You passed a Resource that is not a texture to the texture loader");

    std::vector<unsigned char> buffer(
        (std::istreambuf_iterator<char>(*this->data_)),
        std::istreambuf_iterator<char>()
    );

    auto result = do_load(buffer);

    /* Respect the auto_upload option if it exists*/
    bool auto_upload = true;
    if(options.count("auto_upload")) {
        auto_upload = smlt::any_cast<bool>(options.at("auto_upload"));
    }

    if (result.data.empty()) {
        L_ERROR(_F("Unable to load texture with name: {0}").format(filename_));
        throw std::runtime_error("Couldn't load the file: " + filename_.encode());
    } else {
        tex->set_source(filename_);
        tex->set_format(result.format, result.texel_type);
        tex->resize(result.width, result.height);
        tex->data().assign(result.data.begin(), result.data.end());
        tex->set_auto_upload(auto_upload);

        if(format_stored_upside_down()) {
            tex->flip_vertically();
        }
    }
}

}
}
