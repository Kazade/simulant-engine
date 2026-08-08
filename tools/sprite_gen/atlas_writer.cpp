#include "atlas_writer.h"

#include <cstdio>

#include <simulant/logging.h>

namespace sprite_gen {

namespace {

std::string json_escape(const std::string& in) {
    std::string out;
    out.reserve(in.size());
    for(char c: in) {
        if(c == '"' || c == '\\') {
            out.push_back('\\');
        }
        out.push_back(c);
    }
    return out;
}

} // namespace

bool write_atlas_json(const std::string& path,
                      const std::string& image_filename, uint16_t sheet_w,
                      uint16_t sheet_h,
                      const std::vector<FrameInfo>& frames,
                      const std::string& animation_name, float fps) {
    FILE* fp = fopen(path.c_str(), "wb");
    if(!fp) {
        S_ERROR("Could not open '{0}' for writing", path);
        return false;
    }

    const int duration_ms = (fps > 0.0f) ? int(1000.0f / fps + 0.5f) : 0;

    fprintf(fp, "{\n  \"frames\": [\n");
    for(std::size_t i = 0; i < frames.size(); ++i) {
        auto& f = frames[i];
        fprintf(fp,
                "    {\n"
                "      \"filename\": \"%s\",\n"
                "      \"frame\": {\"x\": %u, \"y\": %u, \"w\": %u, \"h\": %u},\n"
                "      \"rotated\": false,\n"
                "      \"trimmed\": false,\n"
                "      \"sourceSize\": {\"w\": %u, \"h\": %u},\n"
                "      \"spriteSourceSize\": {\"x\": 0, \"y\": 0, \"w\": %u, \"h\": %u},\n"
                "      \"duration\": %d\n"
                "    }%s\n",
                json_escape(f.name).c_str(), f.x, f.y, f.w, f.h, f.w, f.h,
                f.w, f.h, duration_ms,
                (i + 1 < frames.size()) ? "," : "");
    }
    fprintf(fp, "  ],\n");

    fprintf(fp,
            "  \"meta\": {\n"
            "    \"image\": \"%s\",\n"
            "    \"size\": {\"w\": %u, \"h\": %u},\n"
            "    \"scale\": \"1\",\n"
            "    \"frameTags\": [\n"
            "      {\"name\": \"%s\", \"from\": 0, \"to\": %d, \"direction\": \"forward\"}\n"
            "    ]\n"
            "  }\n"
            "}\n",
            json_escape(image_filename).c_str(), sheet_w, sheet_h,
            json_escape(animation_name).c_str(),
            frames.empty() ? 0 : int(frames.size()) - 1);

    fclose(fp);
    return true;
}

} // namespace sprite_gen
