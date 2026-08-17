#include "gltf_loader.h"

#include <sstream>

#include "../asset_manager.h"
#include "../assets/prefab.h"
#include "../generic/raii.h"
#include "../math/degrees.h"
#include "../math/radians.h"
#include "../nodes/armature.h"
#include "../platform.h"
#include "../time_keeper.h"
#include "../utils/base64.h"
#include "../utils/random.h"
#include "../vfs.h"
#include "dtex_loader.h"
#include "png_loader.h"
#include "texture_loader.h"

namespace smlt {
namespace loaders {

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
    float r = it[0]->to_float().value_or(1.0f);
    float g = it[1]->to_float().value_or(1.0f);
    float b = it[2]->to_float().value_or(1.0f);
    float a = it[3]->to_float().value_or(1.0f);

    return smlt::Color(r, g, b, a);
}

static smlt::Quaternion parse_quaternion(JSONIterator it) {
    float x = it[0]->to_float().value_or(0.0f);
    float y = it[1]->to_float().value_or(0.0f);
    float z = it[2]->to_float().value_or(0.0f);
    float w = it[3]->to_float().value_or(1.0f);

    return smlt::Quaternion(x, y, z, w);
}

bool check_gltf_version(JSONIterator& js) {
    if(!js->has_key("asset") || !js["asset"]->has_key("version")) {
        S_ERROR("Invalid gltf file");
        return false;
    }

    return js["asset"]["version"]->to_str().value_or("0.0") == "2.0";
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
        {"VEC4",   4},
        {"MAT2",   4},
        {"MAT3",   9},
        {"MAT4",  16}
    };

    return lookup.at(type);
}

struct BufferInfo {
    std::vector<uint8_t> data;
    std::size_t stride = 0;
    ComponentType c_type = INVALID;
    std::size_t c_stride = 0;
    std::size_t c_count = 0;

    bool to_typed_array(std::vector<Vec3>& vecs_out) {
        if(c_type != FLOAT || c_count != 3) {
            S_ERROR("Only float conversion implemented");
            return false;
        }

        vecs_out.clear();

        uint8_t* it = &data[0];
        auto count = data.size() / stride;
        for(std::size_t i = 0; i < count; ++i) {
            float x = *reinterpret_cast<float*>(it);
            it += c_stride;
            float y = *reinterpret_cast<float*>(it);
            it += c_stride;
            float z = *reinterpret_cast<float*>(it);
            it += c_stride;

            vecs_out.push_back(Vec3(x, y, z));
        }

        return true;
    }

    bool to_typed_array(std::vector<Quaternion>& quats_out) {
        if(c_type != FLOAT || c_count != 4) {
            S_ERROR("Only float conversion implemented");
            return false;
        }

        quats_out.clear();

        uint8_t* it = &data[0];
        auto count = data.size() / stride;
        for(std::size_t i = 0; i < count; ++i) {
            float x = *reinterpret_cast<float*>(it);
            it += c_stride;
            float y = *reinterpret_cast<float*>(it);
            it += c_stride;
            float z = *reinterpret_cast<float*>(it);
            it += c_stride;
            float w = *reinterpret_cast<float*>(it);
            it += c_stride;

            quats_out.push_back(Quaternion(x, y, z, w));
        }

        return true;
    }

    bool to_typed_array(std::vector<float>& scalars_out) {
        if(c_type != FLOAT || c_count != 1) {
            S_ERROR("Only float conversion implemented");
            return false;
        }

        scalars_out.clear();

        uint8_t* it = &data[0];
        auto count = data.size() / stride;
        for(std::size_t i = 0; i < count; ++i) {
            for(std::size_t j = 0; j < c_count; ++j) {
                float* thing = reinterpret_cast<float*>(it);
                scalars_out.push_back(*thing);
                it += c_stride;
            }
        }

        return true;
    }
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
                           std::istream* bin) -> BufferInfo {

    auto accessor_component_type = accessor.component_type;
    auto type = accessor.type;
    auto buffer_view_id = accessor.buffer_view_id;

    assert(buffer_view_id >= 0);
    auto buffer_view = js["bufferViews"][buffer_view_id];
    auto buffer_id = buffer_view["buffer"]->to_int().value_or(-1);
    assert(buffer_id >= 0);

    auto buffer = js["buffers"][buffer_id];
    auto uri = buffer["uri"]->to_str().value_or("");
    if(uri.empty() && !bin) {
        S_ERROR("Buffer has no uri");
        return BufferInfo();
    }

    BufferInfo result;

    const char* b64_marker = "data:application/octet-stream;base64,";

    auto byte_offset = buffer_view["byteOffset"]->to_int().value_or(0);
    auto byte_length = buffer_view["byteLength"]->to_int().value_or(0);
    auto byte_stride = buffer_view["byteStride"]->to_int().value_or(0);

    if(uri.empty()) {
        // We need to seek and read from bin
        uint32_t g = bin->tellg();
        bin->seekg(byte_offset, std::ios::cur);
        result.data.resize(byte_length);
        bin->read((char*)&result.data[0], byte_length);
        byte_offset = 0;
        bin->seekg(g, std::ios::beg);
    } else if(uri.find(b64_marker) == 0) {
        auto data = uri.substr(strlen(b64_marker));
        auto decoded = smlt::base64_decode(data);
        if(!decoded) {
            S_ERROR("Failed to decode base64 data");
            return BufferInfo();
        }
        result.data.insert(result.data.begin(), decoded->begin() + byte_offset,
                           decoded->begin() + byte_offset + byte_length);
    } else {
        auto istream = smlt::get_app()->vfs->open_file(uri);
        istream->seekg(byte_offset);
        result.data.resize(byte_length);
        istream->read((char*)&result.data[0], byte_length);
    }
    auto c_stride = component_size(accessor_component_type);
    auto c_count = component_count(type);
    if(byte_stride == 0) {
        byte_stride = c_stride * c_count;
    }
    result.stride = byte_stride;
    result.c_type = accessor_component_type;
    result.c_stride = c_stride;
    result.c_count = c_count;

    return result;
}

