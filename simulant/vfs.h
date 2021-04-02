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
    VirtualFileSystem(Window* window);

    std::list<Path>& search_path() { return resource_path_; }

    Path locate_file(const Path& filename) const;
    std::shared_ptr<std::istream> open_file(const Path& filename);
    std::shared_ptr<std::stringstream> read_file(const Path& filename);
    std::vector<std::string> read_file_lines(const Path& filename);

    bool add_search_path(const Path& path);
    void remove_search_path(const Path& path);

private:
    Path find_executable_directory();
    Path find_working_directory();

    std::list<Path> resource_path_;

    Window* window_;
};

}

#endif // RESOURCE_LOCATOR_H
