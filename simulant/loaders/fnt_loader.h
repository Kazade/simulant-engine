#pragma once

#include "../loader.h"

namespace smlt {
namespace loaders {

class FNTLoader : public Loader {
public:
    FNTLoader(const unicode& filename, std::shared_ptr<std::istream> data):
        Loader(filename, data) {}

    void into(Loadable& resource, const LoaderOptions& options = LoaderOptions());

private:
    void read_binary(Font* font, std::istream& data, const LoaderOptions &options);
    void read_text(Font* font, std::istream& data, const LoaderOptions &options);

    void prepare_texture(Font* font, const std::string& texture_file);
};

class FNTLoaderType : public LoaderType {
public:
    unicode name() override { return "fnt_font"; }
    bool supports(const unicode& filename) const override {
        return filename.lower().ends_with(".fnt");
    }

    Loader::ptr loader_for(const unicode& filename, std::shared_ptr<std::istream> data) const override {
        return Loader::ptr(new FNTLoader(filename, data));
    }
};

}
}
