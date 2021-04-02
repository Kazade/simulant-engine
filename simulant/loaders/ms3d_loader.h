#pragma once

#include "../loader.h"

namespace smlt {
namespace loaders {

class MS3DLoader : public Loader {
public:
    MS3DLoader(const Path& filename, std::shared_ptr<std::istream> data);

    void into(Loadable& resource, const LoaderOptions& options = LoaderOptions());
};

void parse_face(const std::string& input, int32_t& vertex_index, int32_t& tex_index, int32_t& normal_index);

class MS3DLoaderType : public LoaderType {
public:
    MS3DLoaderType() {

    }

    virtual ~MS3DLoaderType() {}

    const char* name() override { return "MS3D"; }
    bool supports(const Path& filename) const override {
        return filename.ext() == ".ms3d";
    }

    Loader::ptr loader_for(const Path& filename, std::shared_ptr<std::istream> data) const override {
        return Loader::ptr(new MS3DLoader(filename, data));
    }
};

}
}
