//
//   Copyright (c) 2011-2017 Luke Benstead https://simulant-engine.appspot.com
//
//     This file is part of Simulant.
//
//     Simulant is free software: you can redistribute it and/or modify
//     it under the terms of the GNU General Public License as published by
//     the Free Software Foundation, either version 3 of the License, or
//     (at your option) any later version.
//
//     Simulant is distributed in the hope that it will be useful,
//     but WITHOUT ANY WARRANTY; without even the implied warranty of
//     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//     GNU General Public License for more details.
//
//     You should have received a copy of the GNU General Public License
//     along with Simulant.  If not, see <http://www.gnu.org/licenses/>.
//

#include <stdexcept>
#include <cassert>

#include "window.h"
#include "material.h"
#include "asset_manager.h"
#include "renderers/renderer.h"


namespace smlt {

const std::string Material::BuiltIns::DEFAULT = "simulant/materials/${RENDERER}/default.kglm";
const std::string Material::BuiltIns::TEXTURE_ONLY = "simulant/materials/${RENDERER}/texture_only.kglm";
const std::string Material::BuiltIns::DIFFUSE_ONLY = "simulant/materials/${RENDERER}/diffuse_only.kglm";
const std::string Material::BuiltIns::ALPHA_TEXTURE = "simulant/materials/${RENDERER}/alpha_texture.kglm";
const std::string Material::BuiltIns::DIFFUSE_WITH_LIGHTING = "simulant/materials/${RENDERER}/diffuse_with_lighting.kglm";
const std::string Material::BuiltIns::MULTITEXTURE2_MODULATE = "simulant/materials/${RENDERER}/multitexture2_modulate.kglm";
const std::string Material::BuiltIns::MULTITEXTURE2_ADD = "simulant/materials/${RENDERER}/multitexture2_add.kglm";
const std::string Material::BuiltIns::TEXTURE_WITH_LIGHTMAP = "simulant/materials/${RENDERER}/texture_with_lightmap.kglm";
const std::string Material::BuiltIns::TEXTURE_WITH_LIGHTMAP_AND_LIGHTING = "simulant/materials/${RENDERER}/texture_with_lightmap_and_lighting.kglm";
const std::string Material::BuiltIns::MULTITEXTURE2_MODULATE_WITH_LIGHTING = "simulant/materials/${RENDERER}/multitexture2_modulate_with_lighting.kglm";
const std::string Material::BuiltIns::SKYBOX = "simulant/materials/${RENDERER}/skybox.kglm";
const std::string Material::BuiltIns::TEXTURED_PARTICLE = "simulant/materials/${RENDERER}/textured_particle.kglm";
const std::string Material::BuiltIns::DIFFUSE_PARTICLE = "simulant/materials/${RENDERER}/diffuse_particle.kglm";

/* This list is used by the particle script loader to determine if a specified material
 * is a built-in or not. Please keep this up-to-date when changing the above materials!
 */
const std::map<std::string, std::string> Material::BUILT_IN_NAMES = {
    {"DEFAULT", Material::BuiltIns::DEFAULT},
    {"TEXTURE_ONLY", Material::BuiltIns::TEXTURE_ONLY},
    {"DIFFUSE_ONLY", Material::BuiltIns::DIFFUSE_ONLY},
    {"ALPHA_TEXTURE", Material::BuiltIns::ALPHA_TEXTURE},
    {"DIFFUSE_WITH_LIGHTING", Material::BuiltIns::DIFFUSE_WITH_LIGHTING},
    {"MULTITEXTURE2_MODULATE", Material::BuiltIns::MULTITEXTURE2_MODULATE},
    {"MULTITEXTURE2_ADD", Material::BuiltIns::MULTITEXTURE2_ADD},
    {"TEXTURE_WITH_LIGHTMAP", Material::BuiltIns::TEXTURE_WITH_LIGHTMAP},
    {"TEXTURE_WITH_LIGHTMAP_AND_LIGHTING", Material::BuiltIns::TEXTURE_WITH_LIGHTMAP_AND_LIGHTING},
    {"MULTITEXTURE2_MODULATE_WITH_LIGHTING", Material::BuiltIns::MULTITEXTURE2_MODULATE_WITH_LIGHTING},
    {"SKYBOX", Material::BuiltIns::SKYBOX},
    {"TEXTURED_PARTICLE", Material::BuiltIns::TEXTURED_PARTICLE},
    {"DIFFUSE_PARTICLE", Material::BuiltIns::DIFFUSE_PARTICLE}
};

static const std::string DEFAULT_VERT_SHADER = R"(
    attribute vec3 vertex_position;
    attribute vec4 vertex_diffuse;

