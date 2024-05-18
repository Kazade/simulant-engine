#include "pvr_api.h"

struct {
    uint8_t* command_list;
} State;

void pvr_start(PVRCommandMode mode, void* context) {

}

void pvr_finish() {

}

void pvr_viewport(int x, int y, int width, int height) {

}

void pvr_shade_model(PVRShadeModel model) {

}

void pvr_clear_color(argb_color_t color) {

}

void pvr_clear(pvr_clear_flag_mask_t mask) {

}

void pvr_ambient_color(argb_color_t color) {

}

void pvr_material(PVRMaterialMode mode, argb_color_t color) {

}

void pvr_specular(float shininess) {

}

void pvr_color_material(pvr_material_mode_mask_t mask) {

}

void pvr_depth_mask(PVRBool value) {

}

void pvr_depth_func(PVRDepthFunc func){

}
void pvr_enable(PVRState state) {

}

void pvr_disable(PVRState state) {

}

void pvr_front_face(PVRWinding winding) {

}

void pvr_scissor(int x, int y, int width, int height) {

}

void pvr_light(PVRLight i, PVRLightType type, PVRLightComponent components,
               pvr_vec3_t* position) {

}

void pvr_light_color(PVRLight i, PVRLightComponent comp, argb_color_t color) {

}

void pvr_light_attenuation(PVRLight i, float constant, float linear,
                           float quadratic) {

}

/** Set the source vertex data for drawing */
void pvr_vertex_pointer(pvr_vertex_in_t* vertices, pvr_vec3_t* normals,
                        pvr_vec2_t* sts) {

}

void pvr_draw_arrays(PVRPrimitive prim, size_t start, size_t count) {

}

void pvr_draw_elements(PVRPrimitive prim, size_t count, const uint32_t* indices) {

}

void pvr_multi_draw_arrays(PVRPrimitive prim, size_t* starts, size_t* counts,
                           size_t draw_count) {

}

void pvr_tex_mode(PVRTexFormat format, int mipmaps, bool twiddled) {

}

void pvr_tex_image(int level, int w, int h, void* data) {

}

void pvr_tex_filter(PVRTexFilter min, PVRTexFilter mag) {

}

void pvr_clut_mode(PVRPaletteFormat format) {

}

void pvr_clut_load(int entries, void* palette) {

}
