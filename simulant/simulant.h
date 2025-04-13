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

#ifndef SIMULANT_MAIN_H
#define SIMULANT_MAIN_H

#include "types.h"

/* In some cases you might want to override window creation, in which case
 * we don't want to include the default platform stuff */

#ifndef SIMULANT_CUSTOM_WINDOW

#ifdef __DREAMCAST__
#include "platforms/dreamcast/kos_window.h"
#elif defined(__PSP__)
#include "platforms/psp/psp_window.h"
#elif defined(__ANDROID__)
#include "platforms/android/android_window.h"
#else
#include "platforms/sdl/sdl2_window.h"
#endif

#endif

#include "assets/material.h"
#include "frustum.h"
#include "nodes/actor.h"
#include "nodes/geom.h"
#include "nodes/mesh_instancer.h"
#include "nodes/physics/dynamic_body.h"
#include "nodes/physics/joints.h"
#include "nodes/physics/kinematic_body.h"
#include "services/physics.h"
#include "stage.h"
#include "time_keeper.h"
#include "viewport.h"

#include "nodes/cylindrical_billboard.h"
#include "nodes/physics/dynamic_body.h"
#include "nodes/physics/static_body.h"
#include "nodes/smooth_follow.h"
#include "nodes/spherical_billboard.h"

#include "nodes/fly_controller.h"

#include "nodes/skies/skybox.h"

#include "nodes/ui/ui_manager.h"
#include "nodes/ui/button.h"
#include "nodes/ui/label.h"
#include "nodes/ui/progress_bar.h"
#include "nodes/ui/image.h"
#include "nodes/ui/frame.h"
#include "nodes/ui/keyboard.h"
#include "nodes/ui/text_entry.h"
#include "nodes/sprite.h"
#include "nodes/particle_system.h"
#include "nodes/camera.h"
#include "nodes/audio_source.h"
#include "nodes/debug.h"
#include "nodes/locators/node_locator.h"
#include "nodes/stats_panel.h"

#include "sound.h"
#include "utils/random.h"
#include "compositor.h"
#include "nodes/light.h"
#include "meshes/mesh.h"
#include "procedural/mesh.h"
#include "procedural/texture.h"
#include "loader.h"
#include "texture.h"
#include "application.h"
#include "assets/binary_data.h"


#include "platform.h"
#include "vfs.h"

#include "assets/meshes/skeleton.h"
#include "assets/meshes/rig.h"

#include "scenes/scene_manager.h"
#include "scenes/scene.h"
#include "scenes/splash.h"

#include "input/input_state.h"
#include "input/input_manager.h"
#include "input/input_axis.h"

#include "renderers/renderer_config.h"

#include "coroutines/helpers.h"

#include "streams/file_ifstream.h"
#include "streams/stream_view.h"

#endif
