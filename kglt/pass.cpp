#include "pass.h"

namespace kglt {

Pass::Pass(Scene* scene, Renderer::ptr renderer, ViewportType viewport, CameraID camera_id):
	scene_(scene),
	renderer_(renderer),
	viewport_(scene),
	camera_id_(camera_id) {
		
	viewport_.configure(viewport);

}

}
