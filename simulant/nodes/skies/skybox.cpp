#include <map>

#include "skybox.h"

#include "../../stage.h"
#include "../../assets/material.h"
#include "../../window.h"
#include "../../application.h"
#include "../../vfs.h"
#include "../actor.h"

namespace smlt {

Skybox::Skybox(Scene* owner):
    ContainerNode(owner, STAGE_NODE_TYPE_SKYBOX) {

}


const AABB &Skybox::aabb() const {
    return actor_->aabb();
}

typedef std::map<SkyboxFace, Path> SkyboxImageDict;

optional<SkyboxImageDict> discover_files_from_directory(const Path& folder) {
    SkyboxImageDict files;

    auto path = get_app()->vfs->locate_file(folder);

    if(!path.has_value()) {
        return SkyboxImageDict();
    }

    for(auto& file: kfs::path::list_dir(path.value().str())) {
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

        auto full_path = kfs::path::join(folder.str(), file);

        // Make sure this is a supported texture file
        if(!get_app()->loader_type("texture")->supports(full_path)) {
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
        return SkyboxImageDict();
    }

    return files;
}

bool Skybox::on_create(void* params) {
    SkyboxParams* args = (SkyboxParams*) params;

    if(!args->source_directory.str().empty()) {
        auto maybe_files = discover_files_from_directory(args->source_directory);
        if(maybe_files) {
            auto files = maybe_files.value();
            generate(
                files.at(SKYBOX_FACE_TOP),
                files.at(SKYBOX_FACE_BOTTOM),
                files.at(SKYBOX_FACE_LEFT),
                files.at(SKYBOX_FACE_RIGHT),
                files.at(SKYBOX_FACE_FRONT),
                files.at(SKYBOX_FACE_BACK),
                args->flags
            );
        } else {
            S_ERROR("Couldn't determine all skybox images");
            return false;
        }
    }

    return true;
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
    auto& assets = scene->assets;
    mesh_ = assets->new_mesh_as_cube_with_submesh_per_face(DEFAULT_SIZE);

    auto mesh = mesh_;
    mesh->reverse_winding();

    // Set the skybox material on all submeshes
    for(auto sm: mesh->each_submesh()) {
        auto mat = assets->new_material_from_file(Material::BuiltIns::TEXTURE_ONLY);

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

    set_texture(mesh->find_submesh("top"), assets->new_texture_from_file(up_path.value_or(Texture::BuiltIns::CHECKERBOARD), tf));
    set_texture(mesh->find_submesh("bottom"), assets->new_texture_from_file(down_path.value_or(Texture::BuiltIns::CHECKERBOARD), tf));
    set_texture(mesh->find_submesh("left"), assets->new_texture_from_file(left_path.value_or(Texture::BuiltIns::CHECKERBOARD), tf));
    set_texture(mesh->find_submesh("right"), assets->new_texture_from_file(right_path.value_or(Texture::BuiltIns::CHECKERBOARD), tf));
    set_texture(mesh->find_submesh("front"), assets->new_texture_from_file(front_path.value_or(Texture::BuiltIns::CHECKERBOARD), tf));
    set_texture(mesh->find_submesh("back"), assets->new_texture_from_file(back_path.value_or(Texture::BuiltIns::CHECKERBOARD), tf));

    if(!actor_) {
        actor_ = scene->create_node<Actor>(mesh_);
        actor_->set_parent(this);
        actor_->transform->set_position(Vec3());
        actor_->set_render_priority(smlt::RENDER_PRIORITY_ABSOLUTE_BACKGROUND);
        actor_->set_cullable(false);
    } else {
        actor_->set_mesh(mesh_);
    }
}



}
