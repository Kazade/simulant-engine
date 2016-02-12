#ifndef OBJ_LOADER_H
#define OBJ_LOADER_H

#include "../loader.h"

namespace kglt {
namespace loaders {

class OBJLoader : public Loader {
public:
    OBJLoader(const unicode& filename, std::shared_ptr<std::stringstream> data):
        Loader(filename, data) {}

    void into(Loadable& resource, const LoaderOptions& options = LoaderOptions());
};

void parse_face(const unicode& input, int32_t& vertex_index, int32_t& tex_index, int32_t& normal_index);

class OBJLoaderType : public LoaderType {
public:
    OBJLoaderType() {

    }

    ~OBJLoaderType() {}

    unicode name() { return "obj"; }
    bool supports(const unicode& filename) const override {
        return filename.lower().contains(".obj");
    }

    Loader::ptr loader_for(const unicode& filename, std::shared_ptr<std::stringstream> data) const {
        return Loader::ptr(new OBJLoader(filename, data));
    }
};

}
}

#endif // OBJ_LOADER_H
