#ifndef LIGHT_H_INCLUDED
#define LIGHT_H_INCLUDED

namespace kglt {

enum LightType {
    LIGHT_TYPE_DISABLED = 0
    LIGHT_TYPE_POINT,
    LIGHT_TYPE_DIRECTIONAL,
    LIGHT_TYPE_SPOT_LIGHT
};

class Light : public Object {
public:
    typedef std::tr1::shared_ptr<Mesh> ptr;
    
    Light(LightType type=LIGHT_TYPE_POINT):
        type_(type) {
        
        kmVec4Fill(&ambient_, 0.2f, 0.2f, 0.2f, 0.2f);
        kmVec4Fill(&diffuse_, 1.0f, 1.0f, 1.0f, 1.0f);
        kmVec4Fill(&specular_, 1.0f, 1.0f, 1.0f, 1.0f);
    }
    
    void set_type(LightType type);
    
    LightType type() const;
    kmVec4 ambient() const;
    kmVec4 diffuse() const;
    kmVec4 specular() const;
    
    float constant_attenuation() const;
    float linear_attenuation() const;
    float quadratic_attenuation() const;
    
private:
    LightType type_;
    
    kmVec4 ambient_;
    kmVec4 diffuse_;
    kmVec4 specular_;
    
    float const_attenuation_;
    float linear_attenuation_;
    float quadratic_attenuation_;
};

}
#endif
