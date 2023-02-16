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

#include "particle_script.h"
#include "../assets/material.h"
#include "../stage.h"
#include "../assets/particle_script.h"
#include "../assets/particles/size_manipulator.h"
#include "../assets/particles/colour_fader.h"
#include "../assets/particles/direction_manipulator.h"
#include "../assets/particles/direction_noise_random_manipulator.h"

#include "../vfs.h"

#include "../utils/json.h"

namespace smlt {
namespace loaders {

static smlt::Manipulator* spawn_size_manipulator(ParticleScript* ps, JSONIterator& manipulator) {
    auto m = std::make_shared<SizeManipulator>(ps);
    ps->add_manipulator(m);

    if(manipulator->has_key("rate")) {
        /* Just a rate, then it's a linear curve */
        m->set_linear_curve(manipulator["rate"]->to_int().value());
    } else if(manipulator->has_key("curve")) {
        /* Parse the curve */
        std::string spec = manipulator["curve"]->to_str().value();
        auto first_brace = spec.find('(');
        if(first_brace == std::string::npos || spec.at(spec.size() - 1) != ')') {
            S_WARN("Invalid curve specification {0}. Ignoring.", spec);
        } else {
            auto kind = spec.substr(0, first_brace);
            auto args = spec.substr(first_brace + 1, spec.size() - 1);
            if(kind == "linear") {
                auto parts = unicode(args).split(",");
                if(parts.size() > 1) {
                    S_WARN("Too many arguments to linear curve");
                }

                m->set_linear_curve(parts[0].to_float());
            } else if(kind == "bell") {
                auto parts = unicode(args).split(",");
                if(parts.size() != 2) {
                    S_WARN("Wrong number of arguments to bell curve");
                } else {
                    m->set_bell_curve(parts[0].to_float(), parts[1].to_float());
                }
            } else {
                S_WARN("Unknown curve type {0}. Ignoring.", kind);
            }
        }
    }

    return m.get();
}

static auto parse_colour = [](const std::string& colour) -> smlt::Colour {
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
        S_WARN("Invalid number of colour components to colour fader");
        return smlt::Colour::WHITE;
    }
};

static auto parse_vec3 = [](const std::string& dir) -> smlt::Vec3 {
    auto parts = unicode(dir).split(" ");
    if(parts.size() == 3) {
        return smlt::Vec3(
            parts[0].to_float(),
            parts[1].to_float(),
            parts[2].to_float()
        );
    } else {
        S_WARN("Invalid number of vector components to direction manipulator");
        return smlt::Vec3();
    }
};

static smlt::Manipulator* spawn_colour_fader_manipulator(ParticleScript* ps, JSONIterator& js) {
    std::vector<smlt::Colour> colours;

    auto colour_array = js["colours"];
    for(auto i = 0u; i < colour_array->size(); ++i) {
        std::string colour = colour_array[(uint32_t) i]->to_str().value();
        colours.push_back(parse_colour(colour));
    }

    bool interpolate = js["interpolate"]->to_bool().value_or(true);

    auto m = std::make_shared<ColourFader>(ps, colours, interpolate);
    ps->add_manipulator(m);
    return m.get();
}

static smlt::Manipulator* spawn_direction_manipulator(ParticleScript* ps, JSONIterator& js) {
    auto dir = (js->has_key("force") ? parse_vec3(js["force"]->to_str().value_or("")) : smlt::Vec3());

    auto m = std::make_shared<DirectionManipulator>(ps, dir);
    ps->add_manipulator(m);
    return m.get();
}

static smlt::Manipulator* spawn_direction_noise_random_manipulator(ParticleScript* ps, JSONIterator& js) {
    auto dir = (js->has_key("force") ? parse_vec3(js["force"]->to_str().value_or("")) : smlt::Vec3());
    auto noise_amount = (js->has_key("noise_amount") ? parse_vec3(js["noise_amount"]->to_str().value_or("")) : smlt::Vec3());

    auto m = std::make_shared<DirectionNoiseRandomManipulator>(ps, dir, noise_amount);
    ps->add_manipulator(m);
    return m.get();
}

void ParticleScriptLoader::into(Loadable &resource, const LoaderOptions &options) {
    _S_UNUSED(options);

    ParticleScript* ps = loadable_to<ParticleScript>(resource);

    auto js = json_read(this->data_);

    ps->set_name(js["name"]->to_str().value());

    if(js->has_key("quota")) {
        ps->set_quota(js["quota"]->to_int().value_or(0));
    }

    if(js->has_key("particle_width")) {
        ps->set_particle_width(js["particle_width"]->to_float().value_or(0.0f));
    }

    if(js->has_key("particle_height")) {
        ps->set_particle_height(js["particle_height"]->to_float().value_or(0.0f));
    }

    if(js->has_key("cull_each")) {
        ps->set_cull_each(js["cull_each"]->to_bool().value_or(false));
    }

    if(js->has_key("material")) {
        std::string material = js["material"]->to_str().value();

        if(Material::BUILT_IN_NAMES.count(material)) {
            material = Material::BUILT_IN_NAMES.at(material);
        }

        auto mat = ps->asset_manager().new_material_from_file(material);
        ps->set_material(mat);

        /* Apply any specified material properties */
        const std::string MATERIAL_PROPERTY_PREFIX = "material.";
        for(std::string& key: js->keys()) {
            if(key.substr(0, MATERIAL_PROPERTY_PREFIX.length()) == MATERIAL_PROPERTY_PREFIX) {
                auto property_name = key.substr(MATERIAL_PROPERTY_PREFIX.length());

                MaterialPropertyType type;
                if(mat->property_type(property_name.c_str(), &type)) {
                    if(type == MATERIAL_PROPERTY_TYPE_BOOL) {
                        mat->set_property_value(property_name.c_str(), (bool) js[key]->to_bool().value());
                    } else if(type == MATERIAL_PROPERTY_TYPE_FLOAT) {
                        mat->set_property_value(property_name.c_str(), (js[key]->to_float().value()));
                    } else if(type == MATERIAL_PROPERTY_TYPE_INT) {
                        if(property_name == BLEND_FUNC_PROPERTY_NAME) {
                            mat->set_blend_func(blend_type_from_name(js[key]->to_str().value().c_str()));
                        } else {
                            // FIXME: There are a load of missing enums here!
                            mat->set_property_value(property_name.c_str(), (int32_t) js[key]->to_int().value());
                        }
                    } else if(type == MATERIAL_PROPERTY_TYPE_TEXTURE) {
                        auto dirname = kfs::path::dir_name(filename_.str());
                        /* Add the local directory for image lookups */
                        auto remove = vfs->add_search_path(dirname);
                        auto tex = ps->asset_manager().new_texture_from_file(js[key]->to_str().value());
                        mat->set_property_value(property_name.c_str(), tex);
                        if(remove) {
                            // Remove the path if necessary
                            vfs->remove_search_path(dirname);
                        }
                    } else {
                        S_ERROR(
                            "Unhandled material property type {0}, please report.", type
                        );
                    }
                } else {
                    S_ERROR("Unrecognised material property: {0}", property_name);
                }
            }
        }
    }

    if(js->has_key("emitters")) {
        S_DEBUG("Loading emitters");

        auto emitters = js["emitters"];
        for(uint32_t i = 0; i < emitters->size(); ++i) {
            auto emitter = emitters[i];

            Emitter new_emitter;
            if(emitter->has_key("type")) {
                auto emitter_type = emitter["type"]->to_str().value_or("point");
                S_DEBUG("Emitter {0} has type {1}", i, emitter_type);
                new_emitter.type = (emitter_type == "point") ? PARTICLE_EMITTER_POINT : PARTICLE_EMITTER_BOX;
            }

            if(emitter->has_key("direction")) {
                auto parts = unicode(emitter["direction"]->to_str().value_or("0 1 0")).split(" ");
                //FIXME: check length
                new_emitter.direction = smlt::Vec3(
                    parts.at(0).to_float(),
                    parts.at(1).to_float(),
                    parts.at(2).to_float()
                );
            }

            if(emitter->has_key("velocity")) {
                float x = emitter["velocity"]->to_float().value();
                new_emitter.velocity_range = std::make_pair(x, x);
            }

            if(emitter->has_key("width")) {
                new_emitter.dimensions.x = emitter["width"]->to_float().value();
            }

            if(emitter->has_key("height")) {
                new_emitter.dimensions.y = emitter["height"]->to_float().value();
            }

            if(emitter->has_key("depth")) {
                new_emitter.dimensions.z = emitter["depth"]->to_float().value();
            }

            if(emitter->has_key("ttl")) {
                float x = emitter["ttl"]->to_float().value();
                new_emitter.ttl_range = std::make_pair(x, x);
            } else {
                if(emitter->has_key("ttl_min") && emitter->has_key("ttl_max")) {
                    new_emitter.ttl_range = std::make_pair(
                        emitter["ttl_min"]->to_float().value(),
                        emitter["ttl_max"]->to_float().value()
                    );
                } else if(emitter->has_key("ttl_min")) {
                    new_emitter.ttl_range = std::make_pair(
                        emitter["ttl_min"]->to_float().value(),
                        new_emitter.ttl_range.second
                    );
                } else if(emitter->has_key("ttl_max")) {
                    new_emitter.ttl_range = std::make_pair(
                        new_emitter.ttl_range.first,
                        emitter["ttl_max"]->to_float().value()
                    );
                }
            }

            if(emitter->has_key("duration")) {
                float x = emitter["duration"]->to_float().value();
                new_emitter.duration_range = std::make_pair(x, x);
            }

            if(emitter->has_key("repeat_delay")) {
                float x = emitter["repeat_delay"]->to_float().value();
                new_emitter.repeat_delay_range = std::make_pair(x, x);
            }

            if(emitter->has_key("angle")) {
                new_emitter.angle = Degrees(emitter["angle"]->to_float().value());
            }

            if(emitter->has_key("colour")) {
                auto colour = parse_colour(emitter["colour"]->to_str().value_or("0 0 0 0"));
                new_emitter.colours = {colour};
            } else if(emitter->has_key("colours")) {
                auto colours = emitter["colours"];
                new_emitter.colours.clear();
                for(int c = 0; c < colours->size(); ++c) {
                    auto colour = parse_colour(colours[c]->to_str().value_or("0 0 0 0"));
                    new_emitter.colours.push_back(colour);
                }
            }

            if(emitter->has_key("emission_rate")) {
                new_emitter.emission_rate = emitter["emission_rate"]->to_float().value();
            }

            ps->push_emitter(new_emitter);
        }

        if(js->has_key("manipulators")) {
            auto manipulators = js["manipulators"];
            for(uint32_t i = 0; i < manipulators->size(); ++i) {
                auto manipulator = manipulators[i];

                if(manipulator["type"]->to_str().value() == "size") {
                    spawn_size_manipulator(ps, manipulator);
                } else if(manipulator["type"]->to_str().value() == "colour_fader") {
                    spawn_colour_fader_manipulator(ps, manipulator);
                } else if(manipulator["type"]->to_str().value() == "direction") {
                    spawn_direction_manipulator(ps, manipulator);
                } else if(manipulator["type"]->to_str().value() == "direction_noise_random") {
                    spawn_direction_noise_random_manipulator(ps, manipulator);
                }
            }
        }
    }
}

}
}