void process_joints(const BufferInfo& buffer_info, JSONIterator&,
    smlt::MeshPtr& final_mesh, const VertexSpecification& spec) {
    const uint8_t* d = buffer_info.data.data();
    auto start = final_mesh->vertex_data->cursor_position();

#ifndef NDEBUG
    auto t0 = smlt::get_app()->time_keeper->now_in_us();
#endif

    if (spec.joint_attribute == VERTEX_ATTRIBUTE_4UB) {
        for(std::size_t i = 0; i < buffer_info.data.size(); i += buffer_info.stride) {
            const uint8_t* ptr = d + i;

            final_mesh->vertex_data->joints<uint8_t>(ptr[0], ptr[1], ptr[2], ptr[3]);
            final_mesh->vertex_data->move_next();
        }
    } else if (spec.joint_attribute == VERTEX_ATTRIBUTE_4US) {
        for(std::size_t i = 0; i < buffer_info.data.size(); i += buffer_info.stride) {
            const uint16_t* ptr = reinterpret_cast<const uint16_t*>(d + i);

            final_mesh->vertex_data->joints<uint16_t>(ptr[0], ptr[1], ptr[2], ptr[3]);
            final_mesh->vertex_data->move_next();
        }
    } else {
        S_ERROR("Unsupported joints component type");
    }
#ifndef NDEBUG
    auto t1 = smlt::get_app()->time_keeper->now_in_us();

    S_DEBUG("Joints loading took {0}us", t1 - t0);
#endif
    final_mesh->vertex_data->move_to(start);

}

void process_weights(const BufferInfo& buffer_info, JSONIterator&,
    smlt::MeshPtr& final_mesh, const VertexSpecification& spec) {
    // Usually floats, sometimes uint8_t
    const uint8_t* d = buffer_info.data.data();
    auto start = final_mesh->vertex_data->cursor_position();

#ifndef NDEBUG
    auto t0 = smlt::get_app()->time_keeper->now_in_us();
#endif

    if (spec.weight_attribute == VERTEX_ATTRIBUTE_4F) {
        for(std::size_t i = 0; i < buffer_info.data.size(); i += buffer_info.stride) {
            const float* ptr = reinterpret_cast<const float*>(d + i);
            float w0 = ptr[0];
            float w1 = ptr[1];
            float w2 = ptr[2];
            float w3 = ptr[3];

            float sum = w0 + w1 + w2 + w3;
            if (sum > 0.0f) { w0/=sum; w1/=sum; w2/=sum; w3/=sum; }

            final_mesh->vertex_data->weights<float>(w0, w1, w2, w3);
            final_mesh->vertex_data->move_next();
        }

    } else if (spec.weight_attribute == VERTEX_ATTRIBUTE_4UB) {
        for(std::size_t i = 0; i < buffer_info.data.size(); i += buffer_info.stride) {
            const uint8_t* ptr = d + i;
            float w0 = ptr[0] / 255.0f;
            float w1 = ptr[1] / 255.0f;
            float w2 = ptr[2] / 255.0f;
            float w3 = ptr[3] / 255.0f;

            final_mesh->vertex_data->weights<float>(w0, w1, w2, w3);
            final_mesh->vertex_data->move_next();
        }
    } else {
        S_ERROR("Unsupported weights component type");
    }
#ifndef NDEBUG
    auto t1 = smlt::get_app()->time_keeper->now_in_us();

    S_DEBUG("Weights loading took {0}us", t1 - t0);
#endif
    final_mesh->vertex_data->move_to(start);
}


