#ifndef KGLT_LOADER_H
#define KGLT_LOADER_H

#include <tr1/memory>
#include "resource.h"

namespace GL {

class Loader {
public:
    typedef std::tr1::shared_ptr<Loader> ptr;

    virtual void load_into(Resource& resource, const std::string& filename) = 0;

};

}

#endif
