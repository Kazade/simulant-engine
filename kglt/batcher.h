#ifndef BATCHER_H_INCLUDED
#define BATCHER_H_INCLUDED

#include <GLee.h>
#include <list>
#include <tr1/unordered_map>
#include "kglt/kazbase/exceptions.h"

namespace kglt {

class RenderGroup {
public:
    typedef std::tr1::shared_ptr<RenderGroup> ptr;

    RenderGroup(uint32_t id):
        id_(id) {

    }

    template<typename T>
    RenderGroup& get_or_create(uint32_t id) {
        RenderGroups::const_iterator it = children_[typeid(T).hash_code()].find(id);

        if(it != children_[typeid(T).hash_code()].end()) {
            return *(*it).second;
        } else {
            std::tr1::shared_ptr<RenderGroup> new_child(new T(id));
            children_[typeid(T).hash_code()].insert(std::make_pair(id, new_child));
            return *new_child;
        }
    }

    template<typename T>
    bool exists(uint32_t id) const {
        std::size_t hash = typeid(T).hash_code();

        RenderGroupChildren::const_iterator it = children_.find(hash);
        if(it == children_.end()) {
            return false;
        }

        return (*it).second.find(id) != (*it).second.end();
    }

    template<typename T>
    RenderGroup& get(uint32_t id) {
        RenderGroups::const_iterator it = children_[typeid(T).hash_code()].find(id);

        if(it != children_[typeid(T).hash_code()].end()) {
            return *(*it).second;
        } else {
            throw DoesNotExist<T>();
        }
    }

    void add(SubEntity* subentity) {
        subentities_.push_back(subentity);
    }

    virtual void bind() = 0;
    virtual void unbind() = 0;

    /*
    virtual RenderGroup& get_root() {
        return parent_->get_root();
    }*/

protected:
    uint32_t id_;

private:
    typedef std::tr1::unordered_map<uint32_t, std::tr1::shared_ptr<RenderGroup> > RenderGroups;
    typedef std::tr1::unordered_map<std::size_t, RenderGroups> RenderGroupChildren;

    RenderGroupChildren children_;

    std::list<SubEntity*> subentities_;
};

class RootGroup : public RenderGroup {
public:
    RootGroup():
        RenderGroup(0) {}

    void bind() {}
    void unbind() {}
};

class ShaderGroup : public RenderGroup {
public:
    ShaderGroup(uint32_t id): RenderGroup(id) {}

    void bind() {

    }

    void unbind() {

    }
};

class MeshGroup : public RenderGroup {};
class SubMeshGroup : public RenderGroup {};


class TextureRG0 : public RenderGroup {
public:
    TextureRG0(uint32_t id): RenderGroup(id) {}

    void bind() {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, id_);
    }

    void unbind() {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, 0);
    }
};

class TextureRG1 : public RenderGroup {
public:
    TextureRG1(uint32_t id): RenderGroup(id) {}

    void bind() {
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, id_);
    }

    void unbind() {
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, 0);
    }
};

}

#endif // BATCHER_H_INCLUDED
