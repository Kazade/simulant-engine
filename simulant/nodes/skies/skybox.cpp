
#include "skybox.h"
#include "skybox_manager.h"
#include "../../stage.h"
#include "../../assets/material.h"
#include "../../window.h"
#include "../../application.h"
#include "../../vfs.h"

namespace smlt {

Skybox::Skybox(SkyManager* manager):
    ContainerNode(&(Stage&)manager->stage, STAGE_NODE_TYPE_SKYBOX),
    manager_(manager) {

}

bool Skybox::init() {
    return true;
}

void Skybox::clean_up() {
}

bool Skybox::destroy() {
    manager_->destroy_skybox(id());
    return true;
}

bool Skybox::destroy_immediately() {
    manager_->sky_manager_->destroy_immediately(id());
    return true;
}

const AABB &Skybox::aabb() const {
    return actor_->aabb();
}

void Skybox::generate(
    const Path& up,
    const Path& down,
    const Path& left,
    const Path& right,
    const Path& front,
    const Path& back,
    const TextureFlags& flags
) {
    auto stage = manager_->stage.get();

    if(!actor_) {
        actor_ = stage->new_actor();
        actor_->set_parent(this);
        actor_->move_to(0, 0, 0);
    }

    if(!mesh_id_) {
        mesh_id_ = stage->assets->new_mesh_as_cube_with_submesh_per_face(DEFAULT_SIZE, smlt::GARBAGE_COLLECT_NEVER);


        auto mesh = mesh_id_.fetch();
        mesh->reverse_winding();

        // Set the skybox material on all submeshes
        for(auto sm: mesh->each_submesh()) {
            auto mat = stage->assets->new_material_from_file(Material::BuiltIns::TEXTURE_ONLY);

            // Disable depth writes and depth testing, but otherwise use the default texture_only material
            mat->set_depth_write_enabled(false);
            mat->set_depth_test_enabled(false);

            sm->set_material(mat);
        }

        auto up_path = get_app()->vfs->locate_file(up);
        auto down_path = get_app()->vfs->locate_file(down);
        auto left_path = get_app()->vfs->locate_file(left);
        auto right_path = get_app()->vfs->locate_file(right);
        auto back_path = get_app()->vfs->locate_file(back);
        auto front_path = get_app()->vfs->locate_file(front);

        TextureFlags tf = flags;
        tf.wrap = TEXTURE_WRAP_CLAMP_TO_EDGE;

        auto set_texture = [](SubMesh* sm, TexturePtr tex) {
            /* Force a flush. Skyboxes are usually big textures */
            tex->flush();
            sm->material()->set_diffuse_map(tex);
        };

        set_texture(mesh->find_submesh("top"), stage->assets->new_texture_from_file(up_path.value_or(Texture::BuiltIns::CHECKERBOARD), tf));
        set_texture(mesh->find_submesh("bottom"), stage->assets->new_texture_from_file(down_path.value_or(Texture::BuiltIns::CHECKERBOARD), tf));
        set_texture(mesh->find_submesh("left"), stage->assets->new_texture_from_file(left_path.value_or(Texture::BuiltIns::CHECKERBOARD), tf));
        set_texture(mesh->find_submesh("right"), stage->assets->new_texture_from_file(right_path.value_or(Texture::BuiltIns::CHECKERBOARD), tf));
        set_texture(mesh->find_submesh("front"), stage->assets->new_texture_from_file(front_path.value_or(Texture::BuiltIns::CHECKERBOARD), tf));
        set_texture(mesh->find_submesh("back"), stage->assets->new_texture_from_file(back_path.value_or(Texture::BuiltIns::CHECKERBOARD), tf));
    }

    actor_->set_mesh(mesh_id_);
    actor_->set_render_priority(smlt::RENDER_PRIORITY_ABSOLUTE_BACKGROUND);
    actor_->set_cullable(false);
}



}
