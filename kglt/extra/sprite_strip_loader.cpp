#include <boost/lexical_cast.hpp>

#include "kazbase/exceptions.h"
#include "kazbase/os/path.h"
#include "sprite_strip_loader.h"

#include "../scene.h"
#include "../shortcuts.h"

namespace kglt {
namespace extra {

SpriteStripLoader::SpriteStripLoader(Scene& scene, const std::string& filename, uint32_t frame_width):
    scene_(scene),
    filename_(filename),
    frame_width_(frame_width) {

    if(!os::path::exists(filename)) {
        throw IOError("File does not exist: " + filename);
    }
}

std::vector<TextureID> SpriteStripLoader::load_frames() {
    //Load texture, but don't upload to OpenGL
    kglt::Texture& tmp = scene_.texture(kglt::create_texture_from_file(scene_.window(), filename_, false));

    if(tmp.width() % frame_width_ != 0) {
        throw IOError("Invalid texture width. Should be a multiple of: " + boost::lexical_cast<std::string>(frame_width_));
    }

    std::vector< Texture::Data > frame_data;
    frame_data.resize(tmp.width() / frame_width_);

    /*
      Go through the loaded texture data in rows, keep track of the
      x position so we can copy the data to the correct frame
    */
    uint32_t bytes = tmp.bpp() / 8;
    for(uint32_t y = 0; y < tmp.height(); ++y) {
        uint32_t current_frame = 0;        

        for(uint32_t x = 0; x < tmp.width(); ++x) {            
            uint32_t idx = ((y * bytes) * tmp.width()) + (x * bytes);

            for(uint32_t i = 0; i < tmp.bpp() / 8; ++i) {
                frame_data[current_frame].push_back(tmp.data().at(idx + i));
            }

            if(x && (x % frame_width_) == 0) {
                current_frame++;
            }
        }
    }

    std::vector<TextureID> results;
    for(Texture::Data& data: frame_data) {
        kglt::Texture& tex = kglt::return_new_texture(scene_);
        tex.set_bpp(tmp.bpp()); //Set the bpp
        tex.resize(frame_width_, tmp.height()); //Resize the texture
        tex.data().assign(data.begin(), data.end()); //Copy the frame data

        //Upload the texture in the idle handler
        scene_.window().idle().add_once(sigc::bind(sigc::mem_fun(&tex, &kglt::Texture::upload), true, true, true, false));

        results.push_back(tex.id()); //Store the ID
    }

    scene_.delete_texture(tmp.id()); //Delete the temporary texture
    return results;
}

}
}
