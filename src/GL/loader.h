#ifndef KGLT_LOADER_H
#define KGLT_LOADER_H

#include <tr1/memory>
#include "resource.h"

namespace GL {

class LoaderType {
public:
    typedef std::tr1::shared_ptr<LoaderType> ptr;
    virtual std::string name() = 0;
    virtual bool supports(const std::string& filename) const = 0;
    virtual Loader loader_for(const std::string& filename) const = 0;
};



/*
    if(LoaderType().supports("filename")) {
        Loader::ptr = LoaderType().loader(filename);
    }
*/
class Loader {
public:
    typedef std::tr1::shared_ptr<Loader> ptr;

    Loader(const std::string& filename);
    virtual ~Loader();
    virtual load_into(Resource& resource) = 0;

protected:
    std::string filename_;
};

}

#endif
