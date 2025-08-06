#include "gltf_loader.h"

#include "../generic/raii.h"
#include "../time_keeper.h"
#include "../utils/base64.h"
#include "../vfs.h"

namespace smlt {
namespace loaders {

const char* GLTFLoader::node_factory_key = "node-factory";

static void approximate_light_intensity(Light* light, const Color& color,
                                        float intensity, float range,
                                        bool directional) {
    if(directional) {
        /*
         * Blender uses W/m2 for sun strength, with 1000 being direct sunlight.
         * GLTF uses lux which is W/m2 * 683
         * However, anything over 5 W/m2 seems to be brighter than the max
         * brightness of the material so we cap it out at 10.0f.
         */

        float max = 683 * 10.0f;
        float m = std::min(intensity, max) / max;
        auto c = color * m;
        c.a = color.a;
        light->set_diffuse(c);
        light->set_ambient(c);
        light->set_specular(c);
    } else {
        // FIXME: This is a very rough approximation
        range = std::sqrt(intensity) / 8;
        auto c = color;
        light->set_diffuse(c);
        light->set_ambient(c);
        light->set_specular(c);
        light->set_attenuation_from_range(range);
    }
}

static smlt::Vec3 parse_pos(JSONIterator it) {
    return smlt::Vec3(it[0]->to_float().value_or(0.0f),
                      it[1]->to_float().value_or(0.0f),
                      it[2]->to_float().value_or(0.0f));
}

static smlt::Vec3 parse_scale(JSONIterator it) {
    return smlt::Vec3(it[0]->to_float().value_or(1.0f),
                      it[1]->to_float().value_or(1.0f),
                      it[2]->to_float().value_or(1.0f));
}

static smlt::Color parse_color3(JSONIterator it) {
    return smlt::Color(it[0]->to_float().value_or(1.0f),
                       it[1]->to_float().value_or(1.0f),
                       it[2]->to_float().value_or(1.0f), 1.0f);
}

static smlt::Color parse_color4(JSONIterator it) {
    return smlt::Color(
        it[0]->to_float().value_or(1.0f), it[1]->to_float().value_or(1.0f),
        it[2]->to_float().value_or(1.0f), it[3]->to_float().value_or(1.0f));
}

static smlt::Quaternion parse_quaternion(JSONIterator it) {
    return smlt::Quaternion(
        it[0]->to_float().value_or(0.0f), it[1]->to_float().value_or(0.0f),
        it[2]->to_float().value_or(0.0f), it[3]->to_float().value_or(1.0f));
}

bool check_gltf_version(JSONIterator& js) {
    if(!js->has_key("asset") || !js["asset"]->has_key("version")) {
        S_ERROR("Invalid gltf file");
        return false;
    }

    return js["asset"]["version"]->to_str().value_or("0.0") == "2.0";
}

StageNode* default_node_factory(StageNode* parent,
                                const GLTFLoader::NodeFactoryInput& input) {
    StageNode* ret = nullptr;
    Light* light = nullptr;

    /* If someone specifies the node type in the extras, we create the node by
     * name*/
    if(input.params.contains("s_node")) {
        auto node_name = input.params.get<std::string>("s_node");
        if(node_name) {
            auto name = node_name.value();
            ret =
                parent->scene->create_node(name.c_str(), input.params, nullptr);

            if(!ret) {
                S_WARN("Unable to spawn node with name: {0}", name);
            } else {
                ret->set_parent(parent);
            }
        }
    }

    // If we don't have a node, fallback to the default behaviour
    // of spawning actors + stages
    if(!ret) {
        if(input.mesh) {
            S_VERBOSE("Spawning actor");
            ret = parent->create_child<Actor>(input.mesh);
        } else if(input.camera) {
            Camera* cam = parent->create_child<Camera>();
            if(input.camera->type == "perspective") {
                smlt::Mat4 proj;
                float a = input.camera->aspect;
                float y = input.camera->yfov;
                float n = input.camera->znear;
                if(input.camera->zfar) {
                    float f = input.camera->zfar.value();
                    proj[0] = 1.0f / (a * std::tan(0.5f * y));
                    proj[5] = 1.0f / std::tan(0.5f * y);
                    proj[10] = (n + f) / (n - f);
                    proj[11] = -1.0f;
                    proj[14] = (2.0f * n * f) / (n - f);
                    proj[15] = 0.0f;
                } else {
                    proj[0] = 1.0f / (a * std::tan(0.5f * y));
                    proj[5] = 1.0f / std::tan(0.5f * y);
                    proj[10] = -1;
                    proj[11] = -1;
                    proj[14] = -2.0f * n;
                    proj[15] = 0.0f;
                }

                cam->set_projection_matrix(proj);
            } else {
                S_ERROR("FIXME: Orthocam");
            }
            ret = cam;
        } else if(input.light) {
            if(input.light->type == "directional") {
                light = parent->create_child<DirectionalLight>();
            } else {
                light = parent->create_child<PointLight>();
            }

            approximate_light_intensity(
                light, input.light->color, input.light->intensity,
                input.light->range, input.light->type == "directional");
            ret = light;

        } else {
            ret = parent->create_child<Stage>();
        }
    }

    if(ret) {
        ret->transform->set_translation(input.translation);
        ret->transform->set_rotation(input.rotation);
        ret->transform->set_scale_factor(input.scale);

        if(light && light->light_type() == LIGHT_TYPE_DIRECTIONAL) {
            light->set_direction(ret->transform->orientation().forward());
        }

        if(input.params.contains("s_mixins")) {
            auto mixins_maybe = input.params.get<std::string>("s_mixins");
            if(mixins_maybe) {
                auto mixins = smlt::split(mixins_maybe.value(), ",");
                for(std::size_t i = 0; i < mixins.size(); ++i) {
                    auto mixin_name = strip(mixins[i]);
                    Params mixin_params;
                    std::string prefix = "s_mixin." + std::to_string(i) + ".";
                    for(auto arg_name: input.params.arg_names()) {
                        if(arg_name.str().find(prefix) == 0) {
                            auto key = arg_name.str().substr(prefix.size());
                            mixin_params.set(
                                key.c_str(),
                                input.params.raw(arg_name.c_str()).value());
                        }
                    }

                    if(input.mesh) {
                        mixin_params.set("mesh", input.mesh);
                    }

                    // FIXME: These inputs should NOT be the relative
                    // positions, but should instead be absolute. If you
                    // have a parent node this may not be correct.
                    mixin_params.set("scale", input.scale);
                    mixin_params.set("position", input.translation);
                    mixin_params.set("orientation", input.rotation);

                    auto new_mixin =
                        ret->create_mixin(mixin_name, mixin_params);
                    if(!new_mixin) {
                        S_ERROR("Failed to create mixin with name {0}",
                                mixin_name);
                    }
                }
            }
        }
    }

    return ret;
}

GLTFLoader::NodeFactory determine_node_factory(const LoaderOptions& options) {
    if(options.count(GLTFLoader::node_factory_key)) {
        try {
            return any_cast<GLTFLoader::NodeFactory>(
                options.at(GLTFLoader::node_factory_key));
        } catch(bad_any_cast& e) {
            S_WARN("Failed to cast node factory, using default");
        }
    }

    return default_node_factory;
}

optional<JSONIterator> find_scene(JSONIterator& js) {
    auto id = js["scene"]->to_int().value_or(0);
    return js["scenes"][id];
}

const auto TRIANGLES = 4;
const auto TRIANGLE_STRIP = 5;

enum ComponentType {
    INVALID = 0,
    BYTE = 5120,
    UNSIGNED_BYTE = 5121,
    SHORT = 5122,
    UNSIGNED_SHORT = 5123,
    UNSIGNED_INT = 5125,
    FLOAT = 5126
};

std::size_t component_size(ComponentType type) {
    switch(type) {
        case BYTE:
        case UNSIGNED_BYTE:
            return 1;
        case SHORT:
        case UNSIGNED_SHORT:
            return 2;
        case UNSIGNED_INT:
        case FLOAT:
            return 4;
        default:
            return 0;
    }
}

std::size_t component_count(const std::string& type) {
    const std::map<std::string, std::size_t> lookup = {
        {"SCALAR", 1},
        {"VEC2",   2},
        {"VEC3",   3},
        {"VEC4",   4}
    };

    return lookup.at(type);
}

struct BufferInfo {
    uint8_t* data = nullptr;
    std::size_t size = 0;
    std::size_t stride = 0;
    ComponentType c_type = INVALID;
    std::size_t c_stride = 0;
};

struct TypeKey {
    ComponentType c_type = INVALID;
    std::string a_type = "";

