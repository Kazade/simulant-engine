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

#include "../../core.h"
#include "../../loader.h"
#include "../../stage.h"
#include "../actor.h"
#include "../../meshes/mesh.h"
#include "../../procedural/constants.h"

#include "skybox_manager.h"

namespace smlt {

SkyManager::SkyManager(Window* window, Stage* stage, StageNodePool* pool):
    WindowHolder(window),
    stage_(stage),
    sky_manager_(new TemplatedSkyboxManager(pool)) {

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
