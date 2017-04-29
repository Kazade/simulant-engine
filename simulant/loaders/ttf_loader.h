#pragma once

#include "../loader.h"

namespace smlt {
namespace loaders {

class TTFLoader : public Loader {
public:
    TTFLoader(const unicode& filename, std::shared_ptr<std::stringstream> data):
        Loader(filename, data) {}

    void into(Loadable& resource, const LoaderOptions& options = LoaderOptions());
};

class TTFLoaderType : public LoaderType {
public:
    unicode name() { return "ttf_font"; }
    bool supports(const unicode& filename) const override {
        return filename.lower().ends_with(".ttf");
    }

    Loader::ptr loader_for(const unicode& filename, std::shared_ptr<std::stringstream> data) const {
        return Loader::ptr(new TTFLoader(filename, data));
    }
};

}
}
