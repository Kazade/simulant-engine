#pragma once

#include "../loader.h"

namespace smlt {
namespace loaders {

class TTFLoader : public Loader {
public:
    TTFLoader(const Path& filename, std::shared_ptr<std::istream> data):
        Loader(filename, data) {}

    void into(Loadable& resource, const LoaderOptions& options = LoaderOptions());
};

class TTFLoaderType : public LoaderType {
public:
    const char* name() override { return "ttf_font"; }
    bool supports(const Path& filename) const override {
        return filename.ext() == ".ttf";
    }

    Loader::ptr loader_for(const Path& filename, std::shared_ptr<std::istream> data) const override {
        return Loader::ptr(new TTFLoader(filename, data));
    }
};

}
}
