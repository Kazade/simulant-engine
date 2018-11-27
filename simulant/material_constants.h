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

#ifndef MATERIAL_CONSTANTS_H
#define MATERIAL_CONSTANTS_H

#ifndef _arch_dreamcast
static const uint32_t MAX_TEXTURE_UNITS = 8;
#else
// The Dreamcast only supports 2 multitexture units
static const uint32_t MAX_TEXTURE_UNITS = 2;
#endif

static const uint32_t MAX_TEXTURE_MATRICES = 8;
static const uint32_t MAX_MATERIAL_PASSES = 8;
static const uint32_t MAX_LIGHTS_PER_RENDERABLE = 8;

#endif // MATERIAL_CONSTANTS_H
