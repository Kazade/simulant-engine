#ifndef Q2BSP_LOADER_H_INCLUDED
#define Q2BSP_LOADER_H_INCLUDED

#include "../loader.h"

namespace kglt {
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
