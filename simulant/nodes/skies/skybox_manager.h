/* *   Copyright (c) 2011-2017 Luke Benstead https://simulant-engine.appspot.com
 *
 *     This file is part of Simulant.
 *
 *     Simulant is free software: you can redistribute it and/or modify
 *     it under the terms of the GNU Lesser General Public License as published by
 *     the Free Software Foundation, either version 3 of the License, or
 *     (at your option) any later version.
 *
 *     Simulant is distributed in the hope that it will be useful,
 *     but WITHOUT ANY WARRANTY; without even the implied warranty of
 *     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *     GNU Lesser General Public License for more details.
 *
 *     You should have received a copy of the GNU Lesser General Public License
 *     along with Simulant.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include <map>

#include "../../generic/managed.h"
#include "../../generic/identifiable.h"
#include "../stage_node.h"
#include "../../types.h"
#include "../../managers/window_holder.h"
#include "../stage_node_manager.h"
#include "skybox.h"
#include "../../asset_manager.h"

namespace smlt {

class Stage;

typedef StageNodeManager<StageNodePool, SkyID, Skybox> TemplatedSkyboxManager;

class SkyManager :
    public virtual WindowHolder {

    friend class Skybox;

public:
    SkyManager(Window* window, Stage* stage, StageNodePool *pool);

    SkyManager(const SkyManager& rhs) = delete;
    SkyManager& operator=(const SkyManager&) = delete;

    SkyboxPtr new_skybox_from_folder(
        const Path& folder,
        const TextureFlags& flags=TextureFlags()
    );

    SkyboxPtr new_skybox_from_files(
        const Path& up,
        const Path& down,
        const Path& left,
        const Path& right,
        const Path& front,
        const Path& back,
        const TextureFlags &flags=TextureFlags()
    );

    SkyboxPtr skybox(SkyID skybox_id);
    void destroy_skybox(SkyID skybox_id);

    Property<Stage* SkyManager::*> stage = { this, &SkyManager::stage_ };
private:
    Stage* stage_ = nullptr;

    std::shared_ptr<TemplatedSkyboxManager> sky_manager_;

};

}
