#ifndef BATCHER_H_INCLUDED
#define BATCHER_H_INCLUDED

#include <list>
#include <unordered_map>
#include <memory>
#include <kazbase/exceptions.h>
#include "../../types.h"
#include "../../generic/auto_weakptr.h"
#include "../../material_constants.h"
#include "../../gpu_program.h"
#include "../../interfaces.h"

//FIXME: Replace with std::optional when C++17 is done
#include "../../std/optional.hpp"

namespace kglt {

template <class T>
inline void hash_combine(std::size_t& seed, const T& v)
{
    std::hash<T> hasher;
    seed ^= hasher(v) + 0x9e3779b9 + (seed<<6) + (seed>>2);
}

class Stage;
class SubActor;
class Camera;
class MaterialPass;
class GPUProgramInstance;

struct GroupData {
    typedef std::shared_ptr<GroupData> ptr;

    virtual ~GroupData() {}

    std::size_t hash(bool reset=false) const {
        if(!hash_ || reset) {
            hash_ = do_hash();
        }

        return hash_;
    }

private:
    mutable std::size_t hash_ = 0;
    virtual std::size_t do_hash() const = 0;
};

class RenderGroup {
public:
    typedef std::shared_ptr<RenderGroup> ptr;

    RenderGroup(RenderGroup* parent):
        parent_(parent) {

    }

    ///Traverses the tree and calls the callback on each subactor we encounter
    void traverse(std::function<void (Renderable*, MaterialPass*)> callback) {
        GPUProgram* program = get_root().current_program();

        bind(program);

        for(auto& p: renderables_) {
            assert(p.first);
            assert(p.second);

            callback(p.first, p.second);
        }

        for(std::pair<std::size_t, std::shared_ptr<RenderGroup>> group: this->children_) {
            group.second->traverse(callback);
        }

        unbind(program);
    }

    template<typename RenderGroupType>
    RenderGroup& get_or_create(const GroupData& data) {
        static_assert(std::is_base_of<RenderGroup, RenderGroupType>::value, "RenderGroupType must derive RenderGroup");

        const typename RenderGroupType::data_type& cast_data = static_cast<const typename RenderGroupType::data_type&>(data);

        auto& container = children_;
        RenderGroups::const_iterator it = container.find(data.hash());

        if(it != container.end()) {
            return *(*it).second;
        } else {
            std::shared_ptr<RenderGroupType> new_child(new RenderGroupType(this, cast_data));
            container.insert(std::make_pair(cast_data.hash(), new_child));
            return *new_child;
        }
    }

    template<typename RenderGroupType>
    bool exists(const GroupData& data) const {
        static_assert(std::is_base_of<RenderGroup, RenderGroupType>::value, "RenderGroupType must derive RenderGroup");
        return children_.find(data.hash()) != children_.end();
    }

    template<typename RenderGroupType>
    RenderGroup& get(const GroupData& data) {
        static_assert(std::is_base_of<RenderGroup, RenderGroupType>::value, "RenderGroupType must derive RenderGroup");

        RenderGroups::const_iterator it = children_.find(data.hash());

        if(it != children_.end()) {
            return *(*it).second;
        } else {
            throw DoesNotExist<RenderGroupType>();
        }
    }

    void add(Renderable* renderable, MaterialPass* pass) {
        renderables_.push_back(std::make_pair(renderable, pass));
    }

    virtual void bind(GPUProgram* program) = 0;
    virtual void unbind(GPUProgram* program) = 0;

    virtual RenderGroup& get_root() {
        return parent_->get_root();
    }

    void clear() {
        renderables_.clear();
        for(auto group: children_) {
            group.second->clear();
        }
        children_.clear();
    }

    void set_current_program(GPUProgram::ptr program) { current_program_ = program; }
    GPUProgram* current_program() const { return current_program_.get(); }

protected:
    RenderGroup* parent_;

private:
    typedef std::unordered_map<std::size_t, std::shared_ptr<RenderGroup> > RenderGroups;

    RenderGroups children_;

    std::list<std::pair<Renderable*, MaterialPass*> > renderables_;

    GPUProgram::ptr current_program_;
};


class RootGroup : public RenderGroup {
public:
    typedef std::shared_ptr<RootGroup> ptr;
    typedef int data_type;

    RootGroup(WindowBase& window, StageID stage, CameraID camera):
        RenderGroup(nullptr),
        window_(window),
        stage_id_(stage),
        camera_id_(camera) {}

