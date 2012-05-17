#ifndef KGLT_SELECTION_RENDERER_H
#define KGLT_SELECTION_RENDERER_H

#include <tr1/unordered_map>

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
	
    uint8_t r_count, g_count, b_count;	
    
    std::tr1::unordered_map<std::tuple<float, float, float>, MeshID> colour_mesh_lookup_;
    MeshID selected_mesh_id_;
};


}
#endif
