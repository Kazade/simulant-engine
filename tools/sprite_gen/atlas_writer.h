#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace sprite_gen {

struct FrameInfo {
    std::string name;
    uint16_t x, y, w, h;
};

/* Writes a TexturePacker-style "array" format JSON atlas describing the
 * frames of a sprite sheet. `image_filename` should be the sheet's
 * filename (not a full path) as referenced from the atlas file. Returns
 * false (and logs an error) if the file couldn't be written. */
bool write_atlas_json(const std::string& path,
                      const std::string& image_filename, uint16_t sheet_w,
                      uint16_t sheet_h,
                      const std::vector<FrameInfo>& frames,
                      const std::string& animation_name, float fps);

} // namespace sprite_gen
