#pragma once

#include "../loader.h"

namespace smlt {
namespace loaders {

class FNTLoader : public Loader {
public:
    FNTLoader(const Path& filename, std::shared_ptr<std::istream> data):
        Loader(filename, data) {}

    bool into(Loadable& resource, const LoaderOptions& options = LoaderOptions());

private:
    bool read_binary(Font* font, std::istream& data, const LoaderOptions &options);
    bool read_text(Font* font, std::istream& data, const LoaderOptions &options);

    void prepare_texture(Font* font, const std::string& texture_file);
};

class FNTLoaderType : public LoaderType {
public:
    const char* name() override { return "fnt_font"; }
    bool supports(const Path& filename) const override {
        return filename.ext() == ".fnt";
    }

    Loader::ptr loader_for(const Path& filename, std::shared_ptr<std::istream> data) const override {
        return Loader::ptr(new FNTLoader(filename, data));
    }
};

}
}
