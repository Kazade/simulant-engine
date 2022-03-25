#pragma once

#include <memory>
#include "../utils/unicode.h"
#include "../loader.h"

namespace smlt {
namespace loaders {

class WAVLoader : public Loader {
public:
    WAVLoader(const Path& filename, std::shared_ptr<std::istream> data):
        Loader(filename, data) {}

    bool into(Loadable& resource, const LoaderOptions &options=LoaderOptions());

};

class WAVLoaderType : public LoaderType {
public:
    const char* name() { return "wav"; }
    bool supports(const Path& filename) const {
        //FIXME: check magic
        return filename.ext() == ".wav";
    }

    Loader::ptr loader_for(const Path& filename, std::shared_ptr<std::istream> data) const {
        return Loader::ptr(new WAVLoader(filename, data));
    }
};


}
}
