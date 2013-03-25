#ifndef BATCHER_H_INCLUDED
#define BATCHER_H_INCLUDED

#include <list>
#include <tr1/unordered_map>
#include <tr1/memory>
#include "kglt/kazbase/exceptions.h"
#include "kglt/types.h"

namespace kglt {

template <class T>
inline void hash_combine(std::size_t& seed, const T& v)
{
    std::hash<T> hasher;
    seed ^= hasher(v) + 0x9e3779b9 + (seed<<6) + (seed>>2);
}

class ResourceManager;
class SubEntity;

struct GroupData {
    typedef std::tr1::shared_ptr<GroupData> ptr;

    virtual ~GroupData() {}
    virtual std::size_t hash() const = 0;
};

class RenderGroup {
public:
    typedef std::tr1::shared_ptr<RenderGroup> ptr;

    RenderGroup(RenderGroup* parent):
        parent_(parent) {

    }

    ///Traverses the tree and calls the callback on each subentity we encounter
    void traverse(std::function<void (SubEntity&)> callback) {
        bind();

        for(SubEntity* entity: subentities_) {
            callback(*entity);
        }

        for(std::pair<std::size_t, RenderGroups> groups: this->children_) {
            for(std::pair<std::size_t, std::tr1::shared_ptr<RenderGroup>> group: groups.second) {
                group.second->traverse(callback);
            }
        }

        unbind();
    }

    template<typename RenderGroupType>
    RenderGroup& get_or_create(const GroupData& data) {
        static_assert(std::is_base_of<RenderGroup, RenderGroupType>::value, "RenderGroupType must derive RenderGroup");

        size_t identifier = typeid(RenderGroupType).hash_code();

        RenderGroupChildren::iterator child_it = children_.find(identifier);
        if(child_it == children_.end()) {
            children_.insert(std::make_pair(identifier, RenderGroups()));
        }

        const typename RenderGroupType::data_type& cast_data = dynamic_cast<const typename RenderGroupType::data_type&>(data);

        RenderGroups& container = children_[identifier];
        RenderGroups::const_iterator it = container.find(data.hash());

        if(it != container.end()) {
            return *(*it).second;
        } else {
            std::tr1::shared_ptr<RenderGroupType> new_child(new RenderGroupType(this, cast_data));
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

    void add(SubEntity* subentity) {
        subentities_.push_back(subentity);
    }

    virtual void bind() = 0;
    virtual void unbind() = 0;

    virtual RenderGroup& get_root() {
        return parent_->get_root();
    }

protected:
    RenderGroup* parent_;

private:
    typedef std::tr1::unordered_map<std::size_t, std::tr1::shared_ptr<RenderGroup> > RenderGroups;
    typedef std::tr1::unordered_map<std::size_t, RenderGroups> RenderGroupChildren;

    RenderGroupChildren children_;

    std::list<SubEntity*> subentities_;
};

class RootGroup : public RenderGroup {
public:
    typedef std::tr1::shared_ptr<RootGroup> ptr;
    typedef int data_type;

    RootGroup(ResourceManager& resource_manager):
        RenderGroup(nullptr),
        resource_manager_(resource_manager) {}

    void bind() {}
    void unbind() {}

    RenderGroup& get_root() {
        return *this;
    }

    ResourceManager& resource_manager() { return resource_manager_; }

    void insert(SubEntity& ent, uint8_t pass_number);

private:
    ResourceManager& resource_manager_;
};


struct DepthGroupData : public GroupData {
    bool depth_test;
    bool depth_write;

    DepthGroupData(bool depth_testing, bool depth_writes):
        depth_test(depth_testing),
        depth_write(depth_writes) {}

    std::size_t hash() const {
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

    void bind();
    void unbind();

private:
    DepthGroupData data_;
};

struct ShaderGroupData : public GroupData {
    ShaderGroupData(ShaderID id):
        shader_id(id) {}

    ShaderID shader_id;

    std::size_t hash() const {
        size_t seed = 0;
        hash_combine(seed, typeid(ShaderGroupData).name());
        hash_combine(seed, shader_id.value());
        return seed;
    }
};

class ShaderGroup : public RenderGroup {
public:
    typedef ShaderGroupData data_type;

    ShaderGroup(RenderGroup* parent, ShaderGroupData data):
        RenderGroup(parent),
        data_(data) {

    }

    void bind();
    void unbind();

private:
    ShaderGroupData data_;
};

struct MeshGroupData : public GroupData {
    MeshGroupData(MeshID id, SubMeshIndex smi):
        mesh_id(id),
        smi(smi) {}

    MeshID mesh_id;
    SubMeshIndex smi;

    std::size_t hash() const {
        size_t seed = 0;
        hash_combine(seed, typeid(MeshGroupData).name());
        hash_combine(seed, mesh_id.value());
        hash_combine(seed, smi);
        return seed;
    }
};

class MeshGroup : public RenderGroup {
public:
    typedef MeshGroupData data_type;

    MeshGroup(RenderGroup* parent, MeshGroupData data):
        RenderGroup(parent),
        data_(data) {

    }

    void bind();
    void unbind();

private:
    MeshGroupData data_;
};

struct TextureGroupData : public GroupData {
    TextureGroupData(uint8_t texture_unit, TextureID id):
        unit(texture_unit),
        texture_id(id) {}

    uint8_t unit;
    TextureID texture_id;

    std::size_t hash() const {
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

    void bind();
    void unbind();

private:
    TextureGroupData data_;
};

struct TextureMatrixGroupData : public GroupData {
    TextureMatrixGroupData(uint8_t texture_unit, kmMat4 matrix):
        unit(texture_unit),
        matrix(matrix) {}

    uint8_t unit;
    kmMat4 matrix;

    std::size_t hash() const {
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

    void bind();
    void unbind();

private:
    TextureMatrixGroupData data_;
};



}

#endif // BATCHER_H_INCLUDED
