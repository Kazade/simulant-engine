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

#ifndef RESOURCE_LOCATOR_H
#define RESOURCE_LOCATOR_H

#include <list>
#include <vector>
#include <string>

#include "generic/managed.h"
#include "utils/unicode.h"

namespace smlt {


class ResourceMissingError : public std::runtime_error {
public:
    ResourceMissingError(const std::string& what):
        std::runtime_error(what) {}
};


class ResourceLocator :
    public Managed<ResourceLocator> {

public:
    ResourceLocator();

    std::list<unicode>& resource_path() { return resource_path_; }

    unicode locate_file(const unicode& filename) const;
    std::shared_ptr<std::istream> open_file(const unicode& filename);
    std::shared_ptr<std::stringstream> read_file(const unicode& filename);
    std::vector<std::string> read_file_lines(const unicode& filename);

    void add_search_path(const unicode& path);
private:
    unicode find_executable_directory();
    unicode find_working_directory();

    std::list<unicode> resource_path_;
};

}

#endif // RESOURCE_LOCATOR_H