    TypeKey(ComponentType c_type, const std::string& a_type) :
        c_type(c_type), a_type(a_type) {}

    bool operator<(const TypeKey& rhs) const {
        if(c_type < rhs.c_type) {
            return true;
        } else if(c_type > rhs.c_type) {
            return false;
        } else {
            return a_type < rhs.a_type;
        }
    }
};

struct Accessor {
    std::string type;
    ComponentType component_type;
    int buffer_view_id;
};

static auto process_buffer(JSONIterator& js, const Accessor& accessor,
                           std::vector<std::vector<uint8_t>>& buffers)
    -> BufferInfo {

    auto accessor_component_type = accessor.component_type;
    auto type = accessor.type;
    auto buffer_view_id = accessor.buffer_view_id;

    assert(buffer_view_id >= 0);
    auto buffer_view = js["bufferViews"][buffer_view_id];
    auto buffer_id = buffer_view["buffer"]->to_int().value_or(-1);
    assert(buffer_id >= 0);

    auto buffer = js["buffers"][buffer_id];
    auto uri = buffer["uri"]->to_str().value_or("");
    if(uri.empty()) {
        S_ERROR("Buffer has no uri");
        return BufferInfo();
    }

    if((int)buffers.size() <= buffer_id) {

        const char* b64_marker = "data:application/octet-stream;base64,";
        if(uri.find(b64_marker) == 0) {
            auto data = uri.substr(strlen(b64_marker));
            auto decoded = smlt::base64_decode(data);
            if(!decoded) {
                S_ERROR("Failed to decode base64 data");
                return BufferInfo();
            }

            buffers.push_back(
                std::vector<uint8_t>(decoded->begin(), decoded->end()));
        } else {
            auto istream = smlt::get_app()->vfs->read_file(uri);
            auto data =
                std::vector<uint8_t>(std::istreambuf_iterator<char>(*istream),
                                     std::istreambuf_iterator<char>());
            buffers.push_back(data);
        }
    }

    uint8_t* buffer_data = &buffers[buffer_id][0];
    auto byte_offset = buffer_view["byteOffset"]->to_int().value_or(0);
    auto byte_length = buffer_view["byteLength"]->to_int().value_or(0);
    auto byte_stride = buffer_view["byteStride"]->to_int().value_or(0);

    auto c_stride = component_size(accessor_component_type);
    auto c_count = component_count(type);
    if(byte_stride == 0) {
        byte_stride = c_stride * c_count;
    }

    return BufferInfo{
        buffer_data + byte_offset,
        (std::size_t)byte_length,
        (std::size_t)byte_stride,
        accessor_component_type,
        c_stride,
    };
}

void process_positions(const BufferInfo& buffer_info, JSONIterator&,
                       smlt::MeshPtr& final_mesh, VertexSpecification& spec) {

    auto start = final_mesh->vertex_data->cursor_position();

#ifndef NDEBUG
    auto t0 = smlt::get_app()->time_keeper->now_in_us();
#endif

    if(spec.position_attribute == VERTEX_ATTRIBUTE_2F) {
        uint8_t* src = buffer_info.data;
        for(std::size_t i = 0; i < buffer_info.size; i += buffer_info.stride) {
            auto x = *(float*)src;
            auto y = *(float*)(src + 4);
            final_mesh->vertex_data->position(x, y);
            src += buffer_info.stride;
            final_mesh->vertex_data->move_next();
        }
    } else if(spec.position_attribute == VERTEX_ATTRIBUTE_3F) {
        uint8_t* src = buffer_info.data;
        for(std::size_t i = 0; i < buffer_info.size; i += buffer_info.stride) {
            auto x = *(float*)src;
            auto y = *(float*)(src + 4);
            auto z = *(float*)(src + 8);
            final_mesh->vertex_data->position(x, y, z);
            src += buffer_info.stride;
            final_mesh->vertex_data->move_next();
        }
    } else if(spec.position_attribute == VERTEX_ATTRIBUTE_4F) {
        uint8_t* src = buffer_info.data;
        for(std::size_t i = 0; i < buffer_info.size; i += buffer_info.stride) {
            auto x = *(float*)src;
            auto y = *(float*)(src + 4);
            auto z = *(float*)(src + 8);
            auto w = *(float*)(src + 12);
            final_mesh->vertex_data->position(x, y, z, w);
            src += buffer_info.stride;
            final_mesh->vertex_data->move_next();
        }
    } else {
        S_ERROR("Unsupported position attribute type");
    }

#ifndef NDEBUG
    auto t1 = smlt::get_app()->time_keeper->now_in_us();

    S_DEBUG("Position loading took {0}us", t1 - t0);
#endif

    final_mesh->vertex_data->move_to(start);
}

void process_colors(const BufferInfo& buffer_info, JSONIterator& js,
                    smlt::MeshPtr& final_mesh, VertexAttribute attr) {

    _S_UNUSED(js);

#ifndef NDEBUG
    auto t0 = smlt::get_app()->time_keeper->now_in_us();
#endif

    auto start = final_mesh->vertex_data->cursor_position();

    for(std::size_t i = 0; i < buffer_info.size; i += buffer_info.stride) {
        if(attr == VERTEX_ATTRIBUTE_3F) {
            auto x = *(float*)(buffer_info.data + i);
            auto y = *(float*)(buffer_info.data + i + 4);
            auto z = *(float*)(buffer_info.data + i + 8);
            final_mesh->vertex_data->base_color(smlt::Color(x, y, z, 1));
        } else if(attr == VERTEX_ATTRIBUTE_4F) {
            auto x = *(float*)(buffer_info.data + i);
            auto y = *(float*)(buffer_info.data + i + 4);
            auto z = *(float*)(buffer_info.data + i + 8);
            auto w = *(float*)(buffer_info.data + i + 12);
            final_mesh->vertex_data->base_color(smlt::Color(x, y, z, w));
        } else if(attr == VERTEX_ATTRIBUTE_4UB) {
            auto r = *(uint8_t*)(buffer_info.data + i);
            auto g = *(uint8_t*)(buffer_info.data + i + 1);
            auto b = *(uint8_t*)(buffer_info.data + i + 2);
            auto a = *(uint8_t*)(buffer_info.data + i + 3);
            final_mesh->vertex_data->base_color(
                smlt::Color::from_bytes(r, g, b, a));
        } else if(attr == VERTEX_ATTRIBUTE_4US) {
            auto r = *(uint16_t*)(buffer_info.data + i);
            auto g = *(uint16_t*)(buffer_info.data + i + 2);
            auto b = *(uint16_t*)(buffer_info.data + i + 4);
            auto a = *(uint16_t*)(buffer_info.data + i + 6);

            final_mesh->vertex_data->base_color(
                smlt::Color::from_bytes(r >> 8, g >> 8, b >> 8, a >> 8));
        } else {
            S_ERROR("Unsupported color attribute type");
        }

        final_mesh->vertex_data->move_next();
    }

#ifndef NDEBUG
    auto t1 = smlt::get_app()->time_keeper->now_in_us();

    S_DEBUG("Color loading took {0}us", t1 - t0);
#endif

    final_mesh->vertex_data->move_to(start);
}

void process_normals(const BufferInfo& buffer_info, JSONIterator& js,
                     smlt::MeshPtr& final_mesh, VertexSpecification& spec) {

    _S_UNUSED(js);

    auto start = final_mesh->vertex_data->cursor_position();

#ifndef NDEBUG
    auto t0 = smlt::get_app()->time_keeper->now_in_us();
#endif

    for(std::size_t i = 0; i < buffer_info.size; i += buffer_info.stride) {
        if(spec.normal_attribute == VERTEX_ATTRIBUTE_3F) {
            auto x = *(float*)(buffer_info.data + i);
            auto y = *(float*)(buffer_info.data + i + 4);
            auto z = *(float*)(buffer_info.data + i + 8);
            final_mesh->vertex_data->normal(x, y, z);
        } else if(spec.normal_attribute == VERTEX_ATTRIBUTE_4F) {
            auto x = *(float*)(buffer_info.data + i);
            auto y = *(float*)(buffer_info.data + i + 4);
            auto z = *(float*)(buffer_info.data + i + 8);
            final_mesh->vertex_data->normal(x, y, z);
        } else {
            S_ERROR("Unsupported normal attribute type");
        }

        final_mesh->vertex_data->move_next();
    }

#ifndef NDEBUG
    auto t1 = smlt::get_app()->time_keeper->now_in_us();

    S_DEBUG("Normal loading took {0}us", t1 - t0);
#endif

    final_mesh->vertex_data->move_to(start);
}

void process_texcoord0s(const BufferInfo& buffer_info, JSONIterator& js,
                        smlt::MeshPtr& final_mesh, VertexSpecification& spec) {

    _S_UNUSED(js);
    auto start = final_mesh->vertex_data->cursor_position();

#ifndef NDEBUG
    auto t0 = smlt::get_app()->time_keeper->now_in_us();
#endif

    for(std::size_t i = 0; i < buffer_info.size; i += buffer_info.stride) {
        if(spec.texcoord0_attribute == VERTEX_ATTRIBUTE_2F) {
            auto x = *(float*)(buffer_info.data + i);
            auto y = *(float*)(buffer_info.data + i + 4);
            final_mesh->vertex_data->tex_coord0(x, -y);
        } else if(spec.texcoord0_attribute == VERTEX_ATTRIBUTE_3F) {
            auto x = *(float*)(buffer_info.data + i);
            auto y = *(float*)(buffer_info.data + i + 4);
            auto z = *(float*)(buffer_info.data + i + 8);
            final_mesh->vertex_data->tex_coord0(x, -y, z);
        } else if(spec.texcoord0_attribute == VERTEX_ATTRIBUTE_4F) {
            auto x = *(float*)(buffer_info.data + i);
            auto y = *(float*)(buffer_info.data + i + 4);
            auto z = *(float*)(buffer_info.data + i + 8);
            auto w = *(float*)(buffer_info.data + i + 12);
            final_mesh->vertex_data->tex_coord0(x, -y, z, w);
        } else {
            S_ERROR("Unsupported texcoord0 attribute type");
        }

        final_mesh->vertex_data->move_next();
    }

#ifndef NDEBUG
    auto t1 = smlt::get_app()->time_keeper->now_in_us();

    S_DEBUG("Texcoord loading took {0}us", t1 - t0);
#endif

    final_mesh->vertex_data->move_to(start);
}

enum Filter {
    NEAREST = 9728,
    LINEAR = 9729,
    NEAREST_MIPMAP_NEAREST = 9984,
    LINEAR_MIPMAP_NEAREST = 9985,
    NEAREST_MIPMAP_LINEAR = 9986,
    LINEAR_MIPMAP_LINEAR = 9987
};

TextureFilter calculate_filter(int magFilter, int minFilter) {
    if(magFilter == LINEAR && minFilter == LINEAR) {
        return smlt::TEXTURE_FILTER_BILINEAR;
    } else if(magFilter == LINEAR && minFilter == LINEAR_MIPMAP_LINEAR) {
        return smlt::TEXTURE_FILTER_TRILINEAR;
    } else {
        return smlt::TEXTURE_FILTER_POINT;
    }
}

static smlt::TexturePtr load_texture(StageNode* node, JSONIterator& js,
                                     JSONIterator& texture, int texture_id,
                                     const std::string& ext = "") {

    _S_UNUSED(texture_id);

    auto& scene = node->scene;

    int sampler_id = texture["sampler"]->to_int().value_or(-1);
    int source_id = texture["source"]->to_int().value_or(-1);
    if(source_id < 0 || sampler_id < 0) {
        return smlt::TexturePtr();
    }

    auto sampler = js["samplers"][sampler_id];
    auto image = js["images"][source_id];
    if(!sampler.is_valid() || !image.is_valid()) {
        return smlt::TexturePtr();
    }

    smlt::Path uri = image["uri"]->to_str().value_or("");
    if(!uri.str().empty()) {
        if(!ext.empty()) {
            uri = uri.replace_ext(ext);
        }
        auto tex = scene->assets->load_texture(uri);

        auto wrapS = sampler["wrapS"]->to_int().value_or(10497);
        auto wrapT = sampler["wrapT"]->to_int().value_or(10497);
        auto magFilter = sampler["magFilter"]->to_int().value_or(9729);
        auto minFilter = sampler["minFilter"]->to_int().value_or(9987);

        TextureWrap u = TEXTURE_WRAP_REPEAT, v = TEXTURE_WRAP_REPEAT;

        switch(wrapS) {
            case 33071:
                u = TEXTURE_WRAP_CLAMP_TO_EDGE;
                break;
            case 33648:
                u = TEXTURE_WRAP_MIRRORED_REPEAT;
                break;
            default:
                break;
        }

        switch(wrapT) {
            case 33071:
                v = TEXTURE_WRAP_CLAMP_TO_EDGE;
                break;
            case 33648:
                v = TEXTURE_WRAP_MIRRORED_REPEAT;
                break;
            default:
                break;
        }

        tex->set_texture_filter(calculate_filter(magFilter, minFilter));
        tex->set_texture_wrap(u, v, TEXTURE_WRAP_REPEAT);
        return tex;
    } else {
        S_ERROR("Only uri textures supported atm");
    }

    return smlt::TexturePtr();
}

static smlt::MaterialPtr create_default_material(StageNode* node) {
    auto& scene = node->scene;
    auto mat = scene->assets->clone_default_material();
    mat->set_name("Default");
    mat->set_lighting_enabled(true);
    mat->set_textures_enabled(0);
    mat->set_cull_mode(smlt::CULL_MODE_BACK_FACE);
    return mat;
}

smlt::MaterialPtr load_material(StageNode* node, JSONIterator& js,
                                JSONIterator& material, int material_id,
                                const std::vector<smlt::TexturePtr>& textures) {

    _S_UNUSED(material_id);
    _S_UNUSED(js);

    auto& scene = node->scene;

    auto base_texture_id =
        material["pbrMetallicRoughness"]["baseColorTexture"]["index"]
            ->to_int()
            .value_or(-1);

    auto metallic_roughness_texture_id =
        material["pbrMetallicRoughness"]["metallicRoughnessTexture"]["index"]
            ->to_int()
            .value_or(-1);

    auto metallic =
        material["pbrMetallicRoughness"]["metallicFactor"]->to_float().value_or(
            1.0f);

    auto roughness = material["pbrMetallicRoughness"]["roughnessFactor"]
                         ->to_float()
                         .value_or(1.0f);

    auto emissive = Color(0, 0, 0, 1);
    if(material["emissiveFactor"].is_valid()) {
        emissive = parse_color3(material["emissiveFactor"]);
    }

    auto normal_texture_id =
        material["normalTexture"]["index"]->to_int().value_or(-1);

    auto occ_texture_id =
        material["occlusionTexture"]["index"]->to_int().value_or(-1);

    smlt::EnabledTextureMask enabled = 0;
    smlt::MaterialPtr ret = scene->assets->clone_default_material();
    if(base_texture_id >= 0) {
        ret->set_base_color_map(textures[base_texture_id]);
        enabled |= BASE_COLOR_MAP_ENABLED;
    }

    if(metallic_roughness_texture_id >= 0) {
        ret->set_metallic_roughness_map(
            textures[metallic_roughness_texture_id]);
        enabled |= METALLIC_ROUGHNESS_MAP_ENABLED;
    }

    if(normal_texture_id >= 0) {
        ret->set_normal_map(textures[normal_texture_id]);
        enabled |= NORMAL_MAP_ENABLED;
    }

    if(occ_texture_id >= 0) {
        ret->set_light_map(textures[occ_texture_id]);
        enabled |= LIGHT_MAP_ENABLED;
    }

    auto base_color = material["pbrMetallicRoughness"]["baseColorFactor"];
    auto color = smlt::Color::white();
    if(base_color.is_valid()) {
        color = parse_color4(base_color);
    }

    auto double_sided = material["doubleSided"]->to_bool().value_or(false);
    if(double_sided) {
        ret->set_cull_mode(smlt::CULL_MODE_NONE);
    } else {
        ret->set_cull_mode(smlt::CULL_MODE_BACK_FACE);
    }
    ret->set_name(material["name"]->to_str().value_or(""));
    ret->set_textures_enabled(enabled);
    ret->set_lighting_enabled(true);

    auto alpha_mode = material["alphaMode"].is_valid()
                          ? material["alphaMode"]->to_str().value_or("OPAQUE")
                          : "OPAQUE";

    // FIXME: If the value is "MASK" we should enable alpha testing
    ret->pass(0)->set_blend_func((alpha_mode == "OPAQUE") ? smlt::BLEND_NONE
                                                          : smlt::BLEND_ALPHA);

    ret->set_metallic(metallic);
    ret->set_roughness(roughness);
    ret->set_base_color(color);

    return ret;
}

smlt::MeshPtr load_mesh(StageNode* node, JSONIterator& js, JSONIterator& mesh,
                        int mesh_id,
                        const std::vector<smlt::MaterialPtr>& materials,
                        std::vector<std::vector<uint8_t>>& buffers) {
    _S_UNUSED(mesh_id);

    auto& scene = node->scene;

    struct MeshPrimitive {
        MeshPrimitive(const smlt::JSONIterator& attrs) :
            attrs(attrs) {}
        const smlt::JSONIterator& attrs;
        int material_id = -1;
        int indexes_id = -1;

        int position_id = -1;
        int normal_id = -1;
        int color_id = -1;
        int texcoord_id = -1;
    };

    std::vector<MeshPrimitive> primitives;
    std::vector<Accessor> accessors;

    /* This is the most complicated part of the loader. A GLTF file has a
       heirarchy of: mesh -> accessor -> bufferView -> buffer */
    for(auto& acc_node: js["accessors"]) {
        auto acc = acc_node.to_iterator();
        Accessor accessor;
        accessor.type = acc["type"]->to_str().value_or("SCALAR");
        accessor.component_type =
            (ComponentType)acc["componentType"]->to_int().value_or(5121);
        accessor.buffer_view_id = acc["bufferView"]->to_int().value_or(-1);
        accessors.push_back(accessor);
    }

    const std::map<TypeKey, VertexAttribute> lookup = {
        {TypeKey(FLOAT,          "VEC2"), VERTEX_ATTRIBUTE_2F },
        {TypeKey(FLOAT,          "VEC3"), VERTEX_ATTRIBUTE_3F },
        {TypeKey(FLOAT,          "VEC4"), VERTEX_ATTRIBUTE_4F },
        // {TypeKey(FLOAT, "SCALAR"), VERTEX_ATTRIBUTE_1F},
        {TypeKey(UNSIGNED_BYTE,  "VEC4"), VERTEX_ATTRIBUTE_4UB},
        {TypeKey(UNSIGNED_SHORT, "VEC4"), VERTEX_ATTRIBUTE_4US},
    };

    auto process_attribute = [&](int acc_id) -> VertexAttribute {
        if(acc_id == -1) {
            return smlt::VERTEX_ATTRIBUTE_NONE;
        }

        auto acc_node = accessors[acc_id];
        auto key = TypeKey(acc_node.component_type, acc_node.type);
        if(lookup.count(key)) {
            return lookup.at(key);
        }

        return smlt::VERTEX_ATTRIBUTE_NONE;
    };

    for(auto& primitive_node: mesh["primitives"]) {
        auto primitive = primitive_node.to_iterator();

        MeshPrimitive mp(primitive["attributes"]);
        mp.material_id = primitive["material"]->to_int().value_or(-1);
        mp.position_id =
            primitive["attributes"]["POSITION"]->to_int().value_or(-1);
        mp.normal_id = primitive["attributes"]["NORMAL"]->to_int().value_or(-1);
        mp.color_id = primitive["attributes"]["COLOR_0"]->to_int().value_or(-1);
        mp.texcoord_id =
            primitive["attributes"]["TEXCOORD_0"]->to_int().value_or(-1);
        mp.indexes_id = primitive["indices"]->to_int().value_or(-1);

        primitives.push_back(mp);
    }

    smlt::MeshPtr final_mesh;
    int i = 0;
    for(auto& primitive: primitives) {
        auto pos = process_attribute(primitive.position_id);
        auto norm = process_attribute(primitive.normal_id);
        auto diff = process_attribute(primitive.color_id);
        auto tex = process_attribute(primitive.texcoord_id);

        auto clean_diffuse = [](VertexAttribute attr) -> VertexAttribute {
            if(attr == VERTEX_ATTRIBUTE_4US) {
                return VERTEX_ATTRIBUTE_4UB;
            }

            return attr;
        };

        auto spec = VertexSpecification(
            pos, norm, tex, VERTEX_ATTRIBUTE_NONE, VERTEX_ATTRIBUTE_NONE,
            VERTEX_ATTRIBUTE_NONE, VERTEX_ATTRIBUTE_NONE, VERTEX_ATTRIBUTE_NONE,
            VERTEX_ATTRIBUTE_NONE, VERTEX_ATTRIBUTE_NONE, clean_diffuse(diff));

        if(!final_mesh) {
            final_mesh = scene->assets->create_mesh(spec);
        } else if(final_mesh->vertex_data->vertex_specification() != spec) {
            S_ERROR("GLTF mesh contains multiple vertex types which is "
                    "currently unsupported");
        }

        auto mode = mesh["mode"]->to_int().value_or(TRIANGLES);
        if(mode != TRIANGLES && mode != TRIANGLE_STRIP) {
            S_ERROR("Mesh with unsupported mode: {0}", mode);
            continue;
        }

        auto sm_name = _F("Primitives: {0}").format(i++);
        auto material_id = primitive.material_id;
        auto material =
            (material_id >= 0) ? materials[material_id] : materials.back();

        /* FIXME: Maybe we can reuse submeshes? */
        auto sm = final_mesh->create_submesh(
            sm_name, material,
            // FIXME: Index type should be derived from the min/max index
            INDEX_TYPE_16_BIT,
            mode == TRIANGLES ? MESH_ARRANGEMENT_TRIANGLES
                              : MESH_ARRANGEMENT_TRIANGLE_STRIP);

        int offset = final_mesh->vertex_data->count();
        final_mesh->vertex_data->move_to(offset);

        if(primitive.position_id >= 0) {
            auto position = accessors[primitive.position_id];
            auto buffer_info = process_buffer(js, position, buffers);
            process_positions(buffer_info, js, final_mesh, spec);
        }

        if(primitive.normal_id >= 0) {
            auto normal = accessors[primitive.normal_id];
            auto buffer_info = process_buffer(js, normal, buffers);
            process_normals(buffer_info, js, final_mesh, spec);
        }

        if(primitive.color_id >= 0) {
            auto color = accessors[primitive.color_id];
            auto buffer_info = process_buffer(js, color, buffers);
            process_colors(buffer_info, js, final_mesh, diff);
        }

        if(primitive.texcoord_id >= 0) {
            auto texcoord0 = accessors[primitive.texcoord_id];
            auto buffer_info = process_buffer(js, texcoord0, buffers);
            process_texcoord0s(buffer_info, js, final_mesh, spec);
        }

        auto indices_id = primitive.indexes_id;
        if(indices_id >= 0) {
            auto indices = accessors[indices_id];
            auto buffer_info = process_buffer(js, indices, buffers);
            S_VERBOSE("Populating indices");
            for(std::size_t i = 0; i < buffer_info.size;
                i += buffer_info.stride) {
                switch(buffer_info.c_type) {
                    case BYTE:
                    case UNSIGNED_BYTE: {
                        auto u8idx = *(uint8_t*)(buffer_info.data + i);
                        sm->index_data->index(u8idx + offset);
                    } break;
                    case SHORT:
                    case UNSIGNED_SHORT: {
                        auto u16idx = *(uint16_t*)(buffer_info.data + i);
                        sm->index_data->index(u16idx + offset);
                    } break;
                    default:
                        S_ERROR("Unsupported index type: {0}",
                                buffer_info.c_type);
                        break;
                }
            }

            sm->index_data->done();
        }
    }

    final_mesh->vertex_data->done();
    return final_mesh;
}

StageNode* spawn_node_recursively(StageNode* parent, int node_id,
                                  JSONIterator& js,
                                  GLTFLoader::NodeFactory factory,
                                  const std::vector<smlt::MeshPtr>& meshes) {
    auto nodes = js["nodes"];
    auto node = nodes[node_id];
    smlt::MeshPtr mesh;
    if(node->has_key("mesh") && !meshes.empty()) {
        mesh = meshes[node["mesh"]->to_int().value_or(0)];
    }

    smlt::optional<GLTFLoader::CameraInfo> cam;
    if(node->has_key("camera")) {
        auto camera_id = node["camera"]->to_int().value_or(-1);
        if(camera_id >= 0) {
            GLTFLoader::CameraInfo c;
            auto cam_node = js["cameras"][camera_id];
            auto persp_node = cam_node["perspective"];
            c.type = cam_node["type"]->to_str().value_or("perspective");
            if(c.type == "perspective") {
                c.aspect =
                    persp_node["aspectRatio"]->to_float().value_or(1.777f);
                c.yfov = persp_node["yfov"]->to_float().value_or(60.0f);
                c.znear = persp_node["znear"]->to_float().value_or(1.0f);
                if(persp_node["zfar"].is_valid()) {
                    c.zfar = persp_node["zfar"]->to_float().value_or(1000.0f);
                }
            }

            cam = c;
        }
    }

    GLTFLoader::NodeFactoryInput input;

    if(node["extras"].is_valid()) {
        for(auto& k: node["extras"]->keys()) {
            auto it = node["extras"];
            if(it[k]->is_str()) {
                input.params.set(k.c_str(), it[k]->to_str().value_or(""));
            } else if(it[k]->is_number()) {
                if(it[k]->is_float()) {
                    input.params.set(k.c_str(),
                                     it[k]->to_float().value_or(0.0f));
                } else {
                    input.params.set(k.c_str(),
                                     (int)it[k]->to_int().value_or(0));
                }

            } else if(it[k]->is_bool()) {
                input.params.set(k.c_str(), it[k]->to_bool().value_or(false));
            } else if(it[k]->is_array()) {
                std::vector<float> farr;
                std::vector<int> iarr;
                std::vector<bool> barr;

                for(auto& v: it[k]) {
                    if(v.is_bool()) {
                        barr.push_back(v.to_bool().value_or(false));
                    } else if(v.is_number()) {
                        if(v.is_float()) {
                            farr.push_back(v.to_float().value_or(0.0f));
                        } else {
                            iarr.push_back(v.to_int().value_or(0));
                        }
                    }
                }

                if(farr.size()) {
                    input.params.set(k.c_str(), farr);
                } else if(iarr.size()) {
                    input.params.set(k.c_str(), iarr);
                } else if(barr.size()) {
                    input.params.set(k.c_str(), barr);
                }
            }
        }
    }

    auto light_id =
        node["extensions"]["KHR_lights_punctual"]["light"]->to_int().value_or(
            -1);

    if(light_id >= 0) {
        auto light =
            js["extensions"]["KHR_lights_punctual"]["lights"][light_id];
        if(light.is_valid()) {
            GLTFLoader::LightInfo info;
            info.color = parse_color3(light["color"]);
            info.intensity = light["intensity"]->to_float().value_or(1.0f);
            info.range = light["range"]->to_float().value_or(100.0f);
            info.type = light["type"]->to_str().value_or("directional");
            input.light = info;
        }
    }

    input.name = node["name"]->to_str().value_or("");
    input.mesh = mesh;

    if(mesh) {
        // Auto populate the mesh in the params
        input.params.set("mesh", mesh);
    }

    input.camera = cam;
    input.scale = (cam) ? smlt::Vec3(1) : parse_scale(node["scale"]);
    input.translation = parse_pos(node["translation"]);
    input.rotation = parse_quaternion(node["rotation"]);
    // FIXME: Additional properties!

    // FIXME: These should be absolute values, not relative!
    input.params.set("scale", input.scale);
    input.params.set("position", input.translation);
    input.params.set("orientation", input.rotation);

    StageNode* new_node = factory(parent, input);
    if(new_node) {
        new_node->set_name(input.name.str());

        if(node->has_key("children")) {
            for(auto& child: node["children"]) {
                spawn_node_recursively(new_node, child.to_int().value_or(0), js,
                                       factory, meshes);
            }
        }
    }

    return new_node;
}

void GLTFLoader::into(Loadable& resource, const LoaderOptions& options) {
    auto scene = loadable_to<StageNode>(resource);
    auto js = json_read(this->data_);

    if(!check_gltf_version(js)) {
        return;
    }

    auto node_factory = determine_node_factory(options);
    auto maybe_scene_it = find_scene(js);
    if(!maybe_scene_it) {
        S_ERROR("No scene in gltf file");
        return;
    }

    std::vector<TexturePtr> textures;
    std::vector<MaterialPtr> materials;
    std::vector<MeshPtr> meshes;
    std::vector<std::vector<uint8_t>> buffers;

    /* We add the containing folder to the front of the search path
       and ensure we always remove it when we're done (if it wasn't there
       already)
    */
    auto folder = filename_.parent();
    auto added = smlt::get_app()->vfs->insert_search_path(0, folder);
    raii::Finally finally([&]() {
        if(added) {
            smlt::get_app()->vfs->remove_search_path(folder);
        }
    });

    std::string ext = "";
    if(options.count("override_texture_extension")) {
        ext = any_cast<std::string>(options.at("override_texture_extension"));
    }

    int j = 0;
    auto textures_it = js["textures"];
    for(auto& node: textures_it) {
        auto tex_it = node.to_iterator();
        auto tex = load_texture(scene, js, tex_it, j++, ext);
        textures.push_back(tex);
    }

    auto materials_it = js["materials"];
    j = 0;
    for(auto& node: materials_it) {
        auto mat_it = node.to_iterator();
        auto mat = load_material(scene, js, mat_it, j++, textures);
        materials.push_back(mat);
    }

    /* Add the default material at the end */
    auto default_material = create_default_material(scene);
    materials.push_back(default_material);

    auto meshes_it = js["meshes"];
    j = 0;
    for(auto& node: meshes_it) {
        auto mesh_it = node.to_iterator();
        auto mesh = load_mesh(scene, js, mesh_it, j++, materials, buffers);
        meshes.push_back(mesh);
    }

    if(options.count("root_name")) {
        auto nodes_it = js["nodes"];
        auto root_name = smlt::any_cast<std::string>(options.at("root_name"));
        int i = -1;
        for(auto& node: nodes_it) {
            ++i;

            auto it = node.to_iterator();
            if(it->has_key("name")) {
                auto name_maybe = it["name"]->to_str();
                if(name_maybe && name_maybe.value() == root_name) {
                    spawn_node_recursively(scene, i, js, node_factory, meshes);
                    return;
                }
            }
        }

        S_ERROR("Unable to locate specified root_name: {0}", root_name);
        // FIXME: convey error!
        return;
    }

    auto scene_it = maybe_scene_it.value();
    for(auto& node: scene_it["nodes"]) {
        auto node_it = node.to_iterator();
        auto maybe_id = node_it->to_int();
        if(!maybe_id) {
            S_WARN("Node id was an unexpected type");
            continue;
        }

        auto node_id = maybe_id.value();
        spawn_node_recursively(scene, node_id, js, node_factory, meshes);
    }
}

} // namespace loaders
} // namespace smlt
