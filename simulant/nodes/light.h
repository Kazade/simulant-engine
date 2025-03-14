/* *   Copyright (c) 2011-2017 Luke Benstead https://simulant-engine.appspot.com
 *
 *     This file is part of Simulant.
 *
 *     Simulant is free software: you can redistribute it and/or modify
 *     it under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
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

#include "../generic/identifiable.h"
#include "../generic/managed.h"
#include "../generic/manual_object.h"
#include "../types.h"

#include "simulant/utils/params.h"
#include "stage_node.h"

namespace smlt {

extern const Color DEFAULT_LIGHT_COLOR;

class Light: public ContainerNode, public ChainNameable<Light> {

public:
    typedef std::shared_ptr<Light> ptr;

    Light(Scene* owner, StageNodeType type);

    void set_type(LightType type);

    /*
     *  Direction (ab)uses the light's position.
     *  Setting the direction implicitly sets the light type to directional
     *
     *  Direction is stored reversed in the position.
     */
    Vec3 direction() const {
        return transform->position();
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
        transform->set_position(Vec3(-dir.x, -dir.y, -dir.z));
    }

    void set_diffuse(const smlt::Color& color) {
        diffuse_ = color;
    }

    void set_ambient(const smlt::Color& color) {
        ambient_ = color;
    }

    void set_specular(const smlt::Color& color) {
        specular_ = color;
    }

    LightType light_type() const {
        return type_;
    }
    const smlt::Color& ambient() const {
        return ambient_;
    }
    const smlt::Color& diffuse() const {
        return diffuse_;
    }
    const smlt::Color& specular() const {
        return specular_;
    }

    /** Returns the owner stage's global ambient value. */
    smlt::Color global_ambient() const;

    void set_attenuation(float range, float constant, float linear,
                         float quadratic);
    void set_attenuation_from_range(float range);

    float range() const {
        return range_;
    }
    float constant_attenuation() const {
        return const_attenuation_;
    }
    float linear_attenuation() const {
        return linear_attenuation_;
    }
    float quadratic_attenuation() const {
        return quadratic_attenuation_;
    }

    const AABB& aabb() const override {
        return bounds_;
    }

protected:
    bool on_create(Params params) override;

private:
    LightType type_;

    smlt::Color ambient_;
    smlt::Color diffuse_;
    smlt::Color specular_;

    AABB bounds_;
    float range_;
    float const_attenuation_;
    float linear_attenuation_;
    float quadratic_attenuation_;
};

class PointLight: public Light {
public:
    S_DEFINE_STAGE_NODE_META(STAGE_NODE_TYPE_POINT_LIGHT, "point_light");
    S_DEFINE_STAGE_NODE_PARAM(PointLight, "position", FloatArray, Vec3(),
                              "The position of the light");

    PointLight(Scene* owner) :
        Light(owner, STAGE_NODE_TYPE_POINT_LIGHT) {}

private:
    bool on_create(Params params) override {
        if(!clean_params<PointLight>(params)) {
            return false;
        }

        if(!Light::on_create(params)) {
            return false;
        }

        set_type(LIGHT_TYPE_POINT);
        transform->set_position(
            params.get<FloatArray>("position").value_or(Vec3()));
        return true;
    }
};

class DirectionalLight: public Light {
public:
    S_DEFINE_STAGE_NODE_META(STAGE_NODE_TYPE_DIRECTIONAL_LIGHT,
                             "directional_light");
    S_DEFINE_STAGE_NODE_PARAM(DirectionalLight, "direction", FloatArray,
                              Vec3(1, -0.5, 0),
                              "The direction the light is pointing");

    DirectionalLight(Scene* owner) :
        Light(owner, STAGE_NODE_TYPE_DIRECTIONAL_LIGHT) {}

    bool on_create(Params params) override {
        if(!clean_params<DirectionalLight>(params)) {
            return false;
        }

        if(!Light::on_create(params)) {
            return false;
        }

        set_type(LIGHT_TYPE_DIRECTIONAL);
        auto direction = params.get<FloatArray>("direction");
        set_direction(direction.value_or(Vec3(1, -0.5, 0)));
        return true;
    }
};

} // namespace smlt
#endif
