#pragma once

#include <memory>
#include "../loader.h"

namespace kglt {
namespace loaders {

class MD2Loader : public Loader {
public:
    MD2Loader(const unicode& filename, std::shared_ptr<std::stringstream> data):
        Loader(filename, data) {}

    void into(Loadable& resource, const LoaderOptions& options=LoaderOptions());
};

class MD2LoaderType : public LoaderType {
public:
    MD2LoaderType() {

    }

    ~MD2LoaderType() {}

    unicode name() {
        return "md2";
    }

    bool supports(const unicode& filename) const override {
        return filename.lower().contains(".md2");
    }

    Loader::ptr loader_for(const unicode& filename, std::shared_ptr<std::stringstream> data) const {
        return Loader::ptr(new MD2Loader(filename, data));
    }
};


}
}
