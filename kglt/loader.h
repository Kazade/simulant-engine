#ifndef KGLT_LOADER_H
#define KGLT_LOADER_H

#include <unordered_map>
#include <stdexcept>
#include <string>
#include <memory>
#include "loadable.h"

#include "kazbase/exceptions.h"
#include "generic/protected_ptr.h"
#include "types.h"

namespace kglt {

typedef std::unordered_map<unicode, unicode> LoaderOptions;

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
};

}

#endif
