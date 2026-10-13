#pragma once

#include "../loader.h"

namespace smlt {
namespace loaders {

class ADXLoader : public Loader {
public:
    ADXLoader(const Path& filename, std::shared_ptr<std::istream> data):
        Loader(filename, data) {}

    bool into(Loadable& resource,
              const LoaderOptions& options = LoaderOptions()) override;
};

class ADXLoaderType : public LoaderType {
public:
    const char* name() { return "adx"; }
    bool supports(const Path& filename) const {
        //FIXME: check magic
        return filename.ext() == ".adx";
    }

    Loader::ptr loader_for(const Path& filename, std::shared_ptr<std::istream> data) const {
        return Loader::ptr(new ADXLoader(filename, data));
    }
};


}
}