    virtual ~RootGroup() {}

    void bind(GPUProgram* program);
    void unbind(GPUProgram* program) {}

    RenderGroup& get_root() {
        return *this;
    }

    StagePtr stage();
    ProtectedPtr<CameraProxy> camera();

    void insert(Renderable* renderable, MaterialPass* pass, const std::vector<kglt::LightID> &lights);

private:
    WindowBase& window_;
    StageID stage_id_;
    CameraID camera_id_;

    void generate_mesh_groups(
        RenderGroup* parent,
        Renderable* renderable,
        MaterialPass* pass,
        const std::vector<kglt::LightID> &lights
    );
};

struct RenderableGroupData : public GroupData {
    std::size_t do_hash() const {
        return 1;
    }
};

/* We subclass RenderableGroupData because this global stuff always shares the same
 * node no matter what the material/mesh
 */
struct GlobalGroupData : public RenderableGroupData {
    std::experimental::optional<std::string> global_ambient_variable_;
};


class GlobalGroup : public RenderGroup {
public:
    typedef GlobalGroupData data_type;

    GlobalGroup(RenderGroup* parent, GlobalGroupData data):
        RenderGroup(parent),
        data_(data) {}

    void bind(GPUProgram* program);
    void unbind(GPUProgram* program) {}

private:
    GlobalGroupData data_;
};


struct DepthGroupData : public GroupData {
    bool depth_test;
    bool depth_write;

    DepthGroupData(bool depth_testing, bool depth_writes):
        depth_test(depth_testing),
        depth_write(depth_writes) {}

    std::size_t do_hash() const {
        size_t seed = 0;
        hash_combine(seed, depth_test);
        hash_combine(seed, depth_write);
        return seed;
    }
};

class DepthGroup : public RenderGroup {
public:
    typedef DepthGroupData data_type;

    DepthGroup(RenderGroup* parent, DepthGroupData data):
        RenderGroup(parent),
        data_(data) {}

    void bind(GPUProgram* program);
    void unbind(GPUProgram* program);

private:
    DepthGroupData data_;
};

struct ShaderGroupData : public GroupData {
    ShaderGroupData(GPUProgram::ptr program):
        shader_(program) {}

    GPUProgram::ptr shader_;

    std::size_t do_hash() const;
};

class ShaderGroup : public RenderGroup {
public:
    typedef ShaderGroupData data_type;

    ShaderGroup(RenderGroup* parent, ShaderGroupData data):
        RenderGroup(parent),
        data_(data) {

    }

    void bind(kglt::GPUProgram *program);
    void unbind(GPUProgram* program);

private:
    ShaderGroupData data_;
};


struct AutoAttributeGroupData : public GroupData {
    AutoAttributeGroupData(const std::map<std::string, ShaderAvailableAttributes>& attributes):
        enabled_attributes(attributes) {}

    std::map<std::string, ShaderAvailableAttributes> enabled_attributes;

    std::size_t do_hash() const {
        size_t seed = 0;
        hash_combine(seed, typeid(AutoAttributeGroupData).name());
        for(auto& p: enabled_attributes) {
            hash_combine(seed, p.first);
            hash_combine(seed, (int32_t) p.second);
        }
        return seed;
    }
};

class AutoAttributeGroup : public RenderGroup {
public:
    typedef AutoAttributeGroupData data_type;

    AutoAttributeGroup(RenderGroup* parent, AutoAttributeGroupData data):
        RenderGroup(parent),
        data_(data) {

    }

    void bind(kglt::GPUProgram *program);
    void unbind(GPUProgram* program);

private:
    AutoAttributeGroupData data_;
};

typedef std::map<std::string, float> FloatUniforms;
typedef std::map<std::string, int32_t> IntUniforms;

struct StagedUniformGroupData : public GroupData {
    FloatUniforms float_uniforms;
    IntUniforms int_uniforms;

    StagedUniformGroupData(FloatUniforms floats, IntUniforms ints):
        float_uniforms(floats), int_uniforms(ints) {}

    std::size_t do_hash() const {
        size_t seed;

        hash_combine(seed, typeid(StagedUniformGroupData).name());

        // std::map is ordered so this will work
        for(auto& p: float_uniforms) {
            hash_combine(seed, p.first);
            hash_combine(seed, p.second);
        }

        for(auto& p: int_uniforms) {
            hash_combine(seed, p.first);
            hash_combine(seed, p.second);
        }

        return seed;
    }
};

class StagedUniformGroup : public RenderGroup {
public:
    typedef StagedUniformGroupData data_type;

