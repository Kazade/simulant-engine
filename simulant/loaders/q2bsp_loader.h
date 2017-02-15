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

#ifndef Q2BSP_LOADER_H_INCLUDED
#define Q2BSP_LOADER_H_INCLUDED

#include <map>

#include "../loader.h"

namespace smlt {

typedef std::map<std::string, std::string> Q2Entity;
typedef std::vector<Q2Entity> Q2EntityList;

namespace loaders {

class Q2BSPLoader : public Loader {
public:
    Q2BSPLoader(const unicode& filename, std::shared_ptr<std::stringstream> data):
        Loader(filename, data) {}

    void into(Loadable& resource, const LoaderOptions& options=LoaderOptions());

};

class Q2BSPLoaderType : public LoaderType {
public:
    unicode name() { return "bsp_loader"; }
    bool supports(const unicode& filename) const {
        //FIXME: check magic
        return filename.lower().contains(".bsp");
    }

    Loader::ptr loader_for(const unicode& filename, std::shared_ptr<std::stringstream> data) const {
        return Loader::ptr(new Q2BSPLoader(filename, data));
    }
};


}
}


#endif // Q2BSP_LOADER_H_INCLUDED
