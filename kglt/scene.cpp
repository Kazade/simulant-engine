#include "utils/glcompat.h"
#include "utils/ownable.h"
#include "background.h"

#include "scene.h"
#include "renderer.h"
#include "camera.h"
#include "render_sequence.h"
#include "loader.h"
#include "stage.h"
#include "ui_stage.h"
#include "partitioners/null_partitioner.h"
#include "partitioners/octree_partitioner.h"

#include "shaders/default_shaders.h"
#include "window_base.h"

namespace kglt {

Scene::Scene(WindowBase* window):
    ResourceManagerImpl(window),
    default_texture_(0),
    default_material_(0),
    render_sequence_(new RenderSequence(*this)) {

}

Scene::~Scene() {
    //TODO: Log the unfreed resources (textures, meshes, materials etc.)    

    //Clear the stages first, they may hold references to cameras, materials
    //etc.
    StageManager::__objects().clear();
    CameraManager::__objects().clear();
}

void Scene::render_tree() {
    for(auto stage: StageManager::objects_) {
        uint32_t counter = 0;
        render_tree(stage.second.get(), counter);
    }
}

MaterialID Scene::clone_default_material() {
    return MaterialManager::manager_clone(default_material_->id());
}

MaterialID Scene::default_material_id() const {
    return default_material_->id();
}

TextureID Scene::default_texture_id() const {
    return default_texture_->id();
}

CameraID Scene::default_camera_id() const {
    return default_camera_;
}

void Scene::initialize_defaults() {
    default_camera_ = new_camera(); //Create a default camera
    default_stage_ = new_stage(kglt::PARTITIONER_NULL);

    stage(default_stage_).host_camera(default_camera_);

    default_ui_stage_ = new_ui_stage();
    default_ui_camera_ = new_camera();

    camera(default_ui_camera_).set_orthographic_projection(
        0, window().width(), window().height(), 0, -1, 1
    );

    //Create a default stage for the default stage with the default camera
    render_sequence_->new_pipeline(default_stage_, default_camera_);

    //Add a pipeline for the default UI stage to render
    //after the main pipeline
    render_sequence_->new_pipeline(
        default_ui_stage_, default_ui_camera_,
        ViewportID(), TextureID(), 100
    );

    //FIXME: Should lock the default texture and material during construction!

    //Create the default blank texture
    default_texture_ = texture(new_texture()).__object;
    default_texture_->resize(1, 1);
    default_texture_->set_bpp(32);

    default_texture_->data()[0] = 255;
    default_texture_->data()[1] = 255;
    default_texture_->data()[2] = 255;
    default_texture_->data()[3] = 255;
    default_texture_->upload();

    //Maintain ref-count
    default_material_ = material(new_material_from_file("kglt/materials/multitexture_and_lighting.kglm")).__object;

    //Set the default material's first texture to the default (white) texture
    default_material_->technique().pass(0).set_texture_unit(0, default_texture_->id());
}

StageID Scene::new_stage(AvailablePartitioner partitioner) {
    Stage& ss = stage(StageManager::manager_new(StageID(), partitioner));

    return ss.id();
}

uint32_t Scene::stage_count() const {
    return StageManager::manager_count();
}

Stage& Scene::stage(StageID s) {
    if(s == DefaultStageID) {
        return *(StageManager::manager_get(default_stage_)).lock();
    }

    return *(StageManager::manager_get(s)).lock();
}

StageRef Scene::stage_ref(StageID s) {
    if(!StageManager::manager_contains(s)) {
        throw DoesNotExist<Stage>();
    }
    return StageManager::__objects()[s];
}

void Scene::delete_stage(StageID s) {
    Stage& ss = stage(s);

    //Recurse through the tree, destroying all children
    ss.apply_recursively_leaf_first(&ownable_tree_node_destroy, false);

    StageManager::manager_delete(s);
}


UIStageID Scene::new_ui_stage() {
    return UIStageManager::manager_new();
}

ProtectedPtr<UIStage> Scene::ui_stage() {
    return ui_stage(default_ui_stage_);
}

ProtectedPtr<UIStage> Scene::ui_stage(UIStageID s) {
    if(!s) {
        return UIStageManager::manager_get(default_ui_stage_);
    } else {
        return UIStageManager::manager_get(s);
    }
}

void Scene::delete_ui_stage(UIStageID s) {
    UIStageManager::manager_delete(s);
}

uint32_t Scene::ui_stage_count() const {
    return UIStageManager::manager_count();
}

CameraRef Scene::camera_ref(CameraID c) {
    if(!CameraManager::manager_contains(c)) {
        throw DoesNotExist<Camera>();
    }
    return CameraManager::__objects()[c];
}

CameraID Scene::new_camera() {
    CameraID new_camera = CameraManager::manager_new();

    return new_camera;
}

Camera& Scene::camera(CameraID c) {
    if(c == CameraID()) {
        //Return the default camera
        return *(CameraManager::manager_get(default_camera_).lock());
    }
    return *(CameraManager::manager_get(c).lock());
}

void Scene::delete_camera(CameraID cid) {
    //Remove any associated proxy
    if(camera(cid).has_proxy()) {
        camera(cid).proxy().stage().evict_camera(cid);
    }

    CameraManager::manager_delete(cid);
}

//============== START BACKGROUNDS ==========

BackgroundID Scene::new_background() {
    BackgroundID bid = BackgroundManager::manager_new();
    return bid;
}

BackgroundID Scene::new_background_from_file(const unicode& filename, float scroll_x, float scroll_y) {
    BackgroundID result = new_background();
    try {
        background(result)->set_texture(new_texture_from_file(filename));
        background(result)->set_horizontal_scroll_rate(scroll_x);
        background(result)->set_vertical_scroll_rate(scroll_y);
    } catch(...) {
        delete_background(result);
        throw;
    }

    return result;
}

ProtectedPtr<Background> Scene::background(BackgroundID bid) {
    return BackgroundManager::manager_get(bid);
}

bool Scene::has_background(BackgroundID bid) const {
    return BackgroundManager::manager_contains(bid);
}

void Scene::delete_background(BackgroundID bid) {
    BackgroundManager::manager_delete(bid);
}

uint32_t Scene::background_count() const {
    return BackgroundManager::manager_count();
}

//============== END BACKGROUNDS ============


bool Scene::init() {    
    return true;
}

void Scene::update(double dt) {
    if(physics_enabled()) {
        physics().step(dt);
    }

    //Update the backgrounds
    for(auto background_pair: BackgroundManager::__objects()) {
        auto* bg = background_pair.second.get();
        bg->update(dt);
    }

    //Update the stages
    for(auto stage_pair: StageManager::__objects()) {
        GenericTreeNode* root = stage_pair.second.get();
        root->apply_recursively([=](GenericTreeNode* node) -> void {
            node->as<Updateable>()->update(dt);
        });
    }
}

void Scene::render() {
    render_sequence_->run();
}

}
