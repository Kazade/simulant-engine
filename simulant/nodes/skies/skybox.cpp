
#include "skybox.h"
#include "skybox_manager.h"
#include "../../stage.h"
#include "../../material.h"
#include "../../core.h"

namespace smlt {

Skybox::Skybox(SkyManager* manager):
    ContainerNode(&(Stage&)manager->stage, STAGE_NODE_TYPE_OTHER),
    manager_(manager) {

}

bool Skybox::init() {
    return true;
}

void Skybox::clean_up() {
}

void Skybox::destroy() {
    manager_->destroy_skybox(id());
}

void Skybox::destroy_immediately() {
    manager_->sky_manager_->destroy_immediately(id());
}

const AABB &Skybox::aabb() const {
    return actor_->aabb();
}

void Skybox::generate(
        const unicode& up,
        const unicode& down,
        const unicode& left,
    const unicode& right,
    const unicode& front,
    const unicode& back
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

        auto up_path = manager_->core->vfs->locate_file(up);
        auto down_path = manager_->core->vfs->locate_file(down);
        auto left_path = manager_->core->vfs->locate_file(left);
        auto right_path = manager_->core->vfs->locate_file(right);
        auto back_path = manager_->core->vfs->locate_file(back);
        auto front_path = manager_->core->vfs->locate_file(front);

        TextureFlags flags;
        flags.wrap = TEXTURE_WRAP_CLAMP_TO_EDGE;

        auto set_texture = [](SubMesh* sm, TexturePtr tex) {
            sm->material()->set_diffuse_map(tex);
        };

        set_texture(mesh->find_submesh("top"), stage->assets->new_texture_from_file(up_path, flags));
        set_texture(mesh->find_submesh("bottom"), stage->assets->new_texture_from_file(down_path, flags));
        set_texture(mesh->find_submesh("left"), stage->assets->new_texture_from_file(left_path, flags));
        set_texture(mesh->find_submesh("right"), stage->assets->new_texture_from_file(right_path, flags));
        set_texture(mesh->find_submesh("front"), stage->assets->new_texture_from_file(front_path, flags));
        set_texture(mesh->find_submesh("back"), stage->assets->new_texture_from_file(back_path, flags));
    }

    actor_->set_mesh(mesh_id_);
    actor_->set_render_priority(smlt::RENDER_PRIORITY_ABSOLUTE_BACKGROUND);
    actor_->lock_rotation();
    actor_->set_cullable(false);
}



}
