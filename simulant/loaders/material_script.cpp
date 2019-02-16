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

#include "material_script.h"
#include "../deps/kazlog/kazlog.h"
#include "../types.h"
#include "../material.h"
#include "../shortcuts.h"
#include "../asset_manager.h"
#include "../utils/gl_thread_check.h"
#include "../renderers/renderer.h"
#include "../generic/raii.h"
#include "../deps/jsonic/jsonic.h"


#ifndef _arch_dreamcast
#include "../renderers/gl2x/gpu_program.h"
#endif

namespace smlt {

MaterialScript::MaterialScript(std::shared_ptr<std::istream> data, const unicode& filename):
    filename_(filename),
    data_(*data.get()) {

}

template<MaterialPropertyType MT>
static void define_property(Material& material, jsonic::Node& prop) {
    std::string name = prop["name"]; // FIXME: Sanitize!

    typedef typename _material_impl::TypeForMaterialType<MT>::type T;

    if(!prop["default"].is_none()) {
        auto def = (T) prop["default"];

        material.define_property(
            MT,
            name,
            def
        );
    } else {
        material.define_property(
            MT,
            name,
            T()
        );
    }
}

template<>
void define_property<MATERIAL_PROPERTY_TYPE_TEXTURE>(Material& material, jsonic::Node& prop) {
    std::string name = prop["name"]; // FIXME: Sanitize!

    if(prop.has_key("default") && !prop["default"].is_none()) {
        std::string def = prop["default"];

        TextureID tex_id = material.resource_manager().new_texture_from_file(def);

        material.define_property(
            MATERIAL_PROPERTY_TYPE_TEXTURE,
            name,
            tex_id
        );
    } else {
        material.define_property(
            MATERIAL_PROPERTY_TYPE_TEXTURE,
            name,
            TextureID()
        );
    }
}

void read_property_values(Material& mat, _material_impl::PropertyValueHolder& holder, jsonic::Node& json) {
    if(json.has_key("property_values")) {
        for(auto& key: json["property_values"].keys()) {
            auto& value = json["property_values"][key];

            if(!holder.top_level()->property_is_defined(key)) {
                L_ERROR(_F("Unrecognized property: {0}").format(key));
                continue;
            }

            auto property_type = holder.top_level()->defined_property_type(key);

            if(property_type == MATERIAL_PROPERTY_TYPE_BOOL) {
                if(!value.is_bool()) {
                    L_ERROR(_F("Invalid property value for: {0}").format(key));
                    continue;
                }

                holder.set_property_value(key, (bool) value);                
            } else if(property_type == MATERIAL_PROPERTY_TYPE_VEC3) {
                if(!value.is_string()) {
                    L_ERROR(_F("Invalid property value for: {0}").format(key));
                    continue;
                }

                auto parts = unicode(std::string(value)).split(" ");

                if(parts.size() != 3) {
                    L_ERROR(_F("Invalid value for property: {0}").format(key));
                    continue;
                }

                float x = parts[0].to_float();
                float y = parts[1].to_float();
                float z = parts[2].to_float();

                holder.set_property_value(key, Vec3(x, y, z));
            } else if(property_type == MATERIAL_PROPERTY_TYPE_VEC4) {
                if(!value.is_string()) {
                    L_ERROR(_F("Invalid property value for: {0}").format(key));
                    continue;
                }

                auto parts = unicode(std::string(value)).split(" ");

                if(parts.size() != 4) {
                    L_ERROR(_F("Invalid value for property: {0}").format(key));
                    continue;
                }

                float x = parts[0].to_float();
                float y = parts[1].to_float();
                float z = parts[2].to_float();
                float w = parts[3].to_float();

                holder.set_property_value(key, Vec4(x, y, z, w));
            } else if(property_type == MATERIAL_PROPERTY_TYPE_FLOAT || property_type == MATERIAL_PROPERTY_TYPE_INT) {
                if(!value.is_string()) {
                    L_ERROR(_F("Invalid property value for: {0}").format(key));
                    continue;
                }

                if(property_type == MATERIAL_PROPERTY_TYPE_FLOAT) {
                    holder.set_property_value(key, (float) value);
                } else {
                    /* Special cases for enums - need a better way to handle this */
                    if(key == BLEND_FUNC_PROPERTY) {
                        std::string v = value;
                        if(v == "alpha") {
                            holder.set_property_value(key, (int) BLEND_ALPHA);
                        } else if(v == "add") {
                            holder.set_property_value(key, (int) BLEND_ADD);
                        } else if(v == "colour") {
                            holder.set_property_value(key, (int) BLEND_COLOUR);
                        } else if(v == "modulate") {
                            holder.set_property_value(key, (int) BLEND_MODULATE);
                        } else if(v == "one_one_minus_alpha") {
                            holder.set_property_value(key, (int) BLEND_ONE_ONE_MINUS_ALPHA);
                        } else {
                            L_WARN(_F("Unrecognised blend value {0}").format(v));
                            holder.set_property_value(key, (int) BLEND_NONE);
                        }
                    } else if(key == SHADE_MODEL_PROPERTY) {
                        std::string v = value;
                        if(v == "smooth") {
                            holder.set_shade_model(SHADE_MODEL_SMOOTH);
                        } else if(v == "front_face") {
                            holder.set_shade_model(SHADE_MODEL_FLAT);
                        } else {
                            L_WARN(_F("Unrecognised shade model value {0}").format(v));
                        }
                    } else if(key == CULL_MODE_PROPERTY) {
                        std::string v = value;
                        if(v == "back_face") {
                            holder.set_cull_mode(CULL_MODE_BACK_FACE);
                        } else if(v == "front_face") {
                            holder.set_cull_mode(CULL_MODE_FRONT_FACE);
                        } else if(v == "front_and_back_face") {
                            holder.set_cull_mode(CULL_MODE_FRONT_AND_BACK_FACE);
                        } else if(v == "none") {
                            holder.set_cull_mode(CULL_MODE_NONE);
                        } else {
                            L_WARN(_F("Unrecognised cull value {0}").format(v));
                        }
                    } else {
                        holder.set_property_value(key, (int) value);
                    }
                }
            } else if(property_type == MATERIAL_PROPERTY_TYPE_TEXTURE) {
                std::string path = value;
                auto tex_id = mat.resource_manager().new_texture_from_file(
                    path
                );

                holder.set_property_value(key, tex_id);
            } else {
                L_ERROR("Unhandled property type");
            }
        }
    }
}

void MaterialScript::generate(Material& material) {
    auto lookup_material_property_type = [](const std::string& kind) -> MaterialPropertyType {
        if(kind == "bool") {
            return MATERIAL_PROPERTY_TYPE_BOOL;
        } else if(kind == "texture") {
            return MATERIAL_PROPERTY_TYPE_TEXTURE;
        } else if(kind == "float") {
            return MATERIAL_PROPERTY_TYPE_FLOAT;
        } else if(kind == "int") {
            return MATERIAL_PROPERTY_TYPE_INT;
        } else if(kind == "vec2") {
            return MATERIAL_PROPERTY_TYPE_VEC2;
        } else if(kind == "vec3") {
            return MATERIAL_PROPERTY_TYPE_VEC3;
        } else if(kind == "vec4") {
            return MATERIAL_PROPERTY_TYPE_VEC4;
        } else {
            L_ERROR(_F("Unrecognised property type: {0}").format(kind));
            return MATERIAL_PROPERTY_TYPE_FLOAT;
        }
    };

    jsonic::Node json;
    jsonic::loads(
        std::string{std::istreambuf_iterator<char>(data_), std::istreambuf_iterator<char>()},
        json
    );

    if(!json.has_key("passes")) {
        throw std::runtime_error("Material is missing the passes key");
    }

    /* Load any custom properties */
    if(json.has_key("custom_properties")) {
        jsonic::Node& custom_props = json["custom_properties"];

        for(uint32_t i = 0u; i < custom_props.length(); ++i) {
            jsonic::Node& prop = custom_props[i];

            std::string kind = prop["type"];
            auto prop_type = lookup_material_property_type(kind);

            switch(prop_type) {
                case MATERIAL_PROPERTY_TYPE_BOOL: {
                    define_property<MATERIAL_PROPERTY_TYPE_BOOL>(material, prop);
                } break;
                case MATERIAL_PROPERTY_TYPE_INT: {
                    define_property<MATERIAL_PROPERTY_TYPE_INT>(material, prop);
                } break;
                case MATERIAL_PROPERTY_TYPE_FLOAT: {
                    define_property<MATERIAL_PROPERTY_TYPE_FLOAT>(material, prop);
                } break;
                case MATERIAL_PROPERTY_TYPE_TEXTURE: {
                    define_property<MATERIAL_PROPERTY_TYPE_TEXTURE>(material, prop);
                } break;
            default:
                L_ERROR(_F("Unhandled property type: {0}").format(kind));
            }
        }
    }

    read_property_values(material, material, json);

    material.set_pass_count(json["passes"].length());

    /* Feels dirty... */
    Window* window = material.resource_manager().window;
    Renderer* renderer = window->renderer;

    for(uint32_t i = 0u; i < json["passes"].length(); ++i) {
        jsonic::Node& pass = json["passes"][i];

        std::string iteration = (pass.has_key("iteration")) ? (std::string) pass["iteration"] : "once";

        if(iteration == "once") {
            material.pass(i)->set_iteration_type(ITERATION_TYPE_ONCE);
        } else if(iteration == "once_per_light") {
            material.pass(i)->set_iteration_type(ITERATION_TYPE_ONCE_PER_LIGHT);
        } else {
            L_ERROR(_F("Unsupported iteration type: {0}").format(iteration));
        }

        read_property_values(material, *material.pass(i), pass);

        /* If we support gpu programs, then load any shaders */
        if(renderer->supports_gpu_programs() && pass.has_key("vertex_shader") && pass.has_key("fragment_shader")) {
            std::string vertex_shader_path = pass["vertex_shader"];
            std::string fragment_shader_path = pass["fragment_shader"];

            auto parent_dir = unicode(kfs::path::dir_name(filename_.encode()));

            bool added = false;

            // Make sure we always remove the search path we add (if it didn't exist before)
            raii::Finally then([&]() {
                if(added) {
                    window->resource_locator->remove_search_path(parent_dir);
                }
            });

            added = window->resource_locator->add_search_path(parent_dir);

            auto vertex_shader = window->resource_locator->read_file(vertex_shader_path);
            auto fragment_shader = window->resource_locator->read_file(fragment_shader_path);

            auto program = renderer->new_or_existing_gpu_program(
                std::string{std::istreambuf_iterator<char>(*vertex_shader), std::istreambuf_iterator<char>()},
                std::string{std::istreambuf_iterator<char>(*fragment_shader), std::istreambuf_iterator<char>()}
            );

            material.pass(i)->set_gpu_program(program);
        }
    }
}

namespace loaders {

void MaterialScriptLoader::into(Loadable& resource, const LoaderOptions& options) {
    Material* mat = loadable_to<Material>(resource);
    parser_->generate(*mat);
}

}
}
