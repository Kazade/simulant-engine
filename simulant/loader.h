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

#ifndef SIMULANT_LOADER_H
#define SIMULANT_LOADER_H

#include <set>
#include <unordered_map>
#include <stdexcept>
#include <string>
#include <memory>

#include "deps/kfs/kfs.h"

#include "loadable.h"

#include "generic/property.h"
#include "generic/any/any.h"
#include "types.h"

namespace smlt {

class ResourceLocator;

/*
 * You may have a situation where a single filetype can be used for different purposes.
 * For example, a TGA image file might be used as a texture, or could be used as a heightmap,
 * this causes a problem when calling loader_for(some_filename).. which loader do we return?
 * The texture or the heightmap?
 *
 * For this reason loaders can register hints, and the window's loader_for() can take a hint
 * argument for distinguishing between the two.
 */

enum LoaderHint {
    LOADER_HINT_NONE = 0,
    LOADER_HINT_TEXTURE,
    LOADER_HINT_MESH
};

typedef std::unordered_map<unicode, smlt::any> LoaderOptions;

class Loader {
public:
    typedef std::shared_ptr<Loader> ptr;

    Loader(const unicode& filename, std::shared_ptr<std::stringstream> data):
        filename_(filename),
        data_(data) {}

    virtual ~Loader();    
    void into(Loadable* resource, const LoaderOptions& options = LoaderOptions()) {
        into(*resource, options);
    }

    void into(std::shared_ptr<Loadable> resource, const LoaderOptions& options=LoaderOptions()) {
        into(*resource, options);
    }

    void into(WindowBase& window, const LoaderOptions& options=LoaderOptions()) {
        into((Loadable&) window, options);
    }

    void set_resource_locator(ResourceLocator* locator) { locator_ = locator; }

    Property<Loader, ResourceLocator> locator = { this, &Loader::locator_ };
protected:
    unicode filename_;
    std::shared_ptr<std::stringstream> data_;

    template<typename T>
    T* loadable_to(Loadable& loadable) {
        T* thing = dynamic_cast<T*>(&loadable);
        if(!thing) {
            L_WARN("Attempted to cast resource to invalid type");
            return nullptr;
        }

        return thing;
    }

private:
    ResourceLocator* locator_ = nullptr;
    virtual void into(Loadable& resource, const LoaderOptions& options = LoaderOptions()) = 0;
};

class LoaderType {
public:
    typedef std::shared_ptr<LoaderType> ptr;

    virtual ~LoaderType() { }

    virtual unicode name() = 0;
    virtual bool supports(const unicode& filename) const = 0;
    virtual Loader::ptr loader_for(const unicode& filename, std::shared_ptr<std::stringstream> data) const = 0;

    bool has_hint(LoaderHint hint) {
        return (bool) hints_.count(hint);
    }

protected:
    void add_hint(LoaderHint hint) {
        hints_.insert(hint);
    }

    std::set<LoaderHint> hints_;    
};

struct TextureLoadResult {
    uint32_t width;
    uint32_t height;
    uint32_t channels;
    std::vector<uint8_t> data;
};

namespace loaders {

class BaseTextureLoader : public Loader {
public:
    BaseTextureLoader(const unicode& filename, std::shared_ptr<std::stringstream> data):
        Loader(filename, data) {
    }

    void into(Loadable& resource, const LoaderOptions& options = LoaderOptions()) override;

private:
    virtual TextureLoadResult do_load(const std::vector<uint8_t>& buffer) = 0;
};

}
}

#endif
