#pragma once

#include "../loader.h"

namespace smlt {
namespace loaders {

class GLTFLoader : public Loader {
public:
    GLTFLoader(const unicode& filename, std::shared_ptr<std::istream> data):
        Loader(filename, data) {}

    void into(Loadable& resource, const LoaderOptions& options=LoaderOptions());
};

class GLTFLoaderType : public LoaderType {
public:
    GLTFLoaderType() {

    }

    virtual ~GLTFLoaderType() {}

    unicode name() override {
        return "gltf";
    }

    bool supports(const unicode& filename) const override {
        return filename.lower().contains(".gltf");
    }

    Loader::ptr loader_for(const unicode& filename, std::shared_ptr<std::istream> data) const override {
        return Loader::ptr(new GLTFLoader(filename, data));
    }
};


}
}
