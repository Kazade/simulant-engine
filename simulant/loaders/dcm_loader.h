#pragma once

#include "../loader.h"

namespace smlt {
namespace loaders {

class DCMLoader : public Loader {
public:
    DCMLoader(const Path& filename, std::shared_ptr<std::istream> data):
        Loader(filename, data) {}

    void into(Loadable& resource, const LoaderOptions& options = LoaderOptions());
};

class DCMLoaderType : public LoaderType {
public:
    virtual ~DCMLoaderType() {}

    const char* name() override {
        return "dcm";
    }

    bool supports(const Path& filename) const override {
        return filename.ext() == ".dcm";
    }

    Loader::ptr loader_for(const Path& filename, std::shared_ptr<std::istream> data) const override {
        return Loader::ptr(new DCMLoader(filename, data));
    }
};

}
}
