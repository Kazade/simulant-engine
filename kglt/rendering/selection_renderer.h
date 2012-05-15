#ifndef KGLT_SELECTION_RENDERER_H
#define KGLT_SELECTION_RENDERER_H

#include "kglt/renderer.h"

namespace kglt {

class SelectionRenderer : public Renderer {
public:
	typedef std::tr1::shared_ptr<SelectionRenderer> ptr;
	
	void visit(Camera* camera) {}
	void visit(Scene* scene) {}
	void visit(Sprite* sprite) {}
	void visit(Mesh* mesh);
	
	MeshID selected_mesh() const { return selected_mesh_; }
	
private:
	void on_start_render();
	void on_finish_render();

	MeshID selected_mesh_;
};


}
#endif
