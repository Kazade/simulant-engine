#version 120
uniform vec4 light_diffuse;
uniform vec4 light_specular;

uniform vec4 material_diffuse;
uniform vec4 material_specular;
uniform float material_shininess;

uniform float constant_attenuation;
uniform float linear_attenuation;
uniform float quadratic_attenuation;

varying vec4 vertex_position_eye;
varying vec4 vertex_normal_eye;
varying vec4 light_position_eye;
varying vec4 frag_diffuse;

void main() {
    vec4 light_dir = light_position_eye - (vertex_position_eye * vec4(light_position_eye.w));
    vec4 n_eye = normalize(vertex_normal_eye);
    vec4 s_eye = normalize(light_dir);
    vec4 v_eye = normalize(-vertex_position_eye);
    vec4 h_eye = normalize(v_eye + s_eye);

    vec4 color = vec4(0);

    float intensity = max(dot(s_eye, n_eye), 0.0);

    if(intensity > 0.0) {
        float d = length(light_dir);
        float attenuation = 1.0;

        if(light_position_eye.w > 0.0) {
            attenuation = 1.0 / (
                constant_attenuation +
                linear_attenuation * d +
                quadratic_attenuation * d * d
            );
        }

        color += attenuation * (light_diffuse * material_diffuse * intensity);
        float spec = max(dot(h_eye, n_eye), 0.0);
        color += attenuation * (light_specular * material_specular) * pow(spec, material_shininess) * intensity;
    }

    gl_FragColor = color * frag_diffuse;
}
