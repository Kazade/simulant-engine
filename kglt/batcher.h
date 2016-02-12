#ifndef BATCHER_H_INCLUDED
#define BATCHER_H_INCLUDED

#include <list>
#include <unordered_map>
#include <memory>
#include <kazbase/exceptions.h>
#include "kglt/types.h"
#include "generic/auto_weakptr.h"

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
    void traverse(std::function<void (Renderable&, MaterialPass&)> callback) {
        bind(get_root().current_program());

        for(auto& p: renderables_) {
            assert(p.first);
            assert(p.second);

            callback(*p.first, *p.second);
        }

        for(std::pair<std::size_t, RenderGroups> groups: this->children_) {
            for(std::pair<std::size_t, std::shared_ptr<RenderGroup>> group: groups.second) {
                group.second->traverse(callback);
            }
        }

        unbind(get_root().current_program());
    }

    template<typename RenderGroupType>
    RenderGroup& get_or_create(const GroupData& data) {
        static_assert(std::is_base_of<RenderGroup, RenderGroupType>::value, "RenderGroupType must derive RenderGroup");

        size_t identifier = typeid(RenderGroupType).hash_code();

        auto& container = children_[identifier];

        const typename RenderGroupType::data_type& cast_data = static_cast<const typename RenderGroupType::data_type&>(data);

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

        std::size_t hash = typeid(RenderGroupType).hash_code();

        typename RenderGroupChildren::const_iterator it = children_.find(hash);
        if(it == children_.end()) {
            return false;
        }

        const RenderGroups& container = (*it).second;

        return container.find(data.hash()) != container.end();
    }

    template<typename RenderGroupType>
    RenderGroup& get(const GroupData& data) {
        static_assert(std::is_base_of<RenderGroup, RenderGroupType>::value, "RenderGroupType must derive RenderGroup");

        std::size_t hash = typeid(RenderGroupType).hash_code();

        typename RenderGroupChildren::const_iterator child_it = children_.find(hash);
        if(child_it == children_.end()) {
            throw DoesNotExist<RenderGroupType>();
        }

        const RenderGroups& container = (*child_it).second;

        RenderGroups::const_iterator it = container.find(data.hash());

        if(it != container.end()) {
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
        for(auto groups: children_) {
            for(auto group: groups.second) {
                group.second->clear();
            }
        }
        children_.clear();
    }

    void set_current_program(GPUProgram* program) { current_program_ = program; }
    GPUProgram* current_program() const { return current_program_; }

protected:
    RenderGroup* parent_;

private:
    typedef std::unordered_map<std::size_t, std::shared_ptr<RenderGroup> > RenderGroups;
    typedef std::unordered_map<std::size_t, RenderGroups> RenderGroupChildren;

    RenderGroupChildren children_;

    std::list<std::pair<Renderable*, MaterialPass*> > renderables_;

    GPUProgram* current_program_ = nullptr;
};

class RootGroup : public RenderGroup {
public:
    typedef std::shared_ptr<RootGroup> ptr;
    typedef int data_type;

    RootGroup(WindowBase& window, StageID stage, CameraID camera):
        RenderGroup(nullptr),
        window_(window),
        stage_id_(stage),
        camera_id_(camera){}

    void bind(GPUProgram* program);
    void unbind(GPUProgram* program) {}

    RenderGroup& get_root() {
        return *this;
    }

    StagePtr stage();
    ProtectedPtr<CameraProxy> camera();

    void insert(Renderable& ent, uint8_t pass_number, const std::vector<kglt::LightID> &lights);

private:
    WindowBase& window_;
    StageID stage_id_;
    CameraID camera_id_;

    void generate_mesh_groups(RenderGroup* parent, Renderable& ent, MaterialPass& pass, const std::vector<kglt::LightID> &lights);
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
    ShaderGroupData(GPUProgram* program):
        shader_(program) {}

    GPUProgram* shader_;

    std::size_t do_hash() const;
};

class ShaderGroup : public RenderGroup {
public:
    typedef ShaderGroupData data_type;

    ShaderGroup(RenderGroup* parent, ShaderGroupData data):
        RenderGroup(parent),
        data_(data) {

    }

    void bind(GPUProgram* program);
    void unbind(GPUProgram* program);

private:
    ShaderGroupData data_;
};

struct RenderableGroupData : public GroupData {
    std::size_t do_hash() const {
        return 1;
    }
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
    MaterialGroupData(
        const kglt::Colour& ambient,
        const kglt::Colour& diffuse,
        const kglt::Colour& specular,
        float shininess,
        int32_t active_texture_count,
        float point_size):
        ambient(ambient),
        diffuse(diffuse),
        specular(specular),
        shininess(shininess),
        active_texture_count(active_texture_count),
        point_size(point_size) {

    }

    kglt::Colour ambient;
    kglt::Colour diffuse;
    kglt::Colour specular;
    float shininess;
    int32_t active_texture_count;
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
    TextureGroupData(uint8_t texture_unit, TextureID id):
        unit(texture_unit),
        texture_id(id) {}

    uint8_t unit;
    TextureID texture_id;

    std::size_t do_hash() const {
        size_t seed = 0;
        hash_combine(seed, typeid(TextureGroupData).name());
        hash_combine(seed, unit);
        hash_combine(seed, texture_id.value());
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

struct TextureMatrixGroupData : public GroupData {
    TextureMatrixGroupData(uint8_t texture_unit, Mat4 matrix):
        unit(texture_unit),
        matrix(matrix) {}

    uint8_t unit;
    Mat4 matrix;

    std::size_t do_hash() const {
        size_t seed = 0;
        hash_combine(seed, typeid(TextureMatrixGroupData).name());
        hash_combine(seed, unit);
        for(uint8_t i = 0; i < 16; ++i) {
            hash_combine(seed, matrix.mat[i]);
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
    LightGroupData(LightID light_id):
        light_id(light_id) {}

    LightID light_id;

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
