#ifndef DXF_LOADER_H
#define DXF_LOADER_H


#include <map>
#include <vector>
#include "../types.h"
#include "../loader.h"

namespace kglt {
namespace loaders {

class DXFLoader : public Loader {
public:
    DXFLoader(const std::string& filename):
        Loader(filename) {}

    void into(Loadable& resource);

private:
    std::string current_section_;
};

class DXFLoaderType : public LoaderType {
public:
    std::string name() { return "opt_loader"; }
    bool supports(const std::string& filename) const {
        //FIXME: check magic
        return filename.find(".dxf") != std::string::npos ||
               filename.find(".DXF") != std::string::npos;
    }

    Loader::ptr loader_for(const std::string& filename) const {
        return Loader::ptr(new DXFLoader(filename));
    }
};


}
}


#endif // DXF_LOADER_H
