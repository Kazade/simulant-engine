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

#include "utils/kfs.h"
#include "logging.h"
#include "vfs.h"
#include "window.h"
#include "renderers/renderer.h"
#include "loader.h"
#include "platform.h"
#include "streams/file_ifstream.h"
#include "application.h"

#ifdef __ANDROID__
#include <SDL_rwops.h>
#endif

namespace smlt {

VirtualFileSystem::VirtualFileSystem() {

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

#ifdef __linux__
    resource_path_.push_back("/usr/local/share"); //Look in /usr/share (smlt files might be installed to /usr/share/smlt)
    resource_path_.push_back("/usr/share"); //Look in /usr/share (smlt files might be installed to /usr/share/smlt)
#endif

    /* In any standard project there are assets in the 'assets' directory.
     * So we add that in here as a standard location in all
     * root paths */
    auto copy = resource_path_;
    for(auto& path: copy) {
        resource_path_.push_back(
            kfs::path::join(path.str(), "assets")
        );
    }
}

bool VirtualFileSystem::add_search_path(const Path& path) {
    Path new_path(kfs::path::abs_path(path.str()));

    if(std::find(resource_path_.begin(), resource_path_.end(), new_path) != resource_path_.end()) {
        return false;
    }

    resource_path_.push_back(new_path);
    return true;
}

void VirtualFileSystem::remove_search_path(const Path& path) {
    resource_path_.erase(std::remove(resource_path_.begin(), resource_path_.end(), path), resource_path_.end());
}

optional<Path> VirtualFileSystem::locate_file(const Path &filename, bool fail_silently) const {
    /**
      Locates a file on one of the resource paths, throws an IOError if the file
      cannot be found
    */

    S_DEBUG("Locating file: {0}", filename);

    // FIXME: Don't use unicode!
    Path final_name(unicode(filename.str()).replace(
        "${RENDERER}",
        get_app()->window->renderer->name()
    ).replace(
        "${PLATFORM}",
        get_platform()->name()
    ).encode());

    final_name = kfs::path::norm_path(final_name.str());

#ifdef __ANDROID__
    //On Android we use SDL_RWops which reads from the APK
    SDL_RWops* ops = SDL_RWFromFile(final_name.str().c_str(), "rb");
    if(ops) {
        //If we could open the file, return the filename
        SDL_RWclose(ops);
        return filename;
    }
#else
    Path abs_final_name(kfs::path::abs_path(final_name.str()));

    S_DEBUG("Checking existence...");
    if(kfs::path::exists(abs_final_name.str())) {
        S_DEBUG("Located file: {0}", abs_final_name);
        return abs_final_name;
    }

    S_DEBUG("Searching resource paths...");
    for(const Path& path: resource_path_) {
        auto full_path = kfs::path::norm_path(
            kfs::path::join(path.str(), final_name.str())
        );

        S_DEBUG("Trying path: {0}", full_path);
        if(kfs::path::exists(full_path)) {
            S_DEBUG("Found: {0}", full_path);
            return optional<Path>(full_path);
        }
    }
#endif
    if(!fail_silently) {
        S_WARN("Unable to find file: {0}", final_name);
    }
    return optional<Path>();
}

std::shared_ptr<std::istream> VirtualFileSystem::open_file(const Path& filename) {
    auto p = locate_file(filename);

    if(!p.has_value()) {
        return std::shared_ptr<std::istream>();
    }

    auto path = p.value();
    auto buf = std::make_shared<FileStreamBuf>(path.str(), "rb");
    auto file_in = std::make_shared<FileIfstream>(buf);

    return file_in;
}

std::shared_ptr<std::stringstream> VirtualFileSystem::read_file(const Path& filename) {
#ifdef __ANDROID__
    //If we're on Android, don't bother trying to locate the file, just try to load it from the APK
    std::shared_ptr<std::stringstream> result = std::make_shared<std::stringstream>();
    SDL_RWops* ops = SDL_RWFromFile(filename.str().c_str(), "r");
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
        throw AssetMissingError("Unable to load file: " + filename.str());
    }
    SDL_FreeRW(ops);
#else

    auto file_in = open_file(filename);
    std::shared_ptr<std::stringstream> result(new std::stringstream);
    (*result) << file_in->rdbuf();
    return result;
#endif
}

std::vector<std::string> VirtualFileSystem::read_file_lines(const Path &filename) {
    auto p = locate_file(filename);

    if(!p) {
        // FIXME: Should this be optional<>?
        return std::vector<std::string>();
    }

    auto path = p.value();

    // Load as binary and let portable_getline do its thing
    std::ifstream file_in(path.str(), std::ios::in | std::ios::binary);

    if(!file_in) {
        S_ERROR("Unable to load file: {0}", filename);
        return std::vector<std::string>();
    }

    std::vector<std::string> results;
    std::string line;
    while(portable_getline(file_in, line)) {
        results.push_back(line);
    }
    return results;
}

Path VirtualFileSystem::find_executable_directory() {
    return kfs::exe_dirname();
}

Path VirtualFileSystem::find_working_directory() {
    return kfs::get_cwd();
}

}
