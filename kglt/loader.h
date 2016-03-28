#ifndef KGLT_LOADER_H
#define KGLT_LOADER_H

#include <unordered_map>
#include <stdexcept>
#include <string>
#include <memory>
#include "loadable.h"

#include "kazbase/any/any.h"
#include "kazbase/exceptions.h"
#include "generic/protected_ptr.h"
#include "types.h"

namespace kglt {

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

typedef std::unordered_map<unicode, kazbase::any> LoaderOptions;

class Loader {
public:
    typedef std::shared_ptr<Loader> ptr;

    Loader(const unicode& filename, std::shared_ptr<std::stringstream> data):
        filename_(filename),
        data_(data) {}

    virtual ~Loader();    
    void into(const ProtectedPtr<Loadable>& resource, const LoaderOptions& options = LoaderOptions()) {
        into(*resource.__object, options);
    }

    void into(std::shared_ptr<Loadable> resource, const LoaderOptions& options=LoaderOptions()) {
        into(*resource, options);
    }

    void into(WindowBase& window, const LoaderOptions& options=LoaderOptions()) {
        into((Loadable&) window, options);
    }

protected:
    unicode filename_;
    std::shared_ptr<std::stringstream> data_;

    template<typename T>
    T* loadable_to(Loadable& loadable) {
        T* thing = dynamic_cast<T*>(&loadable);
        if(!thing) {
            throw LogicError("Attempted to cast resource to invalid type");
        }

        return thing;
    }

private:
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
