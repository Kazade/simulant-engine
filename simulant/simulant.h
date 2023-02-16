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

#ifdef __DREAMCAST__
#include "kos_window.h"
#elif defined(__PSP__)
#include "platforms/psp/psp_window.h"
#elif defined(__ANDROID__)
#include "platforms/android/android_window.h"
#else
#include "sdl2_window.h"
#endif

#include "stage.h"
#include "viewport.h"
#include "frustum.h"
#include "assets/material.h"
#include "nodes/actor.h"
#include "nodes/geom.h"

#include "nodes/ui/ui_manager.h"
#include "nodes/ui/button.h"
#include "nodes/ui/label.h"
#include "nodes/ui/progress_bar.h"
#include "nodes/ui/image.h"
#include "nodes/ui/frame.h"
#include "nodes/ui/keyboard.h"
#include "nodes/ui/text_entry.h"

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
#include "debug.h"
#include "nodes/sprite.h"
#include "nodes/particle_system.h"
#include "nodes/camera.h"
#include "platform.h"
#include "vfs.h"

#include "assets/meshes/skeleton.h"
#include "assets/meshes/rig.h"

#include "scenes/scene_manager.h"
#include "scenes/scene.h"
#include "scenes/physics_scene.h"
#include "scenes/loading.h"
#include "scenes/splash.h"

#include "input/input_state.h"
#include "input/input_manager.h"
#include "input/input_axis.h"

#include "behaviours/fly.h"
#include "behaviours/builtin.h"
#include "behaviours/stage_node_behaviour.h"

#include "renderers/renderer_config.h"

#include "loaders/q2bsp_loader.h"
#include "coroutines/helpers.h"

#include "streams/file_ifstream.h"
#include "streams/stream_view.h"

#endif
