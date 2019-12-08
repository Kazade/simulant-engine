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

#include "../deps/jsonic/jsonic.h"

#include "particle_script.h"
#include "../material.h"
#include "../stage.h"
#include "../assets/particle_script.h"
#include "../assets/particles/size_manipulator.h"
#include "../assets/particles/colour_fader.h"

namespace smlt {
namespace loaders {

static smlt::Manipulator* spawn_size_manipulator(ParticleScript* script, std::shared_ptr<ParticleScriptTransaction> ps, jsonic::Node& manipulator) {
    auto m = std::make_shared<SizeManipulator>(script);
    ps->add_manipulator(m);

    if(manipulator.has_key("rate")) {
        /* Just a rate, then it's a linear curve */
        m->set_linear_curve(manipulator["rate"].get<jsonic::Number>());
    } else if(manipulator.has_key("curve")) {
        /* Parse the curve */
        std::string spec = manipulator["curve"].get<jsonic::String>();
        auto first_brace = spec.find('(');
        if(first_brace == std::string::npos || spec.at(spec.size() - 1) != ')') {
            L_WARN(_F("Invalid curve specification {0}. Ignoring.").format(spec));
        } else {
            auto kind = spec.substr(0, first_brace);
            auto args = spec.substr(first_brace + 1, spec.size() - 1);
            if(kind == "linear") {
                auto parts = unicode(args).split(",");
                if(parts.size() > 1) {
                    L_WARN("Too many arguments to linear curve");
                }

                m->set_linear_curve(parts[0].to_float());
            } else if(kind == "bell") {
                auto parts = unicode(args).split(",");
                if(parts.size() != 2) {
                    L_WARN("Wrong number of arguments to bell curve");
                } else {
                    m->set_bell_curve(parts[0].to_float(), parts[1].to_float());
                }
            } else {
                L_WARN(_F("Unknown curve type {0}. Ignoring.").format(kind));
            }
        }
    }

    return m.get();
}

static smlt::Manipulator* spawn_colour_fader_manipulator(ParticleScript* script, std::shared_ptr<ParticleScriptTransaction> ps, jsonic::Node& js) {
    auto parse_colour = [](const std::string& colour) -> smlt::Colour {
        auto parts = unicode(colour).split(" ");
        if(parts.size() == 3) {
            return smlt::Colour(
                parts[0].to_float(),
                parts[1].to_float(),
                parts[2].to_float(),
                1.0f
            );
        } else if(parts.size() == 4) {
            return smlt::Colour(
                parts[0].to_float(),
                parts[1].to_float(),
                parts[2].to_float(),
                parts[3].to_float()
            );
        } else {
            L_WARN("Invalid number of colour components to colour fader");
            return smlt::Colour::WHITE;
        }
    };

    std::vector<smlt::Colour> colours;

    jsonic::Node& colour_array = js["colours"];
    for(auto i = 0u; i < colour_array.length(); ++i) {
        std::string colour = colour_array[(uint32_t) i].get<jsonic::String>();
        colours.push_back(parse_colour(colour));
    }

    bool interpolate = js["interpolate"].get<jsonic::Boolean>();

    auto m = std::make_shared<ColourFader>(script, colours, interpolate);
    ps->add_manipulator(m);
    return m.get();
}


void ParticleScriptLoader::into(Loadable &resource, const LoaderOptions &options) {
    ParticleScript* ps = loadable_to<ParticleScript>(resource);
    jsonic::Node js;

    jsonic::loads(
        std::string((std::istreambuf_iterator<char>(*this->data_)), std::istreambuf_iterator<char>()),
        js
    );


    auto txn = ps->begin_transaction();
    txn->set_name((js.has_key("name")) ? js["name"].get<jsonic::String>(): "");

    if(js.has_key("quota")) {
        txn->set_quota(js["quota"].get<jsonic::Number>());
    }

    if(js.has_key("particle_width")) {
        txn->set_particle_width(js["particle_width"].get<jsonic::Number>());
    }

    if(js.has_key("particle_height")) {
        txn->set_particle_height(js["particle_height"].get<jsonic::Number>());
    }

    if(js.has_key("cull_each")) {
        txn->set_cull_each(js["cull_each"].get<jsonic::Boolean>());
    }

    if(js.has_key("material")) {
        std::string material = js["material"].get<jsonic::String>();

        if(Material::BUILT_IN_NAMES.count(material)) {
            material = Material::BUILT_IN_NAMES.at(material);
        }

        auto mat = ps->asset_manager().new_material_from_file(material);
        txn->set_material(mat);

        /* Apply any specified material properties */
        const std::string MATERIAL_PROPERTY_PREFIX = "material.";
        for(std::string& key: js.keys()) {
            if(key.substr(0, MATERIAL_PROPERTY_PREFIX.length()) == MATERIAL_PROPERTY_PREFIX) {
                auto property_name = key.substr(MATERIAL_PROPERTY_PREFIX.length());

                auto property_id = mat->find_property_id(property_name);
                auto property = mat->property(property_id);

                if(property) {
                    auto type = property->type;
                    if(type == MATERIAL_PROPERTY_TYPE_BOOL) {
                        mat->MaterialObject::set_property_value<bool>(property_id, (bool) js[key].get<jsonic::Boolean>());
                    } else if(type == MATERIAL_PROPERTY_TYPE_FLOAT) {
                        mat->MaterialObject::set_property_value<float>(property_id, js[key].get<jsonic::Number>());
                    } else if(type == MATERIAL_PROPERTY_TYPE_INT) {
                        if(property_name == BLEND_FUNC_PROPERTY) {
                            mat->MaterialObject::set_property_value<int>(property_id, (int) blend_type_from_name(js[key].get<jsonic::String>()));
                        } else {
                            mat->MaterialObject::set_property_value<int>(property_id, (int) js[key].get<jsonic::Number>());
                        }
                    } else if(type == MATERIAL_PROPERTY_TYPE_TEXTURE) {
                        auto dirname = kfs::path::dir_name(filename_.encode());
                        /* Add the local directory for image lookups */
                        auto remove = vfs->add_search_path(dirname);
                        auto tex = ps->asset_manager().new_texture_from_file(js[key].get<jsonic::String>());
                        mat->set_property_value(property_id, tex);
                        if(remove) {
                            // Remove the path if necessary
                            vfs->remove_search_path(dirname);
                        }
                    } else {
                        L_ERROR(
                            _F("Unhandled material property type {0}, please report.").format(type)
                        );
                    }
                }
            }
        }
    }

    if(js.has_key("emitters")) {
        L_DEBUG("Loading emitters");

        jsonic::Node& emitters = js["emitters"];
        for(uint32_t i = 0; i < emitters.length(); ++i) {
            jsonic::Node& emitter = emitters[i];

            Emitter new_emitter;
            if(emitter.has_key("type")) {
                auto emitter_type = emitter["type"].get<jsonic::String>();
                L_DEBUG(_F("Emitter {0} has type {1}").format(i, emitter_type));
                new_emitter.type = (emitter_type == "point") ? PARTICLE_EMITTER_POINT : PARTICLE_EMITTER_BOX;
            }

            if(emitter.has_key("direction")) {
                auto parts = unicode(emitter["direction"].get<jsonic::String>()).split(" ");
                //FIXME: check length
                new_emitter.direction = smlt::Vec3(
                    parts.at(0).to_float(),
                    parts.at(1).to_float(),
                    parts.at(2).to_float()
                );
            }

            if(emitter.has_key("velocity")) {
                float x = emitter["velocity"].get<jsonic::Number>();
                new_emitter.velocity_range = std::make_pair(x, x);
            }

            if(emitter.has_key("width")) {
                new_emitter.dimensions.x = emitter["width"].get<jsonic::Number>();
            }

            if(emitter.has_key("height")) {
                new_emitter.dimensions.y = emitter["height"].get<jsonic::Number>();
            }

            if(emitter.has_key("depth")) {
                new_emitter.dimensions.z = emitter["depth"].get<jsonic::Number>();
            }

            if(emitter.has_key("ttl")) {
                float x = emitter["ttl"].get<jsonic::Number>();
                new_emitter.ttl_range = std::make_pair(x, x);
            } else {
                if(emitter.has_key("ttl_min") && emitter.has_key("ttl_max")) {
                    new_emitter.ttl_range = std::make_pair(
                        emitter["ttl_min"].get<jsonic::Number>(),
                        emitter["ttl_max"].get<jsonic::Number>()
                    );
                } else if(emitter.has_key("ttl_min")) {
                    new_emitter.ttl_range = std::make_pair(
                        emitter["ttl_min"].get<jsonic::Number>(),
                        new_emitter.ttl_range.second
                    );
                } else if(emitter.has_key("ttl_max")) {
                    new_emitter.ttl_range = std::make_pair(
                        new_emitter.ttl_range.first,
                        emitter["ttl_max"].get<jsonic::Number>()
                    );
                }
            }

            if(emitter.has_key("duration")) {
                float x = emitter["duration"].get<jsonic::Number>();
                new_emitter.duration_range = std::make_pair(x, x);
            }

            if(emitter.has_key("repeat_delay")) {
                float x = emitter["repeat_delay"].get<jsonic::Number>();
                new_emitter.repeat_delay_range = std::make_pair(x, x);
            }

            if(emitter.has_key("angle")) {
                new_emitter.angle = Degrees(emitter["angle"].get<jsonic::Number>());
            }

            if(emitter.has_key("colour")) {
                auto parts = unicode(emitter["colour"].get<jsonic::String>()).split(" ");
                new_emitter.colour = smlt::Colour(
                    parts.at(0).to_float(), parts.at(1).to_float(), parts.at(2).to_float(), parts.at(3).to_float()
                );
            }

            if(emitter.has_key("emission_rate")) {
                new_emitter.emission_rate = emitter["emission_rate"].get<jsonic::Number>();
            }

            txn->push_emitter(new_emitter);
        }

        if(js.has_key("manipulators")) {
            jsonic::Node& manipulators = js["manipulators"];
            for(uint32_t i = 0; i < manipulators.length(); ++i) {
                auto& manipulator = manipulators[i];

                if(manipulator["type"].get<jsonic::String>() == "size") {
                    spawn_size_manipulator(ps, txn, manipulator);
                } else if(manipulator["type"].get<jsonic::String>() == "colour_fader") {
                    spawn_colour_fader_manipulator(ps, txn, manipulator);
                }
            }
        }
    }

    txn->commit();
}

}
}

