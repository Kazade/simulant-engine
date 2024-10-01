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

    LightType light_type() const {
        return type_;
    }

    void set_color(const Color& color) {
        color_ = color;
    }

    const smlt::Color& color() const {
        return color_;
    }

    void set_intensity(float i) {
        intensity_ = i;
        if(range_ == 0) {
            // FIXME: This is ugly, this should probably
            // happen in the renderer, and this is clearly
            // a bad approximation
            range_ = std::sqrt(intensity_) / 8;
        }
    }
    float intensity() const {
        return intensity_;
    }

    float range() const {
        return range_;
    }

    /**
     * @brief effective_range
     * @return if range is non-zero, then this returns range
     * otherwise it returns a value based on the intensity
     * and inverse square law
     */
    float effective_range() const {
        return std::sqrt(effective_range_squared());
    }

    float effective_range_squared() const {
        if(smlt::almost_equal(range_, 0.0f)) {
            return range_ * range_;
        } else {
            // FIXME: This value may need tweaking if it cause
            // lights to unexpectedly cut-off
            const float effectively_zero = 0.001f;

            // FIXME: The use of fast inverse sqrt here is for perf on Dreamcast
            // and should be divisions on other platforms
            const float d =
                effectively_zero * fast_inverse_sqrt(intensity_ * intensity_);
            return fast_inverse_sqrt(d * d);
        }
    }

    const AABB& aabb() const override {
        return bounds_;
    }

protected:
    bool on_create(Params params) override;

private:
    LightType type_;

    smlt::Color color_;
    float intensity_;

    AABB bounds_;
    float range_ = 0;
};

class PointLight: public Light {
public:
    S_DEFINE_STAGE_NODE_META(STAGE_NODE_TYPE_POINT_LIGHT, "point_light");
    S_DEFINE_STAGE_NODE_PARAM(PointLight, "position", FloatArray, Vec3(),
                              "The position of the light");
    S_DEFINE_STAGE_NODE_PARAM(PointLight, "color", FloatArray, Vec4(),
                              "The color of the light");
    S_DEFINE_STAGE_NODE_PARAM(PointLight, "range", float, 0.0f,
                              "The range of the light");
    S_DEFINE_STAGE_NODE_PARAM(PointLight, "intensity", FloatArray, Vec4(),
                              "The intensity of the light");

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
    S_DEFINE_STAGE_NODE_PARAM(PointLight, "color", FloatArray, Vec4(),
                              "The color of the light");
    S_DEFINE_STAGE_NODE_PARAM(PointLight, "range", float, 0.0f,
                              "The range of the light");
    S_DEFINE_STAGE_NODE_PARAM(PointLight, "intensity", FloatArray, Vec4(),
                              "The intensity of the light");

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
