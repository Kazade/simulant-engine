#include "kglt/kglt.h"
#include "kglt/shortcuts.h"

int main(int argc, char* argv[]) {
    kglt::Window::ptr window = kglt::Window::create();
		
    // Create two viewports for the left and right hand side of the screen, set different clear colours
    kglt::Viewport first(kglt::VIEWPORT_TYPE_VERTICAL_SPLIT_LEFT, kglt::Colour::RED);
    kglt::Viewport second(kglt::VIEWPORT_TYPE_VERTICAL_SPLIT_RIGHT, kglt::Colour::GREEN);

    kglt::StageID sid = window->new_stage();
    auto stage = window->stage(sid);

    kglt::MeshID cube = stage->new_mesh_as_cube(1.0);
    kglt::ActorID aid = stage->new_actor_with_mesh(cube);

    stage->actor(aid)->move_to(0, 0, -5);

    // Render new stages to the framebuffer, using both viewports. Make sure we tell the pipeline to clear
    window->render(sid, window->new_camera_for_viewport(first)).to_framebuffer(first).with_clear();
    window->render(sid, window->new_camera_for_viewport(second)).to_framebuffer(second).with_clear();

    while(window->run_frame()) {}
	
	return 0;
}
