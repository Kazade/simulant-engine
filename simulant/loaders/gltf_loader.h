#pragma once

#include "../loader.h"

namespace smlt {
namespace loaders {

class GLTFLoader : public Loader {
public:
    GLTFLoader(const Path& filename, std::shared_ptr<std::istream> data):
        Loader(filename, data) {}

    void into(Loadable& resource, const LoaderOptions& options = LoaderOptions());
};

class GLTFLoaderType : public LoaderType {
public:
    GLTFLoaderType() {}
    virtual ~GLTFLoaderType() {}

    const char* name() override { return "gltf"; }

    bool supports(const Path& filename) const override {
        return filename.ext() == ".gltf" || filename.ext() == ".glb";
    }

    Loader::ptr loader_for(const Path& filename, std::shared_ptr<std::istream> data) const override {
        return Loader::ptr(new GLTFLoader(filename, data));
    }
};

}
}
