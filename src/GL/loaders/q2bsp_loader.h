#ifndef Q2BSP_LOADER_H_INCLUDED
#define Q2BSP_LOADER_H_INCLUDED

#include "../loader.h"

namespace GL {
namespace loaders {

class Q2BSPLoader : public Loader {
public:
    Q2BSPLoader(const std::string& filename):
        Loader(filename) {}

    void into(Resource& resource);

};

class Q2BSPLoaderType : public LoaderType {
public:
    std::string name() { return "bsp_loader"; }
    bool supports(const std::string& filename) const {
        //FIXME: check magic
        return filename.find(".bsp") != std::string::npos;
    }

    Loader::ptr loader_for(const std::string& filename) const {
        return Loader::ptr(new Q2BSPLoader(filename));
    }
};


}
}


#endif // Q2BSP_LOADER_H_INCLUDED
