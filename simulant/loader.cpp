//
//   Copyright (c) 2011-2017 Luke Benstead https://simulant-engine.appspot.com
//
//     This file is part of Simulant.
//
//     Simulant is free software: you can redistribute it and/or modify
//     it under the terms of the GNU Lesser General Public License as published by
//     the Free Software Foundation, either version 3 of the License, or
//     (at your option) any later version.
//
//     Simulant is distributed in the hope that it will be useful,
//     but WITHOUT ANY WARRANTY; without even the implied warranty of
//     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//     GNU Lesser General Public License for more details.
//
//     You should have received a copy of the GNU Lesser General Public License
//     along with Simulant.  If not, see <http://www.gnu.org/licenses/>.
//

#include "logging.h"
#include "loader.h"
#include "texture.h"

namespace smlt {

std::istream& portable_getline(std::istream& is, std::string& t) {
    t.clear();

    // The characters in the stream are read one-by-one using a std::streambuf.
    // That is faster than reading them one-by-one using the std::istream.
    // Code that uses streambuf this way must be guarded by a sentry object.
    // The sentry object performs various tasks,
    // such as thread synchronization and updating the stream state.

    std::istream::sentry se(is, true);
    std::streambuf* sb = is.rdbuf();

    for(;;) {
        int c = sb->sbumpc();
        switch (c) {
        case '\n':
            return is;
        case '\r':
            if(sb->sgetc() == '\n')
                sb->sbumpc();
            return is;
        case std::streambuf::traits_type::eof():
            // Also handle the case when the last line has no line ending
            if(t.empty())
                is.setstate(std::ios::eofbit);
            return is;
        default:
            t += (char)c;
        }
    }
}

Loader::~Loader() {

}

namespace loaders {


void BaseTextureLoader::into(Loadable& resource, const LoaderOptions& options) {
    Loadable* res_ptr = &resource;
    Texture* tex = dynamic_cast<Texture*>(res_ptr);
    assert(tex && "You passed a Resource that is not a texture to the texture loader");

    assert(data_);

    std::shared_ptr<FileIfstream> ifstream = std::dynamic_pointer_cast<FileIfstream>(
        data_
    );

    assert(ifstream);

    auto result = do_load(ifstream);

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
        tex->set_data(result.data);
        tex->set_auto_upload(auto_upload);
        if(format_stored_upside_down()) {
            tex->flip_vertically();
        }
    }
}

}
}
