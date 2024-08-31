//
//   Copyright (c) 2011-2017 Luke Benstead https://simulant-engine.appspot.com
//
//     This file is part of Simulant.
//
//     Simulant is free software: you can redistribute it and/or modify
//     it under the terms of the GNU Lesser General Public License as published
//     by the Free Software Foundation, either version 3 of the License, or (at
//     your option) any later version.
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
#include <iostream>
#include <sstream>
#include <string>

#include "application.h"
#include "loader.h"
#include "logging.h"
#include "platform.h"
#include "renderers/renderer.h"
#include "streams/file_ifstream.h"
#include "utils/kfs.h"
#include "vfs.h"
#include "window.h"

#ifdef __ANDROID__
#define ANDROID_ASSET_DIR_PREFIX "/android_asset"
#define ANDROID_ASSET_DIR_PREFIX_SLASH "/android_asset/"
#endif

namespace smlt {

VirtualFileSystem::VirtualFileSystem() {

    auto cwd = find_working_directory();
    S_INFO("CWD: {0}", cwd.str());
    if(!cwd.str().empty()) {
        add_search_path(cwd); // Add the working directory (might be different)
    }

#ifdef __ANDROID__
    add_search_path(".");
#else
    // Android can't find the executable directory in release mode, but can in
    // debug!
    auto ed = find_executable_directory();
    add_search_path(ed); // Make sure the directory the executable lives is on
                         // the resource path
#endif

#ifdef __DREAMCAST__
    // On the Dreamcast, always add the CD and pc folder as a search path
    add_search_path("/cd");
    add_search_path("/pc");
#endif

#ifdef __PSP__
    add_search_path(".");
    add_search_path("umd0:");
    add_search_path("ms0:");
    add_search_path("disc0:");
#endif

#ifdef __linux__
    add_search_path("/usr/local/share"); // Look in /usr/share (smlt files might
                                         // be installed to /usr/share/smlt)
    add_search_path("/usr/share"); // Look in /usr/share (smlt files might be
                                   // installed to /usr/share/smlt)
#endif

    /* In any standard project there are assets in the 'assets' directory.
     * So we add that in here as a standard location in all
     * root paths */
    auto copy = resource_path_;
    for(auto& path: copy) {
#ifdef __ANDROID__
        if(path.str().find(ANDROID_ASSET_DIR_PREFIX) == 0) {
            path =
                path.str().substr(std::string(ANDROID_ASSET_DIR_PREFIX).size());
        }
#endif
        add_search_path(kfs::path::join(path.str(), "assets"));
    }

    /* Finally add the simulant directory to all search paths as projects
     * have assets/simulant */
    copy = resource_path_;
    for(auto& path: copy) {
#ifdef __ANDROID__
        if(path.str().find(ANDROID_ASSET_DIR_PREFIX) == 0) {
            path =
                path.str().substr(std::string(ANDROID_ASSET_DIR_PREFIX).size());
        }
#endif

        add_search_path(kfs::path::join(path.str(), "simulant"));
    }
}

bool VirtualFileSystem::insert_search_path(uint32_t index, const Path& path) {
    Path new_path(kfs::path::abs_path(path.str()));

    if(path.str().empty() || path.str() == "/") {
        return false;
    }

    if(std::find(resource_path_.begin(), resource_path_.end(), new_path) !=
       resource_path_.end()) {
        return false;
    }

    /* If someone passed in value gte than the length of the list, then we
     * insert at the end, else we insert at the appropriate position */
    auto it = (index >= resource_path_.size())
                  ? resource_path_.end()
                  : std::next(resource_path_.begin(), index);
    resource_path_.insert(it, path);
    clear_location_cache();
    return true;
}

bool VirtualFileSystem::add_search_path(const Path& path) {
    Path new_path(kfs::path::abs_path(path.str()));

#ifdef __ANDROID__
    /* Android is special. Assets can only be loaded using relative paths, so
     * we use this as a placeholder resource path, and we special case it when
     * we do existence checks etc. It's depressing how bad Android is at this
     * stuff */
    new_path =
        kfs::path::norm_path(ANDROID_ASSET_DIR_PREFIX_SLASH + new_path.str());
#endif

    S_INFO("Adding path: {0}", new_path);

    if(path.str().empty() || path.str() == "/") {
        return false;
    }

    if(std::find(resource_path_.begin(), resource_path_.end(), new_path) !=
       resource_path_.end()) {
        return false;
    }

    resource_path_.push_back(new_path);
    clear_location_cache();
    return true;
}

void VirtualFileSystem::remove_search_path(const Path& path) {
    resource_path_.erase(
        std::remove(resource_path_.begin(), resource_path_.end(), path),
        resource_path_.end());
}

std::size_t VirtualFileSystem::location_cache_size() const {
    return location_cache_.size();
}

void VirtualFileSystem::clear_location_cache() {
    location_cache_.clear();
}

void VirtualFileSystem::set_location_cache_limit(std::size_t entries) {
    location_cache_.set_max_size(entries);
}

static const char* RENDERER_PLACEHOLDER = "${RENDERER}";
static const char* PLATFORM_PLACEHOLDER = "${PLATFORM}";

optional<Path> VirtualFileSystem::locate_file(const Path& filename,
                                              bool use_cache,
                                              bool fail_silently) const {

    if(read_blocking_enabled_) {
        S_ERROR("Attempted to locate {0} while read blocking was enabled",
                filename.str());
        return optional<Path>();
    }

    const Path DOES_NOT_EXIST = "";

    S_DEBUG("Locating file: {0}", filename);

    /* Replace any placeholders */

    auto window = get_app()->window.get();

    std::vector<Path> names_to_try;

    /* Here, we need to do the following. Given this example:
     *
     * /my/${RENDERER}/path/${PLATFORM}/
     *
     * We need the search to be:
     *
     * /my/psp/path/psp/
     * /my/psp/path/
     * /my/path/psp/
     * /my/path/
     */

    auto platform = get_platform();
    std::multimap<std::string, std::string> replacements = {
        {RENDERER_PLACEHOLDER,
         (window && window->renderer) ? window->renderer->name() : "__ERROR__"},
        {PLATFORM_PLACEHOLDER, platform->name()                               },
        {RENDERER_PLACEHOLDER, ""                                             },
        {PLATFORM_PLACEHOLDER, ""                                             },
    };

    for(auto& p: replacements) {
        for(auto& q: replacements) {
            if(p.first == q.first) {
                continue;
            }

            auto repl = unicode(filename.str())
                            .replace(p.first, p.second)
                            .replace(q.first, q.second);

            auto new_path = Path(kfs::path::norm_path(repl.encode()));
            if(std::find(names_to_try.begin(), names_to_try.end(), new_path) ==
               names_to_try.end()) {
                names_to_try.push_back(new_path);
            }
        }
    }

    for(const auto& final_name: names_to_try) {

        if(use_cache) {
            auto ret = location_cache_.get(final_name);
            if(ret) {
                /* An empty Path means that the file doesn't exist */
                if(ret.value() == DOES_NOT_EXIST) {
                    return optional<Path>();
                } else {
                    return ret.value();
                }
            }
        }

        Path abs_final_name(kfs::path::abs_path(final_name.str()));

        S_DEBUG("Checking existence...");
        if(kfs::path::exists(abs_final_name.str())) {
            S_INFO("Located file: {0}", abs_final_name);

            if(use_cache) {
                location_cache_.insert(final_name, abs_final_name);
            }

            return abs_final_name;
        }

        S_DEBUG("Searching resource paths...");
        for(const Path& path: resource_path_) {
            auto full_path = kfs::path::norm_path(
                kfs::path::join(path.str(), final_name.str()));

#ifdef __ANDROID__
            /* Hackity hack. Wipe out the /android_asset placeholder folder
             * to make the path relative */
            S_DEBUG("Before manipulation {0}", full_path);

            if(full_path.find(ANDROID_ASSET_DIR_PREFIX) == 0) {
                full_path = full_path.substr(
                    std::string(ANDROID_ASSET_DIR_PREFIX).size());

                /* Remove any leading slashes, all paths must be relative */
                if(!full_path.empty() && full_path[0] == '/') {
                    full_path = full_path.substr(1);
                }
            }
#endif

            S_DEBUG("Trying path: {0}", full_path);
            if(kfs::path::exists(full_path)) {
                S_INFO("Located file: {0}", full_path);

                if(use_cache) {
                    location_cache_.insert(final_name, full_path);
                }

                return optional<Path>(full_path);
            }
        }

        if(use_cache) {
            location_cache_.insert(final_name, DOES_NOT_EXIST);
        }
    }

    if(!fail_silently) {
        S_WARN("Unable to find file: {0}", filename.str());
    }

    return optional<Path>();
}

std::shared_ptr<std::istream>
    VirtualFileSystem::open_file(const Path& filename) {
    auto p = locate_file(filename);

    if(!p.has_value()) {
        return std::shared_ptr<std::istream>();
    }

    auto path = p.value();
    auto buf = std::make_shared<FileStreamBuf>(path.str(), "rb");
    auto file_in = std::make_shared<FileIfstream>(buf);

    return file_in;
}

std::shared_ptr<std::stringstream>
    VirtualFileSystem::read_file(const Path& filename) {
    auto file_in = open_file(filename);
    if(!file_in) {
        return std::shared_ptr<std::stringstream>();
    }

    std::shared_ptr<std::stringstream> result(new std::stringstream);
    (*result) << file_in->rdbuf();
    return result;
}

std::vector<std::string>
    VirtualFileSystem::read_file_lines(const Path& filename) {
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

} // namespace smlt
