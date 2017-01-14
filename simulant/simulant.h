/* *   Copyright (c) 2011-2017 Luke Benstead https://simulant-engine.appspot.com
 *
 *     This file is part of Simulant.
 *
 *     Simulant is free software: you can redistribute it and/or modify
 *     it under the terms of the GNU General Public License as published by
 *     the Free Software Foundation, either version 3 of the License, or
 *     (at your option) any later version.
 *
 *     Simulant is distributed in the hope that it will be useful,
 *     but WITHOUT ANY WARRANTY; without even the implied warranty of
 *     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *     GNU General Public License for more details.
 *
 *     You should have received a copy of the GNU General Public License
 *     along with Simulant.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef SIMULANT_MAIN_H
#define SIMULANT_MAIN_H

#include "types.h"
#include "sdl2_window.h"
#include "input_controller.h"
#include "stage.h"
#include "overlay.h"
#include "viewport.h"
#include "frustum.h"
#include "material.h"
#include "camera.h"
#include "nodes/actor.h"

#include "nodes/ui/ui_manager.h"
#include "nodes/ui/button.h"
#include "nodes/ui/label.h"
#include "nodes/ui/progress_bar.h"

#include "sound.h"
#include "render_sequence.h"
#include "nodes/light.h"
#include "mesh.h"
#include "procedural/mesh.h"
#include "procedural/texture.h"
#include "loader.h"
#include "ui/interface.h"
#include "texture.h"
#include "application.h"
#include "message_bar.h"
#include "debug.h"
#include "nodes/sprite.h"
#include "nodes/particles.h"
#include "nodes/camera_proxy.h"
#include "hardware_buffer.h"
#include "virtual_gamepad.h"
#include "screens/screen_manager.h"
#include "screens/screen.h"
#include "controllers/fly.h"
#include "controllers/rigid_body.h"
#include "controllers/raycast_vehicle.h"

#include "renderers/renderer_config.h"

#endif
