#include "spritesheet_loader.h"

#include "../asset_manager.h"
#include "../assets/spritesheet.h"
#include "../utils/json.h"
#include "../vfs.h"

namespace smlt {
namespace loaders {

bool SpritesheetLoader::into(Loadable& resource, const LoaderOptions& options) {
    _S_UNUSED(options);

    Spritesheet* sheet = loadable_to<Spritesheet>(resource);
    if(!sheet) {
        S_ERROR("Resource is not a Spritesheet");
        return false;
    }

    auto js = json_read(this->data_);

    if(!js->has_key("frames") || !js->has_key("meta")) {
        S_ERROR("Invalid spritesheet atlas: missing 'frames' or 'meta' key");
        return false;
    }

    auto meta = js["meta"];
    auto image_name = meta["image"]->to_str().value_or("");
    if(image_name.empty()) {
        S_ERROR("Spritesheet atlas has no meta.image entry");
        return false;
    }

    /* The image is referenced relative to the atlas file, so add its
       containing folder to the search path while we resolve it (mirrors
       the approach used by the particle script and glTF loaders). */
    auto folder = filename_.parent();
    bool added = false;
    if(vfs) {
        added = vfs->insert_search_path(0, folder);
    }

    auto tex = sheet->asset_manager().load_texture(Path(image_name));

    if(added) {
        vfs->remove_search_path(folder);
    }

    if(!tex) {
        S_ERROR("Couldn't load spritesheet texture: {0}", image_name);
        return false;
    }

    sheet->set_texture(tex);

    auto frames = js["frames"];
    for(std::size_t i = 0; i < frames->size(); ++i) {
        auto f = frames[i];
        auto rect = f["frame"];

        SpritesheetFrame frame;
        frame.name = f["filename"]->to_str().value_or("");
        frame.x = (uint16_t)rect["x"]->to_int().value_or(0);
        frame.y = (uint16_t)rect["y"]->to_int().value_or(0);
        frame.w = (uint16_t)rect["w"]->to_int().value_or(0);
        frame.h = (uint16_t)rect["h"]->to_int().value_or(0);
        frame.duration_ms = (uint32_t)f["duration"]->to_int().value_or(0);

        sheet->push_frame(frame);
    }

    if(meta->has_key("frameTags")) {
        auto tags = meta["frameTags"];
        for(std::size_t i = 0; i < tags->size(); ++i) {
            auto tag = tags[i];

            SpritesheetAnimation animation;
            animation.name = tag["name"]->to_str().value_or("");
            animation.start_frame = (uint32_t)tag["from"]->to_int().value_or(0);
            animation.end_frame = (uint32_t)tag["to"]->to_int().value_or(0);

            sheet->push_animation(animation);
        }
    }

    return true;
}

} // namespace loaders
} // namespace smlt
