#pragma once

#include "../loader.h"

namespace smlt {
namespace loaders {

class TTFLoader : public Loader {
public:
    TTFLoader(const unicode& filename, std::shared_ptr<std::istream> data):
        Loader(filename, data) {}

    void into(Loadable& resource, const LoaderOptions& options = LoaderOptions());
};

class TTFLoaderType : public LoaderType {
public:
    unicode name() override { return "ttf_font"; }
    bool supports(const unicode& filename) const override {
        return filename.lower().ends_with(".ttf");
    }

    Loader::ptr loader_for(const unicode& filename, std::shared_ptr<std::istream> data) const override {
        return Loader::ptr(new TTFLoader(filename, data));
    }
};

}
}
