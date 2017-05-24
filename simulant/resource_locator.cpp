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

#include <fstream>

#include "deps/kfs/kfs.h"
#include "deps/kazlog/kazlog.h"
#include "resource_locator.h"

#ifdef __ANDROID__
#include <SDL_rwops.h>
#endif

namespace smlt {

ResourceLocator::ResourceLocator() {
#ifndef __ANDROID__
    //Android can't find the executable directory in release mode, but can in debug!
    resource_path_.push_back(find_executable_directory()); //Make sure the directory the executable lives is on the resource path
    resource_path_.push_back(find_working_directory()); //Add the working directory (might be different)
#endif
    resource_path_.push_back("/usr/local/share"); //Look in /usr/share (smlt files might be installed to /usr/share/smlt)
    resource_path_.push_back("/usr/share"); //Look in /usr/share (smlt files might be installed to /usr/share/smlt)
}

void ResourceLocator::add_search_path(const unicode& path) {
    resource_path_.push_back(path);
}

unicode ResourceLocator::locate_file(const unicode &filename) const {
    /**
      Locates a file on one of the resource paths, throws an IOError if the file
      cannot be found
    */
#ifdef __ANDROID__
    //On Android we use SDL_RWops which reads from the APK
    SDL_RWops* ops = SDL_RWFromFile(filename.encode().c_str(), "rb");
    if(ops) {
        //If we could open the file, return the filename
        SDL_RWclose(ops);
        return filename;
    }

#else
    if(kfs::path::exists(filename.encode())) { //Absolute path
        return kfs::path::abs_path(filename.encode());
    }

    for(unicode path: resource_path_) {
        auto full_path = kfs::path::join(path.encode(), filename.encode());
        L_DEBUG(_F("Checking {0}").format(full_path));
        if(kfs::path::exists(full_path)) {
            return kfs::path::abs_path(full_path);
        }
    }
#endif
    throw ResourceMissingError("Unable to find file: " + filename.encode());
}

std::shared_ptr<std::stringstream> ResourceLocator::read_file(const unicode& filename) {
#ifdef __ANDROID__
    //If we're on Android, don't bother trying to locate the file, just try to load it from the APK
    std::shared_ptr<std::stringstream> result = std::make_shared<std::stringstream>();
    SDL_RWops* ops = SDL_RWFromFile(filename.encode().c_str(), "r");
    if(ops) {
        //If we could open the file, return the filename
        SDL_RWseek(ops, 0, SEEK_END);
        int length = SDL_RWtell(ops);
        SDL_RWseek(ops, 0, SEEK_SET);

        std::vector<unsigned char> data(length); //Make room for all the data
        SDL_RWread(ops, &data[0], sizeof(unsigned char), length);
        SDL_FreeRW(ops);

        std::string str(data.begin(), data.end());
        //Populate the stringstream
        (*result) << str;
        return result;
    } else {
        L_ERROR("There was an error loading the specified file");
        throw IOError(_u("Unable to load file: ") + filename);
    }
    SDL_FreeRW(ops);
#else
    unicode path = locate_file(filename);

    std::ifstream file_in(path.encode());
    if(!file_in) {
        throw ResourceMissingError("Unable to load file: " + filename.encode());
    }

    std::shared_ptr<std::stringstream> result(new std::stringstream);
    (*result) << file_in.rdbuf();
    return result;
#endif
}

std::vector<std::string> ResourceLocator::read_file_lines(const unicode &filename) {
    unicode path = locate_file(filename);

    std::ifstream file_in(path.encode().c_str());
    if(!file_in) {
        throw ResourceMissingError("Unable to load file: " + filename.encode());
    }

    std::vector<std::string> results;
    std::string line;
    while(std::getline(file_in, line)) {
        results.push_back(line);
    }
    return results;
}

unicode ResourceLocator::find_executable_directory() {
    return kfs::exe_dirname();
}

unicode ResourceLocator::find_working_directory() {
    return kfs::get_cwd();
}

}
