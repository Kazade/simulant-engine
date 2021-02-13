#include <string>
#include "../utils/string.h"
#include "fnt_loader.h"
#include "../font.h"
#include "../asset_manager.h"
#include "../utils/string.h"
#include "../macros.h"
#include "../streams/utils.h"

namespace smlt {
namespace loaders {

struct OptionsBitField {
    uint8_t smooth : 1;
    uint8_t unicode : 1;
    uint8_t italic : 1;
    uint8_t bold : 1;
    uint8_t fixed_height : 1;
    uint8_t reserved : 3;
};

struct InfoBlock {
    uint16_t font_size;
    OptionsBitField flags;
    uint8_t charset;
    uint16_t stretch_h;
    uint8_t aa;
    uint8_t padding_up;
    uint8_t padding_right;
    uint8_t padding_down;
    uint8_t padding_left;
    uint8_t horizontal_spacing;
    uint8_t vertical_spacing;
    uint8_t outline;
    char name[256];
};

struct PackedBitField {
    uint8_t reserved : 7;
    uint8_t packed : 1;
};

#pragma pack(1)
struct Common {
    uint16_t line_height = 0;
    uint16_t base;
    uint16_t scale_w;
    uint16_t scale_h;
    uint16_t pages;
    PackedBitField packed;
    uint8_t alpha;
    uint8_t red;
    uint8_t green;
    uint8_t blue;
};

struct Char {
    uint32_t id;
    uint16_t x;
    uint16_t y;
    uint16_t width;
    uint16_t height;
    int16_t xoffset;
    int16_t yoffset;
    int16_t xadvance;
    uint8_t page;
    uint8_t channel;
};
#pragma pack()

typedef std::unordered_map<std::string, std::string> Options;

static void parse_line(const std::string& line, std::string& line_type, Options& result) {
    std::string key;
    std::string value;

    std::string* target = &key;
    bool in_quotes = false;

    result.clear();
    for(auto& c: line) {
        switch(c) {
            case '"': {
                in_quotes = !in_quotes;
                (*target) += c;
            } break;
            case ' ':
            case '\r':
            case '\n':
            case '\t': {
                /* Only treat these as separators
                 * if we're not in quotes */
                if(!in_quotes) {
                    if(!key.empty()) {
                        result[key] = value;

                        if(value.empty()) {
                            // Assume first token
                            line_type = key;
                        }
                    }

                    key.clear();
                    value.clear();
                    target = &key;
                }
            } break;
            case '=':
                target = &value;
            break;
            default: {
                (*target) += c;
            }
        }
    }

    if(!key.empty()) {
        result[key] = value;
    }
}

void FNTLoader::read_text(Font* font, std::istream& data, const LoaderOptions &options) {
    _S_UNUSED(options);

    data.seekg(0, std::ios::beg);

    std::string page;
    std::string line;
    Options line_settings;

    while(readline(data, line)) {
        std::string type;
        parse_line(line, type, line_settings);

        if(type == "info") {
            font->font_size_ = std::stoi(line_settings["size"]);
        } else if(type == "common") {
            font->line_gap_ = std::stoi(line_settings["lineHeight"]);
        } else if(type == "page") {
            if(page.empty()) {
                page = line_settings["file"];
                if(page[0] == '"' && page[page.size()-1] == '"') {
                    page = std::string(page.begin() + 1, page.begin() + (page.size() - 1));
                }
            }
        } else if(type == "chars") {

        } else if(type == "char") {
            auto id = std::stoi(line_settings["id"]) - 32;
            if(id >= 0) {
                CharInfo c;
                c.x0 = std::stof(line_settings["x"]);
                c.x1 = c.x0 + std::stoi(line_settings["width"]);
                c.y0 = std::stof(line_settings["y"]);
                c.y1 = c.y0 + std::stoi(line_settings["height"]);

                c.xoff = std::stoi(line_settings["xoffset"]);
                c.yoff = std::stoi(line_settings["yoffset"]);
                c.xadvance = std::stoi(line_settings["xadvance"]);

                // Make sure we can contain this character and
                // assign to the right place
                font->char_data_.resize(id + 1);
                font->char_data_[id] = c;
            }
        } else if(type == "kernings") {

        } else if(type == "kerning") {

        } else {
            L_WARN("Unexpected line type while parsing FNT");
        }
    }

    prepare_texture(font, page);
}

void FNTLoader::read_binary(Font* font, std::istream& data, const LoaderOptions& options) {
    _S_UNUSED(options);

    enum BlockType {
        INFO = 1,
        COMMON = 2,
        PAGES = 3,
        CHARS = 4,
        KERNING_PAIRS = 5
    };

    struct BlockHeader {
        uint8_t type;
        uint32_t size;
    };

    InfoBlock info;
    Common common;
    std::vector<std::string> pages;
    std::vector<Char> chars;

    std::memset(info.name, 0, 256);

    while(data.good()) {
        BlockHeader header;
        data.read((char*)&header.type, sizeof(uint8_t));
        data.read((char*)&header.size, sizeof(uint32_t));

        switch(header.type) {
            case INFO: {
                // We allow a name up to 256 characters, this just makes sure that
                // we don't go trashing memory
                assert(header.size < sizeof(InfoBlock));
                data.read((char*) &info, header.size);  // Using size, rather than sizeof(BlockHeader) is important
            } break;
            case COMMON: {
                data.read((char*) &common, sizeof(Common));
            } break;
            case PAGES: {
                /* Pages are a set of null terminated strings, all the same length
                 * so we read the block, then find the full null-char, then we know the
                 * length of all the strings
                 */
                std::vector<char> page_data(header.size);
                data.read(&page_data[0], header.size);
                auto it = std::find(page_data.begin(), page_data.end(), '\0');
                if(it != page_data.end()) {
                    auto length = std::distance(page_data.begin(), it);
                    auto count = header.size / length;

                    for(auto i = 0u; i < count; ++i) {
                        auto start = i * length;
                        auto end = (i * length) + length;
                        std::string page(page_data.begin() + start, page_data.begin() + end);
                        pages.push_back(page);
                    }

                } else {
                    throw std::runtime_error("Invalid binary FNT file. Couldn't determine page name length.");
                }
            } break;
            case CHARS: {
                auto char_count = header.size / sizeof(Char);
                chars.resize(char_count);
                data.read((char*) &chars[0], sizeof(Char) * char_count);
            } break;
            case KERNING_PAIRS: {
                // Do nothing with this for now, just skip to the end of the file
                std::vector<char> buffer(header.size);
                data.read(&buffer[0], header.size);
            } break;
        }
    }

    font->line_gap_ = common.line_height;
    font->char_data_.resize(chars.size());
    font->font_size_ = info.font_size;

    uint32_t i = 0;;
    for(auto& ch: chars) {
        auto& dst = font->char_data_[i];
        dst.x0 = ch.x;
        dst.x1 = ch.x + ch.width;
        dst.y0 = ch.y;
        dst.y1 = ch.y + ch.height;
        dst.xoff = ch.xoffset;
        dst.yoff = ch.yoffset;
        dst.xadvance = ch.xadvance;
        ++i;
    }

    prepare_texture(font, pages[0]);
}

void FNTLoader::prepare_texture(Font* font, const std::string& texture_file) {
    L_DEBUG("Preparing texture for FNT file");

    // FIXME: Support multiple pages
    auto texture_path = kfs::path::dir_name(kfs::path::abs_path(filename_.encode()));
    texture_path = kfs::path::join(texture_path, texture_file);

    L_DEBUG(_F("Texture path: {0}").format(texture_file));

    TextureFlags flags;

    // Disable auto upload until we've converted the texture
    flags.auto_upload = false;
    flags.filter = TEXTURE_FILTER_BILINEAR;

    font->texture_ = font->asset_manager().new_texture_from_file(texture_path, flags);
    assert(font->texture_);

    font->material_ = font->asset_manager().new_material_from_file(Material::BuiltIns::TEXTURE_ONLY);
    font->material_->set_diffuse_map(font->texture_);
    font->material_->set_cull_mode(CULL_MODE_NONE);
    font->material_->set_depth_test_enabled(false);

    // Set the page dimensions
    // FIXME: Multiple pages
    font->page_height_ = font->texture_->height();
    font->page_width_ = font->texture_->width();

    font->material_->set_blend_func(BLEND_ALPHA);
    font->material_->set_depth_test_enabled(false);
    font->material_->set_cull_mode(CULL_MODE_NONE);

    if(font->texture_->channels() == 1) {
        /*
         * Convert 1 channel textures to 4 channel, 16 bits-per-pixel textures
         * which are the most compressed format we can send the Dreamcast without
         * getting bogged down with VQ compression or paletted textures
         */
        font->texture_->convert(
            TEXTURE_FORMAT_RGBA4444,
            {{TEXTURE_CHANNEL_ONE, TEXTURE_CHANNEL_ONE, TEXTURE_CHANNEL_ONE, TEXTURE_CHANNEL_RED}}
        );
    }

    // OK, it's fine to upload now
    font->texture_->set_auto_upload(true);
    L_DEBUG("Font texture loaded");
}

void FNTLoader::into(Loadable& resource, const LoaderOptions& options) {
    const char TEXT_MARKER[4] = {'i', 'n', 'f', 'o'};
    const char BINARY_MARKER[4] = {'B', 'M', 'F', '\3'};

    L_DEBUG("Loading FNT file");

    Font* font = loadable_to<Font>(resource);

    char version_details[4];
    data_->read(version_details, sizeof(char) * 4);

    if(std::memcmp(version_details, TEXT_MARKER, 4) == 0) {
        L_DEBUG("Loading text FNT");
        read_text(font, *data_, options);
    } else if(std::memcmp(version_details, BINARY_MARKER, 4) == 0) {
        L_DEBUG("Loading binary FNT");
        read_binary(font, *data_, options);
    } else {
        throw std::runtime_error("Unsupported .FNT file");
    }

    L_DEBUG("FNT loaded successfully");
}

}
}