void process_positions(const BufferInfo& buffer_info, JSONIterator&,
                       smlt::MeshPtr& final_mesh, VertexSpecification& spec) {

    auto start = final_mesh->vertex_data->cursor_position();

#ifndef NDEBUG
    auto t0 = smlt::get_app()->time_keeper->now_in_us();
#endif
    if(spec.position_attribute == VERTEX_ATTRIBUTE_2F) {

        for(std::size_t i = 0; i < buffer_info.data.size();
            i += buffer_info.stride) {
            const uint8_t* src = &buffer_info.data[i];
            auto x = *(float*)src;
            auto y = *(float*)(src + 4);
            final_mesh->vertex_data->position(x, y);
            final_mesh->vertex_data->move_next();
        }
    } else if(spec.position_attribute == VERTEX_ATTRIBUTE_3F) {

        for(std::size_t i = 0; i < buffer_info.data.size();
            i += buffer_info.stride) {
            const uint8_t* src = &buffer_info.data[i];
            auto x = *(float*)src;
            auto y = *(float*)(src + 4);
            auto z = *(float*)(src + 8);
            final_mesh->vertex_data->position(x, y, z);
            final_mesh->vertex_data->move_next();
        }
    } else if(spec.position_attribute == VERTEX_ATTRIBUTE_4F) {

        for(std::size_t i = 0; i < buffer_info.data.size();
            i += buffer_info.stride) {

            const uint8_t* src = &buffer_info.data[i];
            auto x = *(float*)src;
            auto y = *(float*)(src + 4);
            auto z = *(float*)(src + 8);
            auto w = *(float*)(src + 12);
            final_mesh->vertex_data->position(x, y, z, w);
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

    for(std::size_t i = 0; i < buffer_info.data.size();
        i += buffer_info.stride) {
        auto d = &buffer_info.data[0];
        if(attr == VERTEX_ATTRIBUTE_3F) {
            auto x = *(float*)(d + i);
            auto y = *(float*)(d + i + 4);
            auto z = *(float*)(d + i + 8);
            final_mesh->vertex_data->color(smlt::Color(x, y, z, 1));
        } else if(attr == VERTEX_ATTRIBUTE_4F) {
            auto x = *(float*)(d + i);
            auto y = *(float*)(d + i + 4);
            auto z = *(float*)(d + i + 8);
            auto w = *(float*)(d + i + 12);
            final_mesh->vertex_data->color(smlt::Color(x, y, z, w));
        } else if(attr == VERTEX_ATTRIBUTE_4UB) {
            auto r = *(uint8_t*)(d + i);
            auto g = *(uint8_t*)(d + i + 1);
            auto b = *(uint8_t*)(d + i + 2);
            auto a = *(uint8_t*)(d + i + 3);
            final_mesh->vertex_data->color(smlt::Color::from_bytes(r, g, b, a));
        } else if(attr == VERTEX_ATTRIBUTE_4US) {
            auto r = *(uint16_t*)(d + i);
            auto g = *(uint16_t*)(d + i + 2);
            auto b = *(uint16_t*)(d + i + 4);
            auto a = *(uint16_t*)(d + i + 6);

            final_mesh->vertex_data->color(
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

    for(std::size_t i = 0; i < buffer_info.data.size();
        i += buffer_info.stride) {
        const auto d = &buffer_info.data[0];
        if(spec.normal_attribute == VERTEX_ATTRIBUTE_3F) {
            auto x = *(float*)(d + i);
            auto y = *(float*)(d + i + 4);
            auto z = *(float*)(d + i + 8);
            final_mesh->vertex_data->normal(x, y, z);
        } else if(spec.normal_attribute == VERTEX_ATTRIBUTE_4F) {
            auto x = *(float*)(d + i);
            auto y = *(float*)(d + i + 4);
            auto z = *(float*)(d + i + 8);
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

    for(std::size_t i = 0; i < buffer_info.data.size();
        i += buffer_info.stride) {
        const auto& d = &buffer_info.data[0];

        if(spec.texcoord0_attribute == VERTEX_ATTRIBUTE_2F) {
            auto x = *(float*)(d + i);
            auto y = *(float*)(d + i + 4);
            final_mesh->vertex_data->tex_coord0(x, -y);
        } else if(spec.texcoord0_attribute == VERTEX_ATTRIBUTE_3F) {
            auto x = *(float*)(d + i);
            auto y = *(float*)(d + i + 4);
            auto z = *(float*)(d + i + 8);
            final_mesh->vertex_data->tex_coord0(x, -y, z);
        } else if(spec.texcoord0_attribute == VERTEX_ATTRIBUTE_4F) {
            auto x = *(float*)(d + i);
            auto y = *(float*)(d + i + 4);
            auto z = *(float*)(d + i + 8);
            auto w = *(float*)(d + i + 12);
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

class VectorStreamBuf: public std::streambuf {
public:
    VectorStreamBuf(std::vector<uint8_t>& vec) {
        setg(reinterpret_cast<char*>(vec.data()),
             reinterpret_cast<char*>(vec.data()),
             reinterpret_cast<char*>(vec.data() + vec.size()));
    }
};

static smlt::TexturePtr load_texture(AssetManager* assets, JSONIterator& js,
                                     JSONIterator& texture, int texture_id,
                                     std::istream* bin_chunk,
                                     const std::string& ext = "",
                                     bool use_asset_cache = true) {

    _S_UNUSED(texture_id);

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

    auto apply_sampler_settings = [&sampler](const smlt::TexturePtr& tex) {
        if(!tex) {
            // The texture failed to load (e.g. a missing/unreadable file
            // referenced by the glTF) - nothing to apply settings to.
            return;
        }

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
        tex->flush();
    };

    /* On Dreamcast, prefer a pre-converted .dtex texture if the source gltf
     * was processed with the SMLT_dtex_texture extension (see tools/optimise_gltf).
     * .dtex textures are in the native PowerVR2 format so require no runtime
     * conversion, unlike PNG/JPEG. */
    if(get_platform()->name() == "dreamcast") {
        auto dtex_uri =
            image["extensions"]["SMLT_dtex_texture"]["uri"]->to_str().value_or("");
        if(!dtex_uri.empty()) {
            const char* b64_marker = ";base64,";
            auto marker_pos = dtex_uri.find(b64_marker);
            if(marker_pos != std::string::npos) {
                auto decoded =
                    smlt::base64_decode(dtex_uri.substr(marker_pos + strlen(b64_marker)));
                if(decoded) {
                    auto is = std::make_shared<std::istringstream>(*decoded, std::ios::binary);
                    auto loader = DTEXLoader("embedded.dtex", is);
                    auto tex = assets->create_texture(8, 8);
                    if(loader.into(*tex)) {
                        apply_sampler_settings(tex);
                        tex->flush();
                        return tex;
                    }
                    S_ERROR("Failed to load embedded .dtex texture");
                } else {
                    S_ERROR("Failed to base64 decode embedded .dtex texture");
                }
            } else {
                S_VERBOSE("Loading dreamcast .dtex texture from uri: ", dtex_uri);
                TextureFlags tex_flags;
                tex_flags.use_asset_cache = use_asset_cache;
                auto tex = assets->load_texture(smlt::Path(dtex_uri), tex_flags);
                apply_sampler_settings(tex);
                return tex;
            }
        }
    }

    smlt::Path uri = image["uri"]->to_str().value_or("");
    if(!uri.str().empty()) {
        if(!ext.empty()) {
            uri = uri.replace_ext(ext);
        }

        S_VERBOSE("Loading texture from uri: ", uri.str());
        TextureFlags tex_flags;
        tex_flags.use_asset_cache = use_asset_cache;
        auto tex = assets->load_texture(uri, tex_flags);
        apply_sampler_settings(tex);
        return tex;
    } else {
        Accessor acc;
        acc.component_type = BYTE;
        acc.type = "SCALAR";
        acc.buffer_view_id = image["bufferView"]->to_int().value_or(-1);
        if(acc.buffer_view_id != -1) {
            auto buff = process_buffer(js, acc, bin_chunk);
            auto mime = image["mimeType"]->to_str().value_or("");

            VectorStreamBuf buffer(buff.data);
            auto is = std::make_shared<std::istream>(&buffer);

            if(mime == "image/png") {
                S_VERBOSE("Loading png from stream");
                auto loader = PNGLoader("something.png", is);
                auto tex = assets->create_texture(8, 8);
                loader.into(*tex);
                tex->flush();
                return tex;
            } else if(mime == "image/jpeg") {
                S_VERBOSE("Loading jpeg from stream");
                auto loader = TextureLoader("something.jpg", is);
                auto tex = assets->create_texture(8, 8);
                loader.into(*tex);
                tex->flush();
                return tex;
            } else {
                S_ERROR("Unsupported texture format");
            }
        }
    }

    return smlt::TexturePtr();
}

static smlt::MaterialPtr create_default_material(AssetManager* assets) {
    auto mat = assets->clone_default_material();
    mat->set_name("Default");
    mat->set_lighting_enabled(true);
    mat->set_textures_enabled(0);
    mat->set_cull_mode(smlt::CULL_MODE_NONE);
    return mat;
}

smlt::MaterialPtr load_material(AssetManager* assets, JSONIterator& js,
                                JSONIterator& material, int material_id,
                                const std::vector<smlt::TexturePtr>& textures) {

    _S_UNUSED(material_id);
    _S_UNUSED(js);

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

    /* Textures referenced by index can be null here if they failed to load
     * (e.g. a missing/unreadable file referenced by the glTF) - treat that
     * the same as the material simply not referencing a texture, rather
     * than enabling a map that points at nothing (which would crash later
     * when the renderer tries to upload/bind it). */
    smlt::EnabledTextureMask enabled = 0;
    smlt::MaterialPtr ret = assets->clone_default_material();
    if(base_texture_id >= 0 && textures[base_texture_id]) {
        ret->set_base_color_map(textures[base_texture_id]);
        enabled |= BASE_COLOR_MAP_ENABLED;
    }

    if(metallic_roughness_texture_id >= 0 &&
       textures[metallic_roughness_texture_id]) {
        ret->set_metallic_roughness_map(
            textures[metallic_roughness_texture_id]);
        enabled |= METALLIC_ROUGHNESS_MAP_ENABLED;
    }

    if(normal_texture_id >= 0 && textures[normal_texture_id]) {
        ret->set_normal_map(textures[normal_texture_id]);
        enabled |= NORMAL_MAP_ENABLED;
    }

    if(occ_texture_id >= 0 && textures[occ_texture_id]) {
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

    auto cutoff = material["alphaCutoff"].is_valid()
                      ? material["alphaCutoff"]->to_float().value_or(0.5f)
                      : 0.5f;

    ret->set_blend_func((alpha_mode == "OPAQUE") ? smlt::BLEND_NONE
                        : (alpha_mode == "MASK") ? smlt::BLEND_MASK
                                                 : smlt::BLEND_ALPHA);
    ret->set_alpha_threshold(cutoff);
    ret->set_metallic(metallic);
    ret->set_roughness(roughness);
    ret->set_base_color(color);

    // Look for the unlit extension, if it's there then disable lighting
    if(material->has_key("extensions")) {
        auto ext = material["extensions"];
        if(ext->has_key("KHR_materials_unlit")) {
            ret->set_lighting_enabled(false);
        }
    }

    return ret;
}

static smlt::MeshPtr load_mesh(AssetManager* assets, JSONIterator& js,
                               JSONIterator& mesh, int mesh_id,
                               const std::vector<Accessor>& accessors,
                               const std::vector<smlt::MaterialPtr>& materials,
                               std::istream* bin_chunk,
                               std::shared_ptr<Mesh::Skin> skin) {
    _S_UNUSED(mesh_id);

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

        int joints_id = -1;
        int weights_id = -1;

        int mode = TRIANGLES;
    };

    std::vector<MeshPrimitive> primitives;


    const std::map<TypeKey, VertexAttribute> lookup = {
        {TypeKey(FLOAT,          "VEC2"), VERTEX_ATTRIBUTE_2F },
        {TypeKey(FLOAT,          "VEC3"), VERTEX_ATTRIBUTE_3F },
        {TypeKey(FLOAT,          "VEC4"), VERTEX_ATTRIBUTE_4F },
        // {TypeKey(FLOAT, "SCALAR"), VERTEX_ATTRIBUTE_1F},
        {TypeKey(UNSIGNED_BYTE,  "VEC4"), VERTEX_ATTRIBUTE_4UB},
        {TypeKey(UNSIGNED_SHORT, "VEC4"), VERTEX_ATTRIBUTE_4US},
        {TypeKey(UNSIGNED_BYTE,  "VEC4"), VERTEX_ATTRIBUTE_4UB},
        {TypeKey(FLOAT,          "VEC4"), VERTEX_ATTRIBUTE_4F },
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
        mp.joints_id = primitive["attributes"]["JOINTS_0"]->to_int().value_or(-1);
        mp.weights_id = primitive["attributes"]["WEIGHTS_0"]->to_int().value_or(-1);
        mp.mode = primitive["mode"]->to_int().value_or(TRIANGLES);

        S_DEBUG("Joint on primitive: {0}", mp.joints_id);

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
            VERTEX_ATTRIBUTE_NONE, VERTEX_ATTRIBUTE_NONE, clean_diffuse(diff), VERTEX_ATTRIBUTE_NONE,
            primitive.joints_id >= 0 ? (process_attribute(primitive.joints_id)) : VERTEX_ATTRIBUTE_NONE, // Joints
            primitive.weights_id >= 0 ? (process_attribute(primitive.weights_id)) : VERTEX_ATTRIBUTE_NONE); // Weights

        if(!final_mesh) {
            final_mesh = assets->create_mesh(spec);
        } else if(final_mesh->vertex_data->vertex_specification() != spec) {
            S_ERROR("GLTF mesh contains multiple vertex types which is "
                    "currently unsupported");
        }

        auto mode = primitive.mode;
        if(mode != TRIANGLES && mode != TRIANGLE_STRIP) {
            S_ERROR("Mesh with unsupported mode: {0}", mode);
            continue;
        }

        auto sm_name = _F("Primitives: {0}").format(i++);
        auto material_id = primitive.material_id;
        auto material =
            (material_id >= 0) ? materials[material_id] : materials.back();

        int offset = final_mesh->vertex_data->count();
        final_mesh->vertex_data->move_to(offset);

        if(primitive.position_id >= 0) {
            auto position = accessors[primitive.position_id];
            auto buffer_info = process_buffer(js, position, bin_chunk);
            process_positions(buffer_info, js, final_mesh, spec);
        }

        if(primitive.normal_id >= 0) {
            auto normal = accessors[primitive.normal_id];
            auto buffer_info = process_buffer(js, normal, bin_chunk);
            process_normals(buffer_info, js, final_mesh, spec);
        }

        if(primitive.color_id >= 0) {
            auto color = accessors[primitive.color_id];
            auto buffer_info = process_buffer(js, color, bin_chunk);
            process_colors(buffer_info, js, final_mesh, diff);
        }

        if(primitive.texcoord_id >= 0) {
            auto texcoord0 = accessors[primitive.texcoord_id];
            auto buffer_info = process_buffer(js, texcoord0, bin_chunk);
            process_texcoord0s(buffer_info, js, final_mesh, spec);
        }

        auto indices_id = primitive.indexes_id;
        if(indices_id >= 0) {

            auto indices = accessors[indices_id];
            auto buffer_info = process_buffer(js, indices, bin_chunk);
            S_VERBOSE("Populating indices");

            const auto d = &buffer_info.data[0];

            /* FIXME: Maybe we can reuse submeshes? */
            auto sm = final_mesh->create_submesh(
                sm_name, material,
                (buffer_info.c_type == UNSIGNED_INT) ? INDEX_TYPE_32_BIT
                : (buffer_info.c_type == UNSIGNED_SHORT ||
                   buffer_info.c_type == SHORT)
                    ? INDEX_TYPE_16_BIT
                    : INDEX_TYPE_8_BIT,
                mode == TRIANGLES ? MESH_ARRANGEMENT_TRIANGLES
                                  : MESH_ARRANGEMENT_TRIANGLE_STRIP);

            for(std::size_t i = 0; i < buffer_info.data.size();
                i += buffer_info.c_stride) {

                switch(buffer_info.c_type) {
                    case BYTE:
                    case UNSIGNED_BYTE: {
                        auto u8idx = *(uint8_t*)(d + i);
                        sm->index_data->index(u8idx + offset);
                    } break;
                    case SHORT:
                    case UNSIGNED_SHORT: {
                        auto u16idx = *(uint16_t*)(d + i);
                        sm->index_data->index(u16idx + offset);
                    } break;
                    case UNSIGNED_INT: {
                        auto u32idx = *(uint32_t*)(d + i);
                        sm->index_data->index(u32idx + offset);
                    } break;
                    default:
                        S_ERROR("Unsupported index type: {0}",
                                buffer_info.c_type);
                        break;
                }
            }

            sm->index_data->done();
        }
        if(skin && !final_mesh->skin) {
            /* Shared with every other mesh bound to the same skeleton -
             * an Armature node is what actually poses it */
            final_mesh->skin = skin;
        }

        if (primitive.joints_id >= 0) {
            auto joints = accessors[primitive.joints_id];
            auto buffer_info = process_buffer(js, joints, bin_chunk);
            process_joints(buffer_info, js, final_mesh, spec);
        }

        if (primitive.weights_id >= 0) {
            auto weights = accessors[primitive.weights_id];
            auto buffer_info = process_buffer(js, weights, bin_chunk);
            process_weights(buffer_info, js, final_mesh, spec);
        }
    }

    final_mesh->vertex_data->done();
    return final_mesh;
}

/* Everything we need to know about the file's skins to turn them into
 * Armature -> Joint hierarchies */
/* One generated Armature. A glTF skin maps to one of these, except that
 * skins driving the same joints in the same order are merged - exporters
 * routinely emit one skin per mesh part over a single shared skeleton, and
 * the only thing that differs between them (the inverse bind matrices) lives
 * on the mesh, not the armature. */
struct ArmatureInfo {
    std::string name;

    /* glTF node index of each joint, in the order the inverse bind matrices
     * expect them */
    std::vector<int> joint_nodes;

    /* The node the Armature is inserted above. This is the common ancestor
     * of every joint, so all of them end up below the armature without any
     * of them having their world transform changed. */
    int anchor_node = -1;

    /* Prefab node id given to the generated Armature. glTF node ids are
     * used as prefab node ids directly, so these continue on from the end
     * of the file's node list. */
    uint32_t armature_node_id = 0;

    /* The meshes this armature poses, in glTF mesh order */
    std::vector<smlt::MeshPtr> meshes;
};

struct SkeletonInfo {
    std::vector<ArmatureInfo> armatures;

    /* glTF skin index -> the armature driving it (-1 if unusable) */
    std::vector<int> skin_to_armature;

    /* glTF skin index -> the inverse bind matrices to attach to its meshes */
    std::vector<std::shared_ptr<Mesh::Skin>> skin_data;

    /* glTF node index -> parent node index (-1 for roots) */
    std::vector<int> parents;

    /* glTF node index -> (armature index, joint index within it) */
    std::map<int, std::pair<int, int>> joints;

    /* glTF node index -> indices of the armatures anchored above it */
    std::map<int, std::vector<int>> anchors;
};

/* Builds the node -> parent map. glTF only stores the relationship in the
 * other direction */
static void build_parent_map(JSONIterator& js, std::vector<int>& parents) {
    auto nodes = js["nodes"];
    const std::size_t count = nodes->size();

    parents.assign(count, -1);

    for(std::size_t i = 0; i < count; ++i) {
        auto node = nodes[i];
        if(!node->has_key("children")) {
            continue;
        }

        for(auto& child: node["children"]) {
            int child_id = child.to_int().value_or(-1);
            if(child_id >= 0 && (std::size_t)child_id < count) {
                parents[child_id] = (int)i;
            }
        }
    }
}

static int lowest_common_ancestor(const std::vector<int>& parents,
                                  const std::vector<int>& depths, int a,
                                  int b) {
    if(a < 0 || b < 0) {
        return -1;
    }

    while(depths[a] > depths[b]) {
        a = parents[a];
    }

    while(depths[b] > depths[a]) {
        b = parents[b];
    }

    while(a != b) {
        if(a < 0 || b < 0) {
            return -1;
        }

        a = parents[a];
        b = parents[b];
    }

    return a;
}

static void load_skeletons(JSONIterator& js,
                           const std::vector<Accessor>& accessors,
                           std::istream* bin_chunk, SkeletonInfo& info) {
    auto skins_it = js["skins"];
    if(!skins_it.is_valid() || skins_it->size() == 0) {
        return;
    }

    build_parent_map(js, info.parents);

    std::vector<int> depths(info.parents.size(), 0);
    for(std::size_t i = 0; i < info.parents.size(); ++i) {
        int depth = 0;
        for(int p = info.parents[i]; p >= 0; p = info.parents[p]) {
            ++depth;
        }
        depths[i] = depth;
    }

    int skin_index = 0;
    for(auto& skin_node_it: skins_it) {
        auto skin_node = skin_node_it.to_iterator();

        auto name = skin_node["name"]->to_str().value_or(
            _F("Armature {0}").format(skin_index));

        std::vector<int> joint_nodes;
        for(auto& j: skin_node["joints"]) {
            int node_index = j.to_int().value_or(-1);
            if(node_index < 0 || (std::size_t)node_index >= info.parents.size()) {
                S_ERROR("Invalid joint index in skin");
                node_index = -1;
            }

            joint_nodes.push_back(node_index);
        }

        auto skin = std::make_shared<Mesh::Skin>();
        info.skin_data.push_back(skin);

        int ibm_accessor_idx =
            skin_node["inverseBindMatrices"]->to_int().value_or(-1);
        if(ibm_accessor_idx < 0) {
            S_WARN("Skin has no inverse bind matrices.");
        } else {
            auto buffer_info =
                process_buffer(js, accessors[ibm_accessor_idx], bin_chunk);

            std::size_t ibm_count =
                js["accessors"][ibm_accessor_idx]["count"]->to_int().value_or(0);

            if(buffer_info.data.size() < ibm_count * 16 * sizeof(float)) {
                S_ERROR("IBM buffer too small: expected {0} floats, got {1}",
                        ibm_count * 16, buffer_info.data.size() / sizeof(float));
                ibm_count = 0;
            }

            const float* data =
                reinterpret_cast<const float*>(buffer_info.data.data());

            skin->inverse_bind_matrices.reserve(ibm_count);
            for(std::size_t i = 0; i < ibm_count; ++i) {
                // 16 floats per Mat4 as usual
                FloatArray arr(data + i * 16, data + (i + 1) * 16);
                skin->inverse_bind_matrices.push_back(Mat4(arr));
            }

            S_DEBUG("Loaded {0} joints and {1} inverse bind matrices",
                    joint_nodes.size(), skin->inverse_bind_matrices.size());
        }

        ++skin_index;

        /* Anything driving the same joints in the same order is the same
         * skeleton as far as we're concerned, so reuse its armature rather
         * than nesting a second one that would fight it for the joints. */
        int existing = -1;
        for(std::size_t a = 0; a < info.armatures.size(); ++a) {
            if(info.armatures[a].joint_nodes == joint_nodes) {
                existing = (int)a;
                break;
            }
        }

        if(existing >= 0) {
            info.skin_to_armature.push_back(existing);
            continue;
        }

        /* Where to hang the armature. The file's own "skeleton" entry is
         * optional (and not always the common root it claims to be), so
         * derive it from the joints themselves. */
        int anchor = -1;
        bool have_anchor = false;
        for(auto joint_node: joint_nodes) {
            if(joint_node < 0) {
                continue;
            }

            anchor = (have_anchor) ? lowest_common_ancestor(
                                         info.parents, depths, anchor, joint_node)
                                   : joint_node;
            have_anchor = true;

            if(anchor < 0) {
                /* The joints aren't all in one tree, so there's nowhere to
                 * put an armature that would own all of them */
                break;
            }
        }

        if(anchor < 0) {
            S_WARN("Skin '{0}' has no usable joints and will be ignored", name);
            info.skin_to_armature.push_back(-1);
            continue;
        }

        ArmatureInfo out;
        out.name = name;
        out.joint_nodes = joint_nodes;
        out.anchor_node = anchor;
        out.armature_node_id =
            (uint32_t)(info.parents.size() + info.armatures.size());

        const int armature_index = (int)info.armatures.size();

        for(std::size_t i = 0; i < out.joint_nodes.size(); ++i) {
            if(out.joint_nodes[i] < 0) {
                continue;
            }

            if(info.joints.count(out.joint_nodes[i])) {
                S_WARN("Node {0} is a joint of more than one skeleton - only "
                       "the first will pose it",
                       out.joint_nodes[i]);
                continue;
            }

            info.joints[out.joint_nodes[i]] =
                std::make_pair(armature_index, (int)i);
        }

        info.anchors[anchor].push_back(armature_index);
        info.armatures.push_back(out);
        info.skin_to_armature.push_back(armature_index);
    }
}

static bool spawn_node_recursively(Prefab& prefab, int32_t parent, int node_id,
                                   JSONIterator& js,
                                   const std::vector<smlt::MeshPtr>& meshes,
                                   const SkeletonInfo& skeletons) {
    auto nodes = js["nodes"];
    auto node = nodes[node_id];

    /* If any skeleton is anchored here, the Armature goes in above this
     * node. Chaining them keeps things sane in the (unusual) case of
     * several skins sharing a common root. */
    auto anchored = skeletons.anchors.find(node_id);
    if(anchored != skeletons.anchors.end()) {
        for(auto armature_index: anchored->second) {
            auto& armature = skeletons.armatures[armature_index];

            PrefabNode armature_node;
            armature_node.id = armature.armature_node_id;
            armature_node.node_type_name = "armature";
            armature_node.name = armature.name;
            armature_node.params.set("translation", Vec3());
            armature_node.params.set("rotation", Quaternion());
            armature_node.params.set("scale_factor", Vec3(1, 1, 1));

            for(std::size_t i = 0; i < armature.meshes.size(); ++i) {
                auto key = (i == 0) ? std::string("mesh")
                                    : Armature::extra_mesh_param_prefix() +
                                          std::to_string(i);
                armature_node.params.set(key, armature.meshes[i]);
            }

            prefab.push_node(armature_node, parent);
            parent = (int32_t)armature_node.id;
        }
    }

    PrefabNode prefab_node;
    prefab_node.id = node_id;

    auto light_id =
        node["extensions"]["KHR_lights_punctual"]["light"]->to_int().value_or(
            -1);

    auto joint = skeletons.joints.find(node_id);

    if(joint != skeletons.joints.end()) {
        prefab_node.node_type_name = "joint";
        prefab_node.params.set("joint_index", joint->second.second);

        if(node->has_key("mesh")) {
            S_WARN("Node {0} is both a joint and a mesh - the mesh will be "
                   "ignored",
                   node_id);
        }
    } else if(node->has_key("mesh") && node->has_key("skin")) {
        /* The mesh is posed by (and rendered from) the armature, not from
         * here. Per the glTF spec a skinned mesh node's own transform is
         * ignored anyway, so all this node does now is hold its place in
         * the hierarchy for anything parented to it. */
        prefab_node.node_type_name = "stage";
    } else if(node->has_key("mesh") && !meshes.empty()) {
        prefab_node.node_type_name = "actor";
        prefab_node.params.set("mesh",
                               meshes[node["mesh"]->to_int().value_or(0)]);
    } else if(node->has_key("camera")) {
        auto camera_id = node["camera"]->to_int().value_or(-1);
        if(camera_id >= 0) {
            prefab_node.node_type_name = "camera3d";

            auto cam_node = js["cameras"][camera_id];
            auto persp_node = cam_node["perspective"];
            auto type = cam_node["type"]->to_str().value_or("perspective");
            if(type == "perspective") {
                prefab_node.params.set(
                    "aspect",
                    persp_node["aspectRatio"]->to_float().value_or(1.777f));

                /* glTF stores yfov in radians, but the camera takes degrees */
                prefab_node.params.set(
                    "yfov", Degrees(Radians(persp_node["yfov"]
                                                ->to_float()
                                                .value_or(1.047f)))
                                .to_float());
                prefab_node.params.set(
                    "znear", persp_node["znear"]->to_float().value_or(1.0f));

                if(persp_node["zfar"].is_valid()) {
                    prefab_node.params.set(
                        "zfar",
                        persp_node["zfar"]->to_float().value_or(1000.0f));
                }
            }
        }
    } else if(light_id >= 0) {
        auto light =
            js["extensions"]["KHR_lights_punctual"]["lights"][light_id];
        if(light.is_valid()) {
            prefab_node.node_type_name = "light";
            prefab_node.params.set("color", parse_color3(light["color"]));
            prefab_node.params.set(
                "intensity", light["intensity"]->to_float().value_or(1.0f));
            prefab_node.params.set("range",
                                   light["range"]->to_float().value_or(100.0f));
            prefab_node.params.set(
                "type", light["type"]->to_str().value_or("directional"));
        }
    } else {
        prefab_node.node_type_name = "stage";
    }

    GLTFLoader::NodeFactoryInput input;

    if(node["extras"].is_valid()) {
        for(auto& k: node["extras"]->keys()) {
            auto it = node["extras"];
            if(it[k]->is_str()) {
                prefab_node.params.set(k.c_str(), it[k]->to_str().value_or(""));
            } else if(it[k]->is_number()) {
                if(it[k]->is_float()) {
                    prefab_node.params.set(k.c_str(),
                                           it[k]->to_float().value_or(0.0f));
                } else {
                    prefab_node.params.set(k.c_str(),
                                           (int)it[k]->to_int().value_or(0));
                }

            } else if(it[k]->is_bool()) {
                prefab_node.params.set(k.c_str(),
                                       it[k]->to_bool().value_or(false));
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
                    prefab_node.params.set(k.c_str(), farr);
                } else if(iarr.size()) {
                    prefab_node.params.set(k.c_str(), iarr);
                } else if(barr.size()) {
                    prefab_node.params.set(k.c_str(), barr);
                }
            }
        }
    }

    auto extract_transform =
        [](JSONIterator& node) -> std::tuple<Vec3, Quaternion, Vec3> {
        auto trn = parse_pos(node["translation"]);
        auto rot = (node->has_key("rotation"))
                       ? parse_quaternion(node["rotation"])
                       : smlt::Quaternion();
        auto sf = parse_scale(node["scale"]);

        if(node["matrix"]) {
            auto mat = Mat4();
            for(int i = 0; i < 16; ++i) {
                mat[i] = node["matrix"][i]->to_float().value_or(0.0f);
            }

            mat.extract_rotation_and_translation(rot, trn);
            // FIXME: Handle different axis.
            // FIXME: This assumes axis-aligned, Y+ up!
            sf.x = mat[0];
            sf.y = mat[5];
            sf.z = mat[10];
        }

        return std::make_tuple(trn, rot, sf);
    };

    prefab_node.name = node["name"]->to_str().value_or("");
    // FIXME: Additional properties!

    auto trn = extract_transform(node);
    prefab_node.params.set("translation", std::get<0>(trn));
    prefab_node.params.set("rotation", std::get<1>(trn));
    prefab_node.params.set("scale_factor", std::get<2>(trn));

    prefab.push_node(prefab_node, parent);

    if(node->has_key("children")) {
        for(auto& child: node["children"]) {
            spawn_node_recursively(prefab, prefab_node.id,
                                   child.to_int().value_or(0), js, meshes,
                                   skeletons);
        }
    }

    return true;
}

bool GLTFLoader::into(Loadable& resource, const LoaderOptions& options) {
    auto prefab = loadable_to<Prefab>(resource);
    std::shared_ptr<std::istream> bin_chunk;

    uint32_t magic;
    data_->read((char*)&magic, sizeof(magic));
    if(magic == 0x46546C67) {
        // This is a glb file
        uint32_t file_length;
        uint32_t version;

        data_->read((char*)&version, sizeof(version));

        if(version != 2) {
            S_ERROR("Unsupported glTF version: {0}", version);
            return false;
        }

        data_->read((char*)&file_length, sizeof(file_length));

        uint32_t chunk_length;
        uint32_t chunk_type;
        data_->read((char*)&chunk_length, sizeof(chunk_length));
        data_->read((char*)&chunk_type, sizeof(chunk_type));
        if(chunk_type != 0x4E4F534A) {
            S_ERROR("First chunk is not JSON");
            return false;
        }

        uint32_t here = data_->tellg();

        uint32_t json_end = here + chunk_length;

        if(json_end < file_length) {
            // Now let's open a second stream and seek to the bin chunk
            bin_chunk = std::make_shared<std::ifstream>(filename_.str(),
                                                        std::ios::binary);

            uint32_t bin_length;
            uint32_t bin_type;
            bin_chunk->seekg(json_end);
            bin_chunk->read((char*)&bin_length, sizeof(uint32_t));
            bin_chunk->read((char*)&bin_type, sizeof(uint32_t));

            if(bin_type != 0x004E4942) {
                S_ERROR("Unrecognised chunk: {0}", bin_type);
                return false;
            }
        }
    } else {
        // Rewind, bo selecta
        data_->seekg(0, std::ios::beg);
    }

    auto js = json_read(this->data_);

    if(!check_gltf_version(js)) {
        return false;
    }

    auto maybe_scene_it = find_scene(js);
    if(!maybe_scene_it) {
        S_ERROR("No scene in gltf file");
        return false;
    }

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

    bool use_asset_cache = true;
    if(options.count("use_asset_cache")) {
        use_asset_cache = any_cast<bool>(options.at("use_asset_cache"));
    }

    std::vector<TexturePtr> textures;
    std::vector<MaterialPtr> materials;
    std::vector<MeshPtr> meshes;
    std::vector<Accessor> accessors;

    SkeletonInfo skeletons;

    std::unordered_map<int, int> mesh_to_skin;

    // Looping through all the nodes for skin association
    for (auto &node_it : js["nodes"]) {
        auto node = node_it.to_iterator();

        int mesh_id = node["mesh"]->to_int().value_or(-1);
        int skin_id = node["skin"]->to_int().value_or(-1);

        if (mesh_id >= 0 && skin_id >= 0) {
            mesh_to_skin[mesh_id] = skin_id;
            S_DEBUG("Mesh {} uses skin {}", mesh_id, skin_id);
        }
    }

    /* This is the most complicated part of the loader. A GLTF file has a
       heirarchy of: mesh/animation -> accessor -> bufferView -> buffer */
    for(auto& acc_node: js["accessors"]) {
        auto acc = acc_node.to_iterator();
        Accessor accessor;
        accessor.type = acc["type"]->to_str().value_or("SCALAR");
        accessor.component_type =
            (ComponentType)acc["componentType"]->to_int().value_or(5121);
        accessor.buffer_view_id = acc["bufferView"]->to_int().value_or(-1);
        accessors.push_back(accessor);
    }

    int j = 0;
    auto textures_it = js["textures"];
    for(auto& node: textures_it) {
        auto tex_it = node.to_iterator();
        auto tex = load_texture(&prefab->asset_manager(), js, tex_it, j++,
                                bin_chunk.get(), ext, use_asset_cache);
        textures.push_back(tex);
        prefab->push_texture(tex);
    }

    auto materials_it = js["materials"];
    j = 0;
    for(auto& node: materials_it) {
        auto mat_it = node.to_iterator();
        auto mat =
            load_material(&prefab->asset_manager(), js, mat_it, j++, textures);
        prefab->push_material(mat);
        materials.push_back(mat);
    }

    /* Add the default material at the end */
    auto default_material = create_default_material(&prefab->asset_manager());
    prefab->push_material(default_material);
    materials.push_back(default_material);

    /* Skins are parsed up-front: several meshes can share one, and the
     * Armature that poses them has to be woven into the node hierarchy as
     * it's spawned below. */
    load_skeletons(js, accessors, bin_chunk.get(), skeletons);

    auto meshes_it = js["meshes"];
    j = 0;
    for(auto& node: meshes_it) {
        auto mesh_it = node.to_iterator();
        int mesh_id = j;
        int skin_id = -1;

        // Checking if this mesh has an associated skin
        if (mesh_to_skin.count(mesh_id)) {
            skin_id = mesh_to_skin[mesh_id];
            S_DEBUG("Associating skin {} to mesh {}", skin_id, mesh_id);
        }

        std::shared_ptr<Mesh::Skin> skin;
        int armature_index = -1;
        if(skin_id >= 0 && (std::size_t)skin_id < skeletons.skin_data.size()) {
            skin = skeletons.skin_data[skin_id];
            armature_index = skeletons.skin_to_armature[skin_id];
        }

        auto mesh = load_mesh(&prefab->asset_manager(), js, mesh_it, mesh_id,
                              accessors, materials, bin_chunk.get(), skin);
        prefab->push_mesh(mesh);
        meshes.push_back(mesh);

        if(armature_index >= 0) {
            skeletons.armatures[armature_index].meshes.push_back(mesh);
        }

        j++;
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
                    spawn_node_recursively(*prefab, -1, i, js, meshes,
                                           skeletons);
                    return true;
                }
            }
        }

        S_ERROR("Unable to locate specified root_name: {0}", root_name);
        // FIXME: convey error!
        return false;
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
        spawn_node_recursively(*prefab, -1, node_id, js, meshes, skeletons);
    }

    auto animations_it = js["animations"];
    for(auto& node: animations_it) {
        auto node_it = node.to_iterator();

        std::string name = "anim";
        if(node_it["name"]) {
            name = node_it["name"]->to_str().value();
        }

        for(auto& ch_node: node_it["channels"]) {
            auto ch_node_it = ch_node.to_iterator();
            auto target = ch_node_it["target"];
            auto target_node = target["node"]->to_int();
            if(!target_node) {
                continue;
            }
            auto target_node_id = target_node.value();

            auto path_str = target["path"]->to_str().value_or("translation");
            auto path = (path_str == "translation") ? ANIMATION_PATH_TRANSLATION
                        : (path_str == "rotation")  ? ANIMATION_PATH_ROTATION
                        : (path_str == "scale")     ? ANIMATION_PATH_SCALE
                                                    : ANIMATION_PATH_WEIGHTS;

            auto prefab_node = prefab->node(target_node_id);

            if(!prefab_node) {
                continue;
            }

            auto sampler_id = ch_node_it["sampler"]->to_int();
            if(!sampler_id) {
                continue;
            }

            auto sampler = node_it["samplers"][sampler_id.value()];
            auto interpolation_name =
                sampler["interpolation"]->to_str().value_or("LINEAR");
            auto input_id = sampler["input"]->to_int();
            auto output_id = sampler["output"]->to_int();
            if(!input_id || !output_id) {
                continue;
            }

            auto interpolation = (interpolation_name == "LINEAR")
                                     ? ANIMATION_INTERPOLATION_LINEAR
                                 : (interpolation_name == "STEP")
                                     ? ANIMATION_INTERPOLATION_STEP
                                     : ANIMATION_INTERPOLATION_CUBIC_SPLINE;

            /* FIXME: This process ends up with data duplicated in memory
             * (first in the raw data form returned from process_buffer, and
             * then the typed form)
             *
             * Once we've converted to a typed array, the AnimationData
             * constructor does an inline transformation - clearing the source
             * vector as it goes. It would be better if process_buffer returned
             * a vector that could be cleared as we convert it into a typed
             * array */
            auto times_buffer = process_buffer(js, accessors[input_id.value()],
                                               bin_chunk.get());

            std::vector<float> times;
            times_buffer.to_typed_array(times);

            auto output_buffer = process_buffer(
                js, accessors[output_id.value()], bin_chunk.get());

            AnimationDataPtr data;

            if(output_buffer.c_count == 1) {
                std::vector<float> output;
                output_buffer.to_typed_array(output);
                data =
                    std::make_shared<AnimationData>(times, std::move(output));
            } else if(output_buffer.c_count == 3) {
                std::vector<Vec3> output;
                output_buffer.to_typed_array(output);
                data =
                    std::make_shared<AnimationData>(times, std::move(output));
            } else if(output_buffer.c_count == 4) {
                std::vector<Quaternion> output;
                output_buffer.to_typed_array(output);
                data =
                    std::make_shared<AnimationData>(times, std::move(output));
            } else {
                S_ERROR("Unhandled buffer type");
                continue;
            }

            prefab->push_animation_channel(name, prefab_node.value(), path,
                                           interpolation, data);
        }
    }

    return true;
}

} // namespace loaders
} // namespace smlt
