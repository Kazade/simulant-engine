#ifndef KGLT_SELECTION_RENDERER_H
#define KGLT_SELECTION_RENDERER_H

#include <map>

#include "kglt/renderer.h"

namespace kglt {

class SelectionRenderer : public Renderer {
public:
	typedef std::tr1::shared_ptr<SelectionRenderer> ptr;
	
	SelectionRenderer(Scene& scene):
		Renderer(scene),
		selected_mesh_id_(0) {}
	
	SelectionRenderer(Scene& scene, const RenderOptions& options):
		Renderer(scene, options),
		selected_mesh_id_(0) {}
	
	void visit(Camera* camera) {}
	void visit(Scene* scene) {}
	void visit(Sprite* sprite) {}
    void visit(Background* bg) {}
    void visit(BackgroundLayer* layer) {}
	void visit(Mesh* mesh);
    void visit(UI* ui) {}
    void visit(ui::Element* element) {}
    void visit(Text* text) {} //Dunno if this should be selectable..
	
	MeshID selected_mesh() const { return selected_mesh_id_; }
	
	bool pre_visit(Object* obj) {
	    //If this is a mesh, and the entire branch is not selectable,
	    //then bail out
	    if(Mesh* m = dynamic_cast<Mesh*>(obj)) {
	        if(!m->branch_selectable()) {	            
	            return false;
	        }
	    }
	    
	    Renderer::pre_visit(obj);
	    
	    return true;
	}
	
private:
	void on_start_render();
	void on_finish_render();

    uint8_t r_count, g_count, b_count;	
    
    std::map<std::tuple<float, float, float>, MeshID> colour_mesh_lookup_;
    MeshID selected_mesh_id_;
};


}
#endif
