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

#ifndef RESOURCE_LOCATOR_H
#define RESOURCE_LOCATOR_H

#include <list>
#include <vector>
#include <string>
#include <map>
#include <queue>

#include "generic/lru_cache.h"
#include "generic/optional.h"
#include "generic/managed.h"
#include "utils/unicode.h"
#include "path.h"

namespace smlt {

class Window;

class AssetMissingError : public std::runtime_error {
public:
    AssetMissingError(const std::string& what):
        std::runtime_error(what) {}
};

class VirtualFileSystem :
    public RefCounted<VirtualFileSystem> {

public:
    VirtualFileSystem();

    std::list<Path>& search_path() { return resource_path_; }

    optional<Path> locate_file(
        const Path& filename,
        bool use_cache=true,
        bool fail_silently=false
    ) const;
    std::shared_ptr<std::istream> open_file(const Path& filename);
    std::shared_ptr<std::stringstream> read_file(const Path& filename);
    std::vector<std::string> read_file_lines(const Path& filename);

    bool add_search_path(const Path& path);
    void remove_search_path(const Path& path);

    /* Returns the number of entries in the location cache */
    std::size_t location_cache_size() const;

    /* Purge the location cache of all entries */
    void clear_location_cache();

    /* Limit the location cache entries */
    void set_location_cache_limit(std::size_t entries);

    /** If read blocking is enabled, then all attempts to access the filesyste
     * will fail and an error will be logged. This is useful if you are
     * targetting a device that loads from a CD and plays CD-audio where a
     * read could stop the audio playing */
    void enable_read_blocking() {
        read_blocking_enabled_ = true;
    }

    void disable_read_blocking() {
        read_blocking_enabled_ = false;
    }

    bool read_blocking_enabled() const {
        return read_blocking_enabled_;
    }


private:
    bool read_blocking_enabled_ = false;

    Path find_executable_directory();
    Path find_working_directory();

    std::list<Path> resource_path_;

    mutable LRUCache<Path, Path> location_cache_;
};

}

#endif // RESOURCE_LOCATOR_H
