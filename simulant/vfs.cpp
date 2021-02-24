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

#include <fstream>
#include <string>
#include <iostream>
#include <sstream>

#include "deps/kfs/kfs.h"
#include "logging.h"
#include "vfs.h"
#include "window.h"
#include "renderers/renderer.h"
#include "loader.h"
#include "platform.h"
#include "streams/file_ifstream.h"

#ifdef __ANDROID__
#include <SDL_rwops.h>
#endif

namespace smlt {

VirtualFileSystem::VirtualFileSystem(Window *window):
    window_(window) {

    resource_path_.push_back(find_working_directory()); //Add the working directory (might be different)

#ifndef __ANDROID__
    //Android can't find the executable directory in release mode, but can in debug!
    resource_path_.push_back(find_executable_directory()); //Make sure the directory the executable lives is on the resource path
#endif

#ifdef __DREAMCAST__
    // On the Dreamcast, always add the CD and pc folder as a search path
    resource_path_.push_back("/pc");
    resource_path_.push_back("/cd");
#endif

#ifdef __PSP__
    resource_path_.push_back(".");
    resource_path_.push_back("umd0:");
    resource_path_.push_back("ms0:");
    resource_path_.push_back("disc0:");
#endif

#ifdef __LINUX__
    resource_path_.push_back("/usr/local/share"); //Look in /usr/share (smlt files might be installed to /usr/share/smlt)
    resource_path_.push_back("/usr/share"); //Look in /usr/share (smlt files might be installed to /usr/share/smlt)
#endif
}

bool VirtualFileSystem::add_search_path(const unicode& path) {
    unicode new_path(kfs::path::abs_path(path.encode()));

    if(std::find(resource_path_.begin(), resource_path_.end(), new_path) != resource_path_.end()) {
        return false;
    }

    resource_path_.push_back(new_path);
    return true;
}

void VirtualFileSystem::remove_search_path(const unicode& path) {
    resource_path_.erase(std::remove(resource_path_.begin(), resource_path_.end(), path), resource_path_.end());
}

unicode VirtualFileSystem::locate_file(const unicode &filename) const {
    /**
      Locates a file on one of the resource paths, throws an IOError if the file
      cannot be found
    */

    S_DEBUG("Locating file: {0}", filename);

    std::string final_name = filename.replace(
        "${RENDERER}",
        window_->renderer->name()
    ).replace(
        "${PLATFORM}",
        window_->platform->name()
    ).encode();

    final_name = kfs::path::norm_path(final_name);

#ifdef __ANDROID__
    //On Android we use SDL_RWops which reads from the APK
    SDL_RWops* ops = SDL_RWFromFile(final_name.c_str(), "rb");
    if(ops) {
        //If we could open the file, return the filename
        SDL_RWclose(ops);
        return filename;
    }
#else
    auto abs_final_name = kfs::path::abs_path(final_name);

    S_DEBUG("Checking existence...");
    if(kfs::path::exists(abs_final_name)) {
        S_DEBUG("Located file: {0}", abs_final_name);
        return abs_final_name;
    }

    S_DEBUG("Searching resource paths...");
    for(unicode path: resource_path_) {
        auto full_path = kfs::path::norm_path(
            kfs::path::join(path.encode(), final_name)
        );

        S_DEBUG("Trying path: {0}", full_path);
        if(kfs::path::exists(full_path)) {
            S_DEBUG("Found: {0}", full_path);
            return full_path;
        }
    }
#endif
    S_ERROR("Unable to find file: {0}", final_name);
    throw AssetMissingError("Unable to find file: " + final_name);
}

std::shared_ptr<std::istream> VirtualFileSystem::open_file(const unicode& filename) {
    unicode path = locate_file(filename);

    auto buf = std::make_shared<FileStreamBuf>(path.encode(), "rb");
    auto file_in = std::make_shared<FileIfstream>(buf);

    return file_in;
}

std::shared_ptr<std::stringstream> VirtualFileSystem::read_file(const unicode& filename) {
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
        S_ERROR("There was an error loading the specified file");
        throw AssetMissingError("Unable to load file: " + filename.encode());
    }
    SDL_FreeRW(ops);
#else

    auto file_in = open_file(filename);
    std::shared_ptr<std::stringstream> result(new std::stringstream);
    (*result) << file_in->rdbuf();
    return result;
#endif
}

std::vector<std::string> VirtualFileSystem::read_file_lines(const unicode &filename) {
    unicode path = locate_file(filename);

    // Load as binary and let portable_getline do its thing
    std::ifstream file_in(path.encode().c_str(), std::ios::in | std::ios::binary);

    if(!file_in) {
        S_ERROR("Unable to load file: {0}", filename);
        throw AssetMissingError("Unable to load file: " + filename.encode());
    }

    std::vector<std::string> results;
    std::string line;
    while(portable_getline(file_in, line)) {
        results.push_back(line);
    }
    return results;
}

unicode VirtualFileSystem::find_executable_directory() {
    return kfs::exe_dirname();
}

unicode VirtualFileSystem::find_working_directory() {
    return kfs::get_cwd();
}

}
