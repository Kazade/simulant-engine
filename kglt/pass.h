#ifndef KGLT_PASS_H
#define KGLT_PASS_H

#include "viewport.h"
#include "renderer.h"

namespace kglt {
	
class Pass {
public:
	Pass(
		Scene* scene,
		Renderer::ptr renderer, 
		ViewportType viewport,
		CameraID camera_id
	);
	
	Renderer& renderer() { return *renderer_; }
	Viewport& viewport() { return viewport_; }
	Scene& scene() { return *scene_; }
	
private:
	Scene* scene_;
	Renderer::ptr renderer_;
	Viewport viewport_;
	CameraID camera_id_;
};

}
#endif
