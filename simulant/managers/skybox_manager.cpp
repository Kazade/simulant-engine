//
//   Copyright (c) 2011-2017 Luke Benstead https://simulant-engine.appspot.com
//
//     This file is part of Simulant.
//
//     Simulant is free software: you can redistribute it and/or modify
//     it under the terms of the GNU Lesser General Public License as published by
//     the Free Software Foundation, either version 3 of the License, or
//     (at your option) any later version.
//
//     Simulant is distributed in the hope that it will be useful,
//     but WITHOUT ANY WARRANTY; without even the implied warranty of
//     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//     GNU Lesser General Public License for more details.
//
//     You should have received a copy of the GNU Lesser General Public License
//     along with Simulant.  If not, see <http://www.gnu.org/licenses/>.
//

#include "../window.h"
#include "../loader.h"
#include "../stage.h"
#include "../nodes/actor.h"
#include "../meshes/mesh.h"
#include "../procedural/constants.h"
#include "../generic/manual_manager.h"

#include "skybox_manager.h"

namespace smlt {

Skybox::Skybox(SkyID id, SkyManager* manager):
    generic::Identifiable<SkyID>(id),
    ContainerNode(&(Stage&)manager->stage),
    manager_(manager) {

}

bool Skybox::init() {
    return true;
}

void Skybox::clean_up() {
}

void Skybox::ask_owner_for_destruction() {
    manager_->destroy_skybox(id());
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
        mesh->each([&stage](const std::string&, SubMesh* sm) {
            auto mat = stage->assets->new_material_from_file(Material::BuiltIns::TEXTURE_ONLY);

            // Disable depth writes and depth testing, but otherwise use the default texture_only material
            mat->set_depth_write_enabled(false);
            mat->set_depth_test_enabled(false);

            sm->set_material_id(mat->id());
        });

        auto up_path = manager_->window->vfs->locate_file(up);
        auto down_path = manager_->window->vfs->locate_file(down);
        auto left_path = manager_->window->vfs->locate_file(left);
        auto right_path = manager_->window->vfs->locate_file(right);
        auto back_path = manager_->window->vfs->locate_file(back);
        auto front_path = manager_->window->vfs->locate_file(front);

        TextureFlags flags;
        flags.wrap = TEXTURE_WRAP_CLAMP_TO_EDGE;

        auto set_texture = [](SubMesh* sm, TextureID tex) {
            sm->material_id().fetch()->set_diffuse_map(tex);
        };

        set_texture(mesh->submesh("top"), stage->assets->new_texture_from_file(up_path, flags));
        set_texture(mesh->submesh("bottom"), stage->assets->new_texture_from_file(down_path, flags));
        set_texture(mesh->submesh("left"), stage->assets->new_texture_from_file(left_path, flags));
        set_texture(mesh->submesh("right"), stage->assets->new_texture_from_file(right_path, flags));
        set_texture(mesh->submesh("front"), stage->assets->new_texture_from_file(front_path, flags));
        set_texture(mesh->submesh("back"), stage->assets->new_texture_from_file(back_path, flags));
    }

    actor_->set_mesh(mesh_id_);
    actor_->set_render_priority(smlt::RENDER_PRIORITY_ABSOLUTE_BACKGROUND);
    actor_->lock_rotation();
    actor_->set_renderable_culling_mode(RENDERABLE_CULLING_MODE_NEVER);
}


SkyManager::SkyManager(Window* window, Stage* stage):
    WindowHolder(window),
    stage_(stage),
    sky_manager_(new TemplatedSkyboxManager()) {

}

/**
 * @brief SkyboxManager::new_skybox_from_folder
 * @param folder
 * @return New skybox ID
 *
 * Scans a folder for image files containing "top", "bottom", "left", "right", "front", "back"
 * in their names. If duplicates are found, or if images are not found, then this function raises
 * SkyboxImageNotFoundError or SkyboxImageDuplicateError respectively
 */
SkyboxPtr SkyManager::new_skybox_from_folder(const unicode& folder) {
    std::map<SkyboxFace, std::string> files;

    auto path = window->vfs->locate_file(folder);

    for(auto& file: kfs::path::list_dir(path.encode())) {
        SkyboxFace face;

        // Case-insensitive detection
        unicode file_lower = unicode(file).lower();

        if(file_lower.contains("top") || file_lower.contains("up")) {
            face = SKYBOX_FACE_TOP;
        } else if(file_lower.contains("bottom") || file_lower.contains("down")) {
            face = SKYBOX_FACE_BOTTOM;
        } else if(file_lower.contains("left")) {
            face = SKYBOX_FACE_LEFT;
        } else if(file_lower.contains("right")) {
            face = SKYBOX_FACE_RIGHT;
        } else if(file_lower.contains("front")) {
            face = SKYBOX_FACE_FRONT;
        } else if(file_lower.contains("back")) {
            face = SKYBOX_FACE_BACK;
        } else {
            // Not a filename worth paying attention to
            continue;
        }

        auto full_path = kfs::path::join(folder.encode(), file);

        // Make sure this is a supported texture file
        if(!window->loader_type("texture")->supports(full_path)) {
            continue;
        }

        // If we already have found this face, then throw an error
        if(files.count(face)) {
            throw SkyboxImageDuplicateError("Found multiple potential images for face");
        }

        // Store the relative path, rather than the absolute one
        files[face] = full_path;
    }

    if(files.size() != 6) {
        throw SkyboxImageNotFoundError("Unable detect all skybox images");
    }

    // Call through now that we've detected the files
    return new_skybox_from_files(
        files.at(SKYBOX_FACE_TOP),
        files.at(SKYBOX_FACE_BOTTOM),
        files.at(SKYBOX_FACE_LEFT),
        files.at(SKYBOX_FACE_RIGHT),
        files.at(SKYBOX_FACE_FRONT),
        files.at(SKYBOX_FACE_BACK)
    );
}

/**
 * @brief new_skybox_from_absolute_files
 * @param up
 * @param down
 * @param left
 * @param right
 * @param front
 * @param back
 * @return New skybox ID
 *
 * Creates a skybox with the 6 images specified. If any of the images are not found this throws a
 * SkyboxImageNotFoundError
 */
SkyboxPtr SkyManager::new_skybox_from_files(
    const unicode& up,
    const unicode& down,
    const unicode& left,
    const unicode& right,
    const unicode& front,
    const unicode& back) {

    assert(stage_);

    auto sb = sky_manager_->make(this);
    sb->generate(
        up, down, left, right, front, back
    );

    return sb;
}

SkyboxPtr SkyManager::skybox(SkyID skybox_id) {
    return sky_manager_->get(skybox_id);
}

void SkyManager::destroy_skybox(SkyID skybox_id) {
    sky_manager_->destroy(skybox_id);
}

}
