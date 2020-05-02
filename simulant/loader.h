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

#ifndef SIMULANT_LOADER_H
#define SIMULANT_LOADER_H

#include <set>
#include <unordered_map>
#include <stdexcept>
#include <string>
#include <memory>

#include "logging.h"
#include "deps/kfs/kfs.h"

#include "loadable.h"

#include "generic/property.h"
#include "generic/any/any.h"
#include "types.h"

#include "texture.h"

namespace smlt {

/* Like std::getline, but, it handles \n and \r\n line endings automatically */
std::istream& portable_getline(std::istream& stream, std::string& str);

class VirtualFileSystem;

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

    Loader(const unicode& filename, std::shared_ptr<std::istream> data):
        filename_(filename),
        data_(data) {}

    virtual ~Loader();
    void into(Loadable* resource, const LoaderOptions& options = LoaderOptions()) {
        into(*resource, options);
    }

    void into(std::shared_ptr<Loadable> resource, const LoaderOptions& options=LoaderOptions()) {
        into(*resource, options);
    }

    void into(Window& window, const LoaderOptions& options=LoaderOptions()) {
        into((Loadable&) window, options);
    }

    void set_vfs(VirtualFileSystem* locator) { locator_ = locator; }

    Property<VirtualFileSystem* Loader::*> vfs = { this, &Loader::locator_ };
protected:
    unicode filename_;
    std::shared_ptr<std::istream> data_;

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
    VirtualFileSystem* locator_ = nullptr;
    virtual void into(Loadable& resource, const LoaderOptions& options = LoaderOptions()) = 0;
};

class LoaderType {
public:
    typedef std::shared_ptr<LoaderType> ptr;

    virtual ~LoaderType() { }

    virtual unicode name() = 0;
    virtual bool supports(const unicode& filename) const = 0;
    virtual Loader::ptr loader_for(const unicode& filename, std::shared_ptr<std::istream> data) const = 0;

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
    uint16_t width;
    uint16_t height;
    uint8_t channels;
    TextureTexelType texel_type;
    TextureFormat format;
    std::vector<uint8_t> data;
};

namespace loaders {

class BaseTextureLoader : public Loader {
public:
    BaseTextureLoader(const unicode& filename, std::shared_ptr<std::istream> data):
        Loader(filename, data) {
    }

    void into(Loadable& resource, const LoaderOptions& options = LoaderOptions()) override;

private:
    virtual bool format_stored_upside_down() const { return true; }
    virtual TextureLoadResult do_load(const std::vector<uint8_t>& buffer) = 0;
};

}


struct MeshLoadOptions {
    /* By default, no backface culling is applied to meshes, set this so that the materials
     * generated inherit the cull mode you want */
    CullMode cull_mode = CULL_MODE_NONE;

    /* Some obj files have faces which have a diffuse texture, but no texture coordinates.
     * The spec says that these should just have no texture applied, but in some files
     * they need to be skipped entirely */
    bool obj_include_faces_with_missing_texture_vertices = false;

    /* If set to false, the materials created by the model loader will have blending
     * force disabled. This is useful on the Dreamcast where having blending enabled
     * is costly */
    bool blending_enabled = true;
};

#define MESH_LOAD_OPTIONS_KEY "mesh_options"

}

#endif
