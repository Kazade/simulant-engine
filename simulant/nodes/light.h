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

#ifndef LIGHT_H_INCLUDED
#define LIGHT_H_INCLUDED

#include "../generic/managed.h"
#include "../generic/identifiable.h"
#include "../generic/manual_object.h"
#include "../types.h"

#include "stage_node.h"

namespace smlt {

struct LightParams {
    LightType type;
    Vec3 position_or_direction;
    Colour color;
};

class Light :
    public ContainerNode,
    public ChainNameable<Light> {

public:
    typedef std::shared_ptr<Light> ptr;

    Light(Scene* owner);

    void set_type(LightType type);

    /*
     *  Direction (ab)uses the light's position.
     *  Setting the direction implicitly sets the light type to directional
     *
     *  Direction is stored reversed in the position.
     */
    Vec3 direction() const {
        return absolute_position();
    }

    void set_direction(float x, float y, float z) {
        set_direction(Vec3(x, y, z));
    }

    void set_range(float range) {
        range_ = range;
        bounds_ = AABB(Vec3(), range_);
    }

    void set_direction(const Vec3& dir) {
        set_type(LIGHT_TYPE_DIRECTIONAL);
        move_to(-dir.x, -dir.y, -dir.z);
    }

    void set_diffuse(const smlt::Colour& colour) {
        diffuse_ = colour;
    }

    void set_ambient(const smlt::Colour& colour) {
        ambient_ = colour;
    }

    void set_specular(const smlt::Colour& colour) {
        specular_ = colour;
    }

    LightType light_type() const { return type_; }
    const smlt::Colour& ambient() const { return ambient_; }
    const smlt::Colour& diffuse() const { return diffuse_; }
    const smlt::Colour& specular() const { return specular_; }

    /** Returns the owner stage's global ambient value. */
    smlt::Colour global_ambient() const;

    void set_attenuation(float range, float constant, float linear, float quadratic);
    void set_attenuation_from_range(float range);

    float range() const { return range_; }
    float constant_attenuation() const { return const_attenuation_; }
    float linear_attenuation() const { return linear_attenuation_; }
    float quadratic_attenuation() const { return quadratic_attenuation_; }

    const AABB& aabb() const override {
        return bounds_;
    }

private:
    LightType type_;

    smlt::Colour ambient_;
    smlt::Colour diffuse_;
    smlt::Colour specular_;

    AABB bounds_;
    float range_;
    float const_attenuation_;
    float linear_attenuation_;
    float quadratic_attenuation_;
};

extern const Colour DEFAULT_LIGHT_COLOUR;

class PointLightParams {
    Vec3 position;
    Colour color;

public:
    PointLightParams():
        PointLightParams(Vec3(), DEFAULT_LIGHT_COLOUR) {}

    PointLightParams(const Vec3& position, const Colour& color):
        position(position),
        color(color) {}
};

class PointLight : public Light {
    PointLight(Scene* owner):
        Light(owner) {}
};

struct DirectionalLightParams {
    Vec3 direction;
    Colour color;

    DirectionalLightParams():
        DirectionalLightParams(Vec3(1, -0.5, 0), DEFAULT_LIGHT_COLOUR) {}

    DirectionalLightParams(const Vec3& direction, const Colour& color):
        direction(direction),
        color(color) {}
};

class DirectionalLight : public Light {
public:
    DirectionalLight(Scene* owner):
        Light(owner) {}
};

template<>
struct stage_node_traits<Light> {
    const static StageNodeType node_type = STAGE_NODE_TYPE_LIGHT;
    typedef LightParams params_type;
};

template<>
struct stage_node_traits<DirectionalLight> {
    const static StageNodeType node_type = STAGE_NODE_TYPE_DIRECTIONAL_LIGHT;
    typedef DirectionalLightParams params_type;
};

template<>
struct stage_node_traits<PointLight> {
    const static StageNodeType node_type = STAGE_NODE_TYPE_POINT_LIGHT;
    typedef PointLightParams params_type;
};

}
#endif
