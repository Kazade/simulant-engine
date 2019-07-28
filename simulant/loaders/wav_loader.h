#pragma once

#include <memory>
#include "../utils/unicode.h"
#include "../loader.h"

namespace smlt {
namespace loaders {

class WAVLoader : public Loader {
public:
    WAVLoader(const unicode& filename, std::shared_ptr<std::istream> data):
        Loader(filename, data) {}

    void into(Loadable& resource, const LoaderOptions &options=LoaderOptions());

};

class WAVLoaderType : public LoaderType {
public:
    unicode name() { return "wav"; }
    bool supports(const unicode& filename) const {
        //FIXME: check magic
        return filename.lower().contains(".wav");
    }

    Loader::ptr loader_for(const unicode& filename, std::shared_ptr<std::istream> data) const {
        return Loader::ptr(new WAVLoader(filename, data));
    }
};


}
}
