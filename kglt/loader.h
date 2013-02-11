#ifndef KGLT_LOADER_H
#define KGLT_LOADER_H

#include <stdexcept>
#include <string>
#include <tr1/memory>
#include "loadable.h"

#include "kazbase/exceptions.h"
#include "kglt/option_list.h"

namespace kglt {

class Scene;

class LoaderOptionsUnsupportedError : public std::logic_error {
public:
    LoaderOptionsUnsupportedError():
        std::logic_error("This loader does not support options") {}
};

class LoaderRequiresOptionsError : public std::logic_error {
public:
    LoaderRequiresOptionsError():
        std::logic_error("This loader requires options") {}
};
/*
    if(LoaderType().supports("filename")) {
        Loader::ptr = LoaderType().loader(filename);
    }
*/
class Loader {
public:
    typedef std::tr1::shared_ptr<Loader> ptr;

    Loader(const std::string& filename):
        filename_(filename) {}

    virtual ~Loader();
    virtual void into(Loadable& resource) {
        throw LoaderRequiresOptionsError();
    }

    virtual void into(Loadable& resource, const kglt::option_list::OptionList& options) {
        throw LoaderOptionsUnsupportedError();
    }
protected:
    std::string filename_;

    Scene* loadable_to_scene_ptr(Loadable& resource);
};

class LoaderType {
public:
    typedef std::tr1::shared_ptr<LoaderType> ptr;
    virtual std::string name() = 0;
    virtual bool supports(const std::string& filename) const = 0;
    virtual bool has_hint(const std::string& type_hint) const { return false; }
    virtual bool requires_hint() const { return false; }
    virtual Loader::ptr loader_for(const std::string& filename) const = 0;
};

}

#endif
