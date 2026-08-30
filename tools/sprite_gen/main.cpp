#include <simulant/simulant.h>

#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <sstream>
#include <string>

#include "sprite_gen_options.h"
#include "sprite_gen_scene.h"

namespace {

void usage() {
    std::cout
        << "Usage: sprite_gen -i <input.glb|.gltf|.obj> [options]\n"
           "\n"
           "Renders an animation from a 3D model to a sprite sheet.\n"
           "\n"
           "Options:\n"
           "  -i, --input <path>             Input .glb/.gltf/.obj file "
           "(required; .obj has no animation data, so only a single static "
           "frame is generated)\n"
           "  -o, --output <path>            Output .dtex or .tga file "
           "(default: <input>.dtex)\n"
           "  -C, --compress                 Write a compressed .dtex "
           "(requires texconv on the PATH). Only valid for .dtex output\n"
           "  -a, --animation <name>         Animation to render (default: "
           "first found)\n"
           "  -r, --rotation <x,y,z>         Euler rotation in degrees "
           "applied to the model (default: 0,0,0)\n"
           "  -s, --scale <factor>           Uniform scale applied to the "
           "model (default: 1.0)\n"
           "      --light-color <r,g,b>      Light color, 0-1 floats "
           "(default: 1,1,1)\n"
           "      --light-direction <x,y,z>  Direction for a directional "
           "light\n"
           "      --light-position <x,y,z>   Position for a point light "
           "(overrides --light-direction)\n"
           "      --ambient <r,g,b>          Ambient light color, 0-1 "
           "floats (default: 1,1,1 - full brightness)\n"
           "      --fps <n>                  Animation sampling rate "
           "(default: 15)\n"
           "      --atlas <path>             Also write a JSON atlas "
           "describing the sheet\n"
           "  -h, --help                     Show this message\n";
}

bool parse_floats(const std::string& text, float* out, int count) {
    std::stringstream ss(text);
    std::string part;
    int i = 0;
    while(std::getline(ss, part, ',')) {
        if(i >= count) {
            return false;
        }
        try {
            std::size_t consumed = 0;
            out[i] = std::stof(part, &consumed);
            if(consumed != part.size()) {
                return false;
            }
        } catch(...) {
            return false;
        }
        ++i;
    }
    return i == count;
}

bool parse_vec3(const std::string& text, smlt::Vec3& out) {
    float v[3];
    if(!parse_floats(text, v, 3)) {
        return false;
    }
    out = smlt::Vec3(v[0], v[1], v[2]);
    return true;
}

bool parse_color(const std::string& text, smlt::Color& out) {
    float v[3];
    if(!parse_floats(text, v, 3)) {
        return false;
    }
    out = smlt::Color(v[0], v[1], v[2], 1.0f);
    return true;
}

} // namespace

class SpriteGenApp: public smlt::Application {
public:
    SpriteGenApp(const smlt::AppConfig& config,
                sprite_gen::SpriteGenOptions opts):
        smlt::Application(config), opts_(std::move(opts)) {}

private:
    bool init() override {
        scenes->register_scene<sprite_gen::SpriteGenScene>("main", opts_);
        return true;
    }

    sprite_gen::SpriteGenOptions opts_;
};

int main(int argc, char* argv[]) {
    sprite_gen::SpriteGenOptions opts;
#ifdef DEFAULT_TEXCONV
    opts.texconv = DEFAULT_TEXCONV;
#endif

    for(int i = 1; i < argc; ++i) {
        std::string a = argv[i];
        auto next = [&]() -> std::string {
            if(i + 1 >= argc) {
                std::cerr << "Missing value for " << a << "\n";
                std::exit(1);
            }
            return argv[++i];
        };

        if(a == "-h" || a == "--help") {
            usage();
            return 0;
        } else if(a == "-i" || a == "--input") {
            opts.input_path = next();
        } else if(a == "-o" || a == "--output") {
            opts.output_path = next();
        } else if(a == "-C" || a == "--compress") {
            opts.compress = true;
        } else if(a == "-a" || a == "--animation") {
            opts.animation_name = next();
        } else if(a == "-r" || a == "--rotation") {
            if(!parse_vec3(next(), opts.rotation)) {
                std::cerr << "Error: --rotation expects x,y,z (degrees)\n";
                return 1;
            }
        } else if(a == "-s" || a == "--scale") {
            opts.scale = std::stof(next());
        } else if(a == "--light-color") {
            if(!parse_color(next(), opts.light_color)) {
                std::cerr << "Error: --light-color expects r,g,b (0-1)\n";
                return 1;
            }
        } else if(a == "--light-direction") {
            smlt::Vec3 v;
            if(!parse_vec3(next(), v)) {
                std::cerr << "Error: --light-direction expects x,y,z\n";
                return 1;
            }
            opts.light_direction = v;
        } else if(a == "--light-position") {
            smlt::Vec3 v;
            if(!parse_vec3(next(), v)) {
                std::cerr << "Error: --light-position expects x,y,z\n";
                return 1;
            }
            opts.light_position = v;
        } else if(a == "--ambient") {
            if(!parse_color(next(), opts.ambient_color)) {
                std::cerr << "Error: --ambient expects r,g,b (0-1)\n";
                return 1;
            }
        } else if(a == "--fps") {
            opts.fps = std::stof(next());
        } else if(a == "--atlas") {
            opts.atlas_path = next();
        } else {
            std::cerr << "Error: unknown option '" << a << "'\n\n";
            usage();
            return 1;
        }
    }

    if(opts.input_path.empty()) {
        std::cerr << "Error: -i/--input is required\n\n";
        usage();
        return 1;
    }

    smlt::Path in_path(opts.input_path);
    if(!std::filesystem::exists(opts.input_path)) {
        std::cerr << "Error: input file does not exist: " << opts.input_path
                  << "\n";
        return 1;
    }

    if(opts.fps <= 0.0f) {
        std::cerr << "Error: --fps must be greater than 0\n";
        return 1;
    }

    if(opts.scale <= 0.0f) {
        std::cerr << "Error: --scale must be greater than 0\n";
        return 1;
    }

    if(opts.output_path.empty()) {
        opts.output_path = in_path.replace_ext("dtex").str();
    }

    std::string out_ext = smlt::Path(opts.output_path).ext();
    std::transform(out_ext.begin(), out_ext.end(), out_ext.begin(), ::tolower);

    if(out_ext != ".dtex" && out_ext != ".tga") {
        std::cerr << "Error: output path must end in .dtex or .tga (got '"
                  << opts.output_path << "')\n";
        return 1;
    }

    if(opts.compress && out_ext != ".dtex") {
        std::cerr << "Error: -C/--compress only applies to .dtex output\n";
        return 1;
    }

    smlt::AppConfig config;
    config.title = "sprite_gen: " + opts.input_path;
    config.width = sprite_gen::MAX_SHEET_DIM;
    config.height = sprite_gen::MAX_SHEET_DIM;
    config.fullscreen = false;
    config.show_cursor = true;
    config.log_level = smlt::LOG_LEVEL_INFO;

    auto failed = opts.failed;

    /* We've already parsed argv ourselves above (smlt::ArgParser only
     * accepts --long-form flags and would reject our -i/-o/-a/-r/-s short
     * flags outright), so run() is called with no arguments to skip the
     * engine's own arg parsing. */
    SpriteGenApp app(config, opts);
    app.run();

    return (*failed) ? 1 : 0;
}
