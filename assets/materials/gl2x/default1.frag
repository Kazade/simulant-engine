#version {0}

#ifdef GL_ES
precision mediump float;
#endif

uniform vec4 s_light_color;
uniform float s_light_intensity;
uniform float s_light_range;
uniform vec4 s_light_position;

uniform sampler2D s_base_color_map;
uniform vec4 s_global_ambient;
uniform vec4 s_material_base_color;
uniform vec4 s_material_specular_color;
uniform float s_material_specular;
uniform mat4 s_base_color_map_matrix;

varying vec2 frag_texcoord0;
varying vec3 frag_position;                       // Fragment position in world space
varying vec3 frag_normal;                         // Fragment normal in world space

void main() {
    // Normalize the normal vector
    vec3 normal = normalize(frag_normal);
    
    // Calculate light direction
    vec3 light_direction = normalize(s_light_position.xyz - frag_position);
    
    // Calculate distance to the light
    float distance = length(s_light_position.xyz - frag_position);
    
    // Calculate attenuation
    float attenuation = clamp(1.0 - (distance / s_light_range), 0.0, 1.0);
    
    // Sample the base color from the texture
    vec4 base_color = texture2D(s_base_color_map, frag_texcoord0);
    base_color *= s_material_base_color; // Combine with material base color
    
    // Ambient component
    vec4 ambient = s_global_ambient * base_color;
    
    // Diffuse component
    float diff = max(dot(normal, light_direction), 0.0);
    vec4 diffuse = s_light_color * s_light_intensity * diff * base_color * attenuation;
    
    // Specular component
    vec3 view_direction = normalize(-frag_position); // Assuming camera at origin
    vec3 reflect_direction = reflect(-light_direction, normal);
    float spec = pow(max(dot(view_direction, reflect_direction), 0.0), s_material_specular);
    vec4 specular = s_material_specular_color * s_light_color * s_light_intensity * spec * attenuation;
    
    // Final color
    gl_FragColor = ambient + diffuse + specular;
}