    StagedUniformGroup(RenderGroup* parent, StagedUniformGroupData data):
        RenderGroup(parent),
        data_(data) {}

    void bind(GPUProgram *program);
    void unbind(GPUProgram *program);

private:
    StagedUniformGroupData data_;
};


class RenderableGroup : public RenderGroup {
public:
    typedef RenderableGroupData data_type;
    RenderableGroup(RenderGroup* parent, RenderableGroupData data):
        RenderGroup(parent) {}

    void bind(GPUProgram *program) {}
    void unbind(GPUProgram* program) {}
};

struct MeshGroupData : public GroupData {
    MeshGroupData(MeshID id, SubMeshID smi):
        mesh_id(id),
        smi(smi) {}

    MeshID mesh_id;
    SubMeshID smi;

    std::size_t do_hash() const {
        size_t seed = 0;
        hash_combine(seed, typeid(MeshGroupData).name());
        hash_combine(seed, mesh_id.value());
        hash_combine(seed, smi);
        return seed;
    }
};

class InstancedMeshGroup : public RenderGroup {
public:
    typedef MeshGroupData data_type;

    InstancedMeshGroup(RenderGroup* parent, MeshGroupData data):
        RenderGroup(parent),
        data_(data) {

    }

    void bind(GPUProgram* program);
    void unbind(GPUProgram* program);

private:
    MeshGroupData data_;
};

struct MaterialGroupData : public GroupData {
    std::experimental::optional<std::string> ambient_variable;
    kglt::Colour ambient;

    std::experimental::optional<std::string> diffuse_variable;
    kglt::Colour diffuse;

    std::experimental::optional<std::string> specular_variable;
    kglt::Colour specular;

    std::experimental::optional<std::string> shininess_variable;
    float shininess;

    std::experimental::optional<std::string> active_texture_count_variable;
    int32_t active_texture_count;

    std::experimental::optional<std::string> point_size_variable;
    float point_size;

    std::size_t do_hash() const {
        size_t seed = 0;

        hash_combine(seed, ambient.r);
        hash_combine(seed, ambient.g);
        hash_combine(seed, ambient.b);
        hash_combine(seed, ambient.a);

        hash_combine(seed, diffuse.r);
        hash_combine(seed, diffuse.g);
        hash_combine(seed, diffuse.b);
        hash_combine(seed, diffuse.a);

        hash_combine(seed, specular.r);
        hash_combine(seed, specular.g);
        hash_combine(seed, specular.b);
        hash_combine(seed, specular.a);

        hash_combine(seed, shininess);
        hash_combine(seed, active_texture_count);
        hash_combine(seed, point_size);

        return seed;
    }
};

class MaterialGroup : public RenderGroup {
public:
    typedef MaterialGroupData data_type;

    MaterialGroup(RenderGroup* parent, MaterialGroupData data):
        RenderGroup(parent),
        data_(data) {

    }

    void bind(GPUProgram* program);
    void unbind(GPUProgram* program);

private:
    MaterialGroupData data_;
};

struct TextureGroupData : public GroupData {
    TextureGroupData(const std::vector<GLuint> texture_units){
        assert(texture_units.size() <= MAX_TEXTURE_UNITS);

        std::fill(textures, textures + MAX_TEXTURE_UNITS, 0);

        uint32_t i = 0;
        for(auto& unit: texture_units) {
            textures[i] = unit;
            ++i;
        }
    }

    GLuint textures[MAX_TEXTURE_UNITS] = {0};

    std::size_t do_hash() const {
        size_t seed = 0;
        hash_combine(seed, typeid(TextureGroupData).name());
        for(uint8_t i = 0; i < MAX_TEXTURE_UNITS; ++i) {
            hash_combine(seed, textures[i]);
        }

        return seed;
    }
};

class TextureGroup : public RenderGroup {
public:
    typedef TextureGroupData data_type;

    TextureGroup(RenderGroup* parent, TextureGroupData data):
        RenderGroup(parent),
        data_(data) {

    }

