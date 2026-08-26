#pragma once

#include <memory>
#include <string>

#include <simulant/color.h>
#include <simulant/generic/optional.h>
#include <simulant/math/vec3.h>

namespace sprite_gen {

struct SpriteGenOptions {
    std::string input_path;
    std::string output_path;
    std::string atlas_path; // empty = don't write an atlas
    std::string animation_name; // empty = use the first animation found

    smlt::Vec3 rotation = smlt::Vec3(0, 0, 0); // Euler degrees, XYZ
    float scale = 1.0f;

    smlt::Color light_color = smlt::Color::white();
    smlt::optional<smlt::Vec3> light_direction;
    smlt::optional<smlt::Vec3> light_position;
    smlt::Color ambient_color = smlt::Color::white();

    float fps = 15.0f;

    /* Output format is inferred from output_path's extension (.dtex or
     * .tga). -C/--compress only applies to .dtex output and requires the
     * texconv tool (https://github.com/tvspelsfreak/texconv/) to be on the
     * PATH - texconv is used to write the compressed data, otherwise
     * sprite_gen writes an uncompressed .dtex itself. */
    bool compress = false;
    std::string texconv = "texconv";

    /* Shared with the SpriteGenApp instance that owns the AppConfig this
     * struct was built from, so main() can report a non-zero exit code
     * for failures discovered only once the Scene starts loading (e.g. an
     * unreadable/unsupported model) - Application::run() itself always
     * returns 0. */
    std::shared_ptr<bool> failed = std::make_shared<bool>(false);
};

} // namespace sprite_gen
