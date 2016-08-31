#include "../window_base.h"
#include "../loader.h"
#include "../stage.h"
#include "../actor.h"
#include "../mesh.h"
#include "../procedural/constants.h"

#include "skybox_manager.h"

namespace kglt {

Skybox::Skybox(SkyboxID id, SkyboxManager* manager):
    generic::Identifiable<SkyboxID>(id),
    ParentSetterMixin<MoveableObject>(&(Stage&)manager->stage) {

}

bool Skybox::init() {
    return true;
}

void Skybox::cleanup() {

}

void Skybox::ask_owner_for_destruction() {
    manager_->delete_skybox(id());
}

void Skybox::generate(
    const unicode& up,
    const unicode& down,
    const unicode& left,
    const unicode& right,
    const unicode& front,
    const unicode& back
) {
    auto& stage = manager_->stage;

    if(!actor_id_) {
        actor_id_ = stage->new_actor();
    }

    if(!mesh_id_) {
        /*
        mesh_id_ = stage->assets->new_mesh_as_box(
            1.0, 1.0, 1.0,
            kglt::GARBAGE_COLLECT_NEVER,
            procedural::MESH_STYLE_SUBMESH_PER_FACE
        );*/
    }

    {
        auto actor = stage->actor(actor_id_);
        actor->set_mesh(mesh_id_);
    }

}


SkyboxManager::SkyboxManager(WindowBase* window, Stage* stage):
    WindowHolder(window),
    stage_(stage) {

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
SkyboxID SkyboxManager::new_skybox_from_folder(const unicode& folder) {
    std::map<SkyboxFace, unicode> files;

    for(auto& file: kfs::path::list_dir(folder.encode())) {
        SkyboxFace face;

        // Case-insensitive detection
        unicode file_lower = unicode(file).lower();

        if(file_lower.contains("top")) {
            face = SKYBOX_FACE_TOP;
        } else if(file_lower.contains("bottom")) {
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
        files[face] = file;
    }

    if(files.size() != 6) {
        throw SkyboxImageNotFoundError("Unable detect all skybox images");
    }

    // Call through now that we've detected the files
    return new_skybox_from_folder_and_relative_files(folder, files);
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
SkyboxID SkyboxManager::new_skybox_from_absolute_files(
    const unicode& up,
    const unicode& down,
    const unicode& left,
    const unicode& right,
    const unicode& front,
    const unicode& back) {

    assert(stage_);

    SkyboxID sid = TemplatedSkyboxManager::make(this);

    auto sb = skybox(sid);
    sb->generate(
        up, down, left, right, front, back
    );

    return sid;
}

/**
 * @brief new_skybox_from_folder_and_relative_files
 * @param folder
 * @param up
 * @param down
 * @param left
 * @param right
 * @param front
 * @param back
 * @return New skybox ID
 *
 * Takes a folder and a list of filenames without paths, and creates a Skybox.
 *
 * Raises SkyboxImageNotFoundError if any of the images don't exist.
 */
SkyboxID SkyboxManager::new_skybox_from_folder_and_relative_files(
    const unicode& folder,
    const unicode& up,
    const unicode& down,
    const unicode& left,
    const unicode& right,
    const unicode& front,
    const unicode& back) {

    return new_skybox_from_absolute_files(
        kfs::path::join(folder.encode(), up.encode()),
        kfs::path::join(folder.encode(), down.encode()),
        kfs::path::join(folder.encode(), left.encode()),
        kfs::path::join(folder.encode(), right.encode()),
        kfs::path::join(folder.encode(), front.encode()),
        kfs::path::join(folder.encode(), back.encode())
    );
}

SkyboxID SkyboxManager::new_skybox_from_folder_and_relative_files(const unicode& folder, std::map<SkyboxFace, unicode> files) {
    return new_skybox_from_folder_and_relative_files(
        folder,
        files.at(SKYBOX_FACE_TOP),
        files.at(SKYBOX_FACE_BOTTOM),
        files.at(SKYBOX_FACE_LEFT),
        files.at(SKYBOX_FACE_RIGHT),
        files.at(SKYBOX_FACE_FRONT),
        files.at(SKYBOX_FACE_BACK)
    );
}

SkyboxPtr SkyboxManager::skybox(SkyboxID skybox_id) {
    return TemplatedSkyboxManager::get(skybox_id).lock().get();
}

void SkyboxManager::delete_skybox(SkyboxID skybox_id) {
    TemplatedSkyboxManager::destroy(skybox_id);
}

}