    void bind(GPUProgram* program);
    void unbind(GPUProgram* program);

private:
    TextureGroupData data_;
};

typedef std::pair<
    std::experimental::optional<std::string>,
    Mat4
> MatrixVariable;

struct TextureMatrixGroupData : public GroupData {
    TextureMatrixGroupData(const std::vector<MatrixVariable>& texture_matrices) {
        assert(texture_matrices.size() <= MAX_TEXTURE_MATRICES);

        uint8_t i = 0;
        for(auto& mat: texture_matrices) {
            texture_matrices_[i] = mat;
            ++i;
        }
    }

    MatrixVariable texture_matrices_[MAX_TEXTURE_MATRICES];

    std::size_t do_hash() const {
        size_t seed = 0;
        hash_combine(seed, typeid(TextureMatrixGroupData).name());
        for(uint8_t i = 0; i < MAX_TEXTURE_MATRICES; ++i) {
            Mat4 matrix;
            if(!texture_matrices_[i].first) {
                kmMat4Identity(&matrix);
            } else {
                matrix = texture_matrices_[i].second;
            }

            for(uint8_t i = 0; i < 16; ++i) {
                hash_combine(seed, matrix.mat[i]);
            }
        }
        return seed;
    }
};

class TextureMatrixGroup : public RenderGroup {
public:
    typedef TextureMatrixGroupData data_type;

    TextureMatrixGroup(RenderGroup* parent, TextureMatrixGroupData data):
        RenderGroup(parent),
        data_(data) {

    }

    void bind(GPUProgram* program);
    void unbind(GPUProgram* program);

private:
    TextureMatrixGroupData data_;
};

struct LightGroupData : public GroupData {
    LightID light_id;    

    std::experimental::optional<std::string> light_position_variable_;
    kglt::Vec4 light_position_value_;

    std::experimental::optional<std::string> light_ambient_variable_;
    kglt::Colour light_ambient_value_;

    std::experimental::optional<std::string> light_diffuse_variable_;
    kglt::Colour light_diffuse_value_;

    std::experimental::optional<std::string> light_specular_variable_;
    kglt::Colour light_specular_value_;

    std::experimental::optional<std::string> light_constant_attenuation_variable_;
    float light_constant_attenuation_value_;

    std::experimental::optional<std::string> light_linear_attenuation_variable_;
    float light_linear_attenuation_value_;

    std::experimental::optional<std::string> light_quadratic_attenuation_variable_;
    float light_quadratic_attenuation_value_;

    std::size_t do_hash() const {
        size_t seed = 0;
        hash_combine(seed, typeid(LightGroupData).name());
        hash_combine(seed, light_id);
        return seed;
    }
};

class LightGroup : public RenderGroup {
public:
    typedef LightGroupData data_type;

    LightGroup(RenderGroup* parent, LightGroupData data):
        RenderGroup(parent),
        data_(data) {

    }

    void bind(GPUProgram* program);
    void unbind(GPUProgram* program);

private:
    LightGroupData data_;
};


struct BlendGroupData : public GroupData {
    BlendGroupData(BlendType type):
        type(type) {}

    BlendType type;

    std::size_t do_hash() const {
        size_t seed = 0;
        hash_combine(seed, typeid(BlendGroupData).name());
        hash_combine(seed, (uint32_t) type);
        return seed;
    }
};

class BlendGroup : public RenderGroup {
public:
    typedef BlendGroupData data_type;

    BlendGroup(RenderGroup* parent, BlendGroupData data):
        RenderGroup(parent),
        data_(data) {}

    void bind(GPUProgram* program);
    void unbind(GPUProgram* program);

private:
    BlendGroupData data_;
};

struct RenderSettingsData : public GroupData {
    RenderSettingsData(float point_size, PolygonMode polygon_mode):
        point_size(point_size),
        polygon_mode(polygon_mode) {}

    float point_size;
    PolygonMode polygon_mode;

    std::size_t do_hash() const {
        size_t seed = 0;
        hash_combine(seed, typeid(RenderSettingsData).name());
        hash_combine(seed, point_size);
        hash_combine(seed, (uint32_t)polygon_mode);
        return seed;
    }
};

class RenderSettingsGroup : public RenderGroup {
public:
    typedef RenderSettingsData data_type;

    RenderSettingsGroup(RenderGroup* parent, RenderSettingsData data):
        RenderGroup(parent),
        data_(data) {}

    void bind(GPUProgram* program);
    void unbind(GPUProgram* program);

private:
    RenderSettingsData data_;
};

}

#endif // BATCHER_H_INCLUDED
