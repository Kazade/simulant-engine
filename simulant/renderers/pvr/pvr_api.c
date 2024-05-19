#include "pvr_api.h"

struct {
    uint8_t* command_list;
} State;

void pvr_start(pvr_command_mode_t mode, void* context) {

}

void pvr_finish() {

}

void pvr_viewport(int x, int y, int width, int height) {

}

void pvr_shade_model(pvr_shade_model_t model) {

}

void pvr_clear_color(argb_color_t color) {

}

void pvr_clear(pvr_clear_flag_mask_t mask) {

}

void pvr_ambient_color(argb_color_t color) {

}

void pvr_material(pvr_material_mode_t mode, argb_color_t color) {

}

void pvr_specular(float shininess) {

}

void pvr_color_material(pvr_material_mode_mask_t mask) {

}

void pvr_depth_mask(pvr_bool_t value) {

}

void pvr_depth_func(pvr_depth_func_t func){

}
void pvr_enable(pvr_state_t state) {

}

void pvr_disable(pvr_state_t state) {

}

void pvr_front_face(pvr_winding_t winding) {

}

void pvr_scissor(int x, int y, int width, int height) {

}

void pvr_light(pvr_light_t i, pvr_light_type_t type, pvr_light_component_t components,
               pvr_vec3_t* position) {

}

void pvr_light_color(pvr_light_t i, pvr_light_component_t comp, argb_color_t color) {

}

void pvr_light_attenuation(pvr_light_t i, float constant, float linear,
                           float quadratic) {

}

void pvr_vertex_pointers(
    pvr_vec3_t* positions,
    pvr_vec2_t* uvs,
    argb_color_t* colors,
    argb_color_t* color_offsets,
    pvr_vec3_t* normals,
    pvr_vec2_t* sts,
    size_t stride
    ) {

}

void pvr_draw_arrays(pvr_primitive_t prim, size_t start, size_t count) {

}

void pvr_draw_elements(pvr_primitive_t prim, size_t count, const uint32_t* indices) {

}

void pvr_multi_draw_arrays(pvr_primitive_t prim, size_t* starts, size_t* counts,
                           size_t draw_count) {

}

void pvr_tex_mode(pvr_tex_format_t format, int mipmaps, pvr_bool_t twiddled) {

}

void pvr_tex_image(int level, int w, int h, void* data) {

}

void pvr_tex_filter(pvr_tex_filter_t min, pvr_tex_filter_t mag) {

}

void pvr_clut_mode(pvr_palette_format_t format) {

}

void pvr_clut_load(int entries, void* palette) {

}