    uniform mat4 modelview_projection;

    varying vec4 diffuse;

    void main() {
        diffuse = vertex_diffuse;
        gl_Position = (modelview_projection * vec4(vertex_position, 1.0));
    }
)";

static const std::string DEFAULT_FRAG_SHADER = R"(
    varying vec4 diffuse;
    void main() {
        gl_FragColor = diffuse;
    }
)";

Material::Material(MaterialID id, AssetManager* asset_manager):
    generic::Identifiable<MaterialID>(id) {

    initialize_default_properties();
}

void Material::initialize_default_properties() {
    define_property(EMISSION_PROPERTY, MATERIAL_PROPERTY_TYPE_VEC4, "s_emission", Vec4(1, 1, 1, 1));
    define_property(AMBIENT_PROPERTY, MATERIAL_PROPERTY_TYPE_VEC4, "s_ambient", Vec4(1, 1, 1, 1));
    define_property(DIFFUSE_PROPERTY, MATERIAL_PROPERTY_TYPE_VEC4, "s_diffuse", Vec4(1, 1, 1, 1));
    define_property(SPECULAR_PROPERTY, MATERIAL_PROPERTY_TYPE_VEC4, "s_specular", Vec4(1, 1, 1, 1));
    define_property(SHININESS_PROPERTY, MATERIAL_PROPERTY_TYPE_FLOAT, "s_shininess", 0);

    define_property(DIFFUSE_MAP_PROPERTY, MATERIAL_PROPERTY_TYPE_TEXTURE, "s_diffuse_map");
    define_property(NORMAL_MAP_PROPERTY, MATERIAL_PROPERTY_TYPE_TEXTURE, "s_normal_map");
    define_property(SPECULAR_MAP_PROPERTY, MATERIAL_PROPERTY_TYPE_TEXTURE, "s_specular_map");

    define_property(BLENDING_ENABLE_PROPERTY, MATERIAL_PROPERTY_TYPE_BOOL, "s_blending_enabled", false);
    define_property(BLEND_FUNC_PROPERTY, MATERIAL_PROPERTY_TYPE_INT, "s_blend_mode", BLEND_NONE);

    define_property(DEPTH_TEST_ENABLED_PROPERTY, MATERIAL_PROPERTY_TYPE_BOOL, "s_depth_test_enabled", true);
    // define_property(DEPTH_FUNC_PROPERTY, MATERIAL_PROPERTY_TYPE_INT, "s_depth_func", DEPTH_FUNC_LEQUAL);

    define_property(DEPTH_WRITE_ENABLED_PROPERTY, MATERIAL_PROPERTY_TYPE_BOOL, "s_depth_write_enabled", true);

    define_property(CULLING_ENABLED_PROPERTY, MATERIAL_PROPERTY_TYPE_BOOL, "s_culling_enaled", true);
    define_property(CULL_MODE_PROPERTY, MATERIAL_PROPERTY_TYPE_INT, "s_cull_mode", CULL_MODE_BACK_FACE);

    define_property(SHADE_MODEL_PROPERTY, MATERIAL_PROPERTY_TYPE_INT, "s_shade_model", SHADE_MODEL_SMOOTH);

    define_property(LIGHTING_ENABLED_PROPERTY, MATERIAL_PROPERTY_TYPE_BOOL, "s_lighting_enabled", false);
}


}
