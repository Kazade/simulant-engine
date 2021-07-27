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

#include "material_script.h"
#include "../logging.h"
#include "../types.h"
#include "../assets/material.h"
#include "../shortcuts.h"
#include "../asset_manager.h"
#include "../utils/gl_thread_check.h"
#include "../renderers/renderer.h"
#include "../generic/raii.h"
#include "../window.h"

#include "../utils/json.h"

#if !defined(__DREAMCAST__) && !defined(__PSP__)
#include "../renderers/gl2x/gpu_program.h"
#endif

namespace smlt {

MaterialScript::MaterialScript(std::shared_ptr<std::istream> data, const Path& filename):
    filename_(filename),
    data_(*data.get()) {

}

template<MaterialPropertyType MT, typename T>
static void define_property(Material& material, JSONIterator prop) {
    std::string name = prop["name"]->to_str(); // FIXME: Sanitize!

    if(!prop["default"]->is_null()) {
        auto def = (T) json_auto_cast<T>(prop["default"]).value();

        material.set_property_value(name, def);
    } else {
        material.set_property_value(name, T());
    }
}

template<>
void define_property<MATERIAL_PROPERTY_TYPE_TEXTURE, TexturePtr>(Material& material, JSONIterator prop) {
    std::string name = prop["name"]->to_str(); // FIXME: Sanitize!

    if(prop.has_key("default") && !prop["default"]->is_null()) {
        std::string def = prop["default"]->to_str();

        auto texture = material.asset_manager().new_texture_from_file(def);
        material.set_property_value(name, texture);
    } else {
        material.set_property_value(name, TexturePtr());
    }
}

void read_property_values(Material& mat, MaterialObject& holder, JSONIterator json) {
    if(json.has_key("property_values")) {
        for(auto& key: json["property_values"].keys()) {
            auto& value = json["property_values"][key];

            MaterialPropertyType property_type;
            if(!mat.property_type(key.c_str(), &property_type)) {
                S_ERROR("Unrecognized property: {0}", key);
                continue;
            }
            if(property_type == MATERIAL_PROPERTY_TYPE_BOOL) {
                if(!value.is_bool()) {
                    S_ERROR("Invalid property value for: {0}", key);
                    continue;
                }

                holder.set_property_value(key.c_str(), value->to_bool().value());
            } else if(property_type == MATERIAL_PROPERTY_TYPE_VEC3) {
                if(!value.is_string()) {
                    S_ERROR("Invalid property value for: {0}", key);
                    continue;
                }

                auto parts = unicode(value->to_str()).split(" ");

                if(parts.size() != 3) {
                    S_ERROR(_F("Invalid value for property: {0}").format(key));
                    continue;
                }

                float x = parts[0].to_float();
                float y = parts[1].to_float();
                float z = parts[2].to_float();

                holder.set_property_value(key.c_str(), Vec3(x, y, z));
            } else if(property_type == MATERIAL_PROPERTY_TYPE_VEC4) {
                if(!value.is_string()) {
                    S_ERROR("Invalid property value for: {0}", key);
                    continue;
                }

                auto parts = unicode(value->to_str()).split(" ");

                if(parts.size() != 4) {
                    S_ERROR("Invalid value for property: {0}", key);
                    continue;
                }

                float x = parts[0].to_float();
                float y = parts[1].to_float();
                float z = parts[2].to_float();
                float w = parts[3].to_float();

                holder.set_property_value(key.c_str(), Vec4(x, y, z, w));
            } else if(property_type == MATERIAL_PROPERTY_TYPE_FLOAT || property_type == MATERIAL_PROPERTY_TYPE_INT) {
                if(property_type == MATERIAL_PROPERTY_TYPE_FLOAT) {
                    holder.set_property_value(key.c_str(), value->to_int().value());
                } else {
                    if(value.is_string()) {
                        /* Special cases for enums - need a better way to handle this */
                        if(key == BLEND_FUNC_PROPERTY_NAME) {
                            std::string v = value->to_str();
                            BlendType type = blend_type_from_name(v.c_str());
                            holder.set_blend_func(type);
                        } else if(key == SHADE_MODEL_PROPERTY_NAME) {
                            std::string v = value->to_str();
                            holder.set_shade_model(shade_model_from_name(v.c_str()));
                        } else if(key == CULL_MODE_PROPERTY_NAME) {
                            std::string v = value->to_str();
                            holder.set_cull_mode(cull_mode_from_name(v.c_str()));
                        }
                    } else {
                        holder.set_property_value(key.c_str(), (int32_t) value->to_int().value());
                    }
                }
            } else if(property_type == MATERIAL_PROPERTY_TYPE_TEXTURE) {
                std::string path = value->to_str();
                auto tex = mat.asset_manager().new_texture_from_file(path);
                holder.set_property_value(key.c_str(), tex);
            } else {
                S_ERROR("Unhandled property type");
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
            S_ERROR("Unrecognised property type: {0}", kind);
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

            std::string kind = prop["type"].get<jsonic::String>();
            auto prop_type = lookup_material_property_type(kind);

            switch(prop_type) {
                case MATERIAL_PROPERTY_TYPE_BOOL: {
                    define_property<MATERIAL_PROPERTY_TYPE_BOOL, bool>(material, prop);
                } break;
                case MATERIAL_PROPERTY_TYPE_INT: {
                    define_property<MATERIAL_PROPERTY_TYPE_INT, int32_t>(material, prop);
                } break;
                case MATERIAL_PROPERTY_TYPE_FLOAT: {
                    define_property<MATERIAL_PROPERTY_TYPE_FLOAT, float>(material, prop);
                } break;
                case MATERIAL_PROPERTY_TYPE_TEXTURE: {
                    define_property<MATERIAL_PROPERTY_TYPE_TEXTURE, TexturePtr>(material, prop);
                } break;
            default:
                S_ERROR("Unhandled property type: {0}", kind);
            }
        }
    }

    read_property_values(material, material, json);

    material.set_pass_count(json["passes"].length());

    /* Feels dirty... */
    Window* window = material.asset_manager().window;
    Renderer* renderer = window->renderer;

    for(uint32_t i = 0u; i < json["passes"].length(); ++i) {
        jsonic::Node& pass = json["passes"][i];

        std::string iteration = (pass.has_key("iteration")) ? pass["iteration"].get<jsonic::String>() : "once";

        if(iteration == "once") {
            material.pass(i)->set_iteration_type(ITERATION_TYPE_ONCE);
        } else if(iteration == "once_per_light") {
            material.pass(i)->set_iteration_type(ITERATION_TYPE_ONCE_PER_LIGHT);
        } else {
            S_ERROR("Unsupported iteration type: {0}", iteration);
        }

        read_property_values(material, *material.pass(i), pass);

        /* If we support gpu programs, then load any shaders */
        if(renderer->supports_gpu_programs() && pass.has_key("vertex_shader") && pass.has_key("fragment_shader")) {
            std::string vertex_shader_path = pass["vertex_shader"].get<jsonic::String>();
            std::string fragment_shader_path = pass["fragment_shader"].get<jsonic::String>();

            auto parent_dir = Path(kfs::path::dir_name(filename_.str()));

            bool added = false;

            // Make sure we always remove the search path we add (if it didn't exist before)
            raii::Finally then([&]() {
                if(added) {
                    window->vfs->remove_search_path(parent_dir);
                }
            });

            added = window->vfs->add_search_path(parent_dir);

            auto vertex_shader = window->vfs->read_file(vertex_shader_path);
            auto fragment_shader = window->vfs->read_file(fragment_shader_path);

            auto program = renderer->new_or_existing_gpu_program(
                std::string{std::istreambuf_iterator<char>(*vertex_shader), std::istreambuf_iterator<char>()},
                std::string{std::istreambuf_iterator<char>(*fragment_shader), std::istreambuf_iterator<char>()}
            );

            material.pass(i)->set_gpu_program(program);
        }
    }
}

namespace loaders {

void MaterialScriptLoader::into(Loadable& resource, const LoaderOptions&) {
    Material* mat = loadable_to<Material>(resource);
    parser_->generate(*mat);
}

}
}
