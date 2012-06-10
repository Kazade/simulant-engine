#include "sprite_loader.h"
#include "kglt/sprite.h"
#include "kglt/shortcuts.h"
#include "kglt/option_list.h"

#include <boost/format.hpp>

namespace kglt {
namespace loaders {

void SpriteLoader::into(Loadable& resource) {
    Loadable* res_ptr = &resource;
    Sprite* sprite = dynamic_cast<Sprite*>(res_ptr);
    assert(sprite && "You passed a Resource that is not a sprite to the Sprite loader");

    Scene& scene = sprite->scene(); //Grab the scene that this sprite belongs to


    //CONTINUE HERE! NEED TO BE ABLE TO PASS DOWN FALLBACK_TO_CHECKERBOARD OPTION
    //Load the texture from the specified filename
    TextureID tex_id = kglt::create_texture_from_file(scene.window(), filename_);

    //Get a handle to the texture
    Texture& tex = scene.texture(tex_id);
    sprite->_set_frame_count(1);
    sprite->_set_texture_id(tex_id);
    sprite->_set_frame_size(tex.width(), tex.height());
    sprite->set_animation_frames(0, 0);
    sprite->set_render_dimensions(tex.width(), tex.height());
}

void SpriteLoader::into(Loadable& resource, const kglt::option_list::OptionList& options) {
    Loadable* res_ptr = &resource;
    Sprite* sprite = dynamic_cast<Sprite*>(res_ptr);
    assert(sprite && "You passed a Resource that is not a sprite to the Sprite loader");

	Scene& scene = sprite->scene(); //Grab the scene that this sprite belongs to
	
	
	//CONTINUE HERE! NEED TO BE ABLE TO PASS DOWN FALLBACK_TO_CHECKERBOARD OPTION
	//Load the texture from the specified filename
	TextureID tex_id = kglt::create_texture_from_file(scene.window(), filename_);
	
	//Get a handle to the texture
	Texture& tex = scene.texture(tex_id);
	
	//Get the frame width option, validate it's an integer
	uint32_t frame_width = kglt::option_list::get_as<uint32_t>(options, "SPRITE_FRAME_WIDTH");		
	
	//Check that the texture width is divisible by the frame width
	if(tex.width() % frame_width != 0) {
		throw IOError((boost::format("Invalid sprite width %d when frame width is %d") % tex.width() % frame_width).str());
	}
	
	sprite->_set_frame_count(tex.width() / frame_width);
	sprite->_set_texture_id(tex_id);
	sprite->_set_frame_size(frame_width, tex.height());		
	sprite->set_animation_frames(0, 0);
    sprite->set_render_dimensions(tex.width(), tex.height());
}

}
}
