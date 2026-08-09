#pragma once

#include "../loader.h"

namespace smlt {
namespace loaders {

/* Loads the TexturePacker-style "array" JSON atlas format written by the
 * sprite_gen tool (see tools/sprite_gen/atlas_writer.h) into a Spritesheet
 * asset. */
class SpritesheetLoader : public Loader {
public:
    SpritesheetLoader(const Path& filename, std::shared_ptr<std::istream> data):
        Loader(filename, data) {}

    bool into(Loadable& resource,
              const LoaderOptions& options = LoaderOptions()) override;
};

class SpritesheetLoaderType : public LoaderType {
public:
    virtual ~SpritesheetLoaderType() {}

    const char* name() override {
        return "spritesheet";
    }

    bool supports(const Path& filename) const override {
        return filename.ext() == ".json";
    }

    Loader::ptr loader_for(const Path& filename,
                           std::shared_ptr<std::istream> data) const override {
        return Loader::ptr(new SpritesheetLoader(filename, data));
    }
};

} // namespace loaders
} // namespace smlt
