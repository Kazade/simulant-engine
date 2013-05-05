#ifndef KGLT_LOADER_H
#define KGLT_LOADER_H

#include <stdexcept>
#include <string>
#include <tr1/memory>
#include "loadable.h"

#include "kazbase/exceptions.h"

namespace kglt {

class Scene;

typedef std::map<unicode, unicode> LoaderOptions;

class Loader {
public:
    typedef std::shared_ptr<Loader> ptr;

    Loader(const unicode& filename):
        filename_(filename) {}

    virtual ~Loader();
    virtual void into(Loadable& resource, const LoaderOptions& options = LoaderOptions()) = 0;

protected:
    unicode filename_;

    template<typename T>
    T* loadable_to(Loadable& loadable) {
        T* thing = dynamic_cast<T*>(&loadable);
        if(!thing) {
            throw LogicError("Attempted to cast resource to invalid type");
        }

        return thing;
    }
};

class LoaderType {
public:
    typedef std::shared_ptr<LoaderType> ptr;

    virtual ~LoaderType() { }

    virtual unicode name() = 0;
    virtual bool supports(const unicode& filename) const = 0;
    virtual Loader::ptr loader_for(const unicode& filename) const = 0;
};

}

#endif
