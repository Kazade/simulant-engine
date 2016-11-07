#ifndef SIMULANT_RELATION_H
#define SIMULANT_RELATION_H


#include <cassert>
#include <map>
#include <set>
#include <stdexcept>
#include <vector>
#include <algorithm>

namespace smlt {

class RelationBase;

template<typename This, typename Remote>
class ReverseRelation;

/**
 * @brief The Relatable class
 *  A class that can contain fields that are subclasses of RelationBase
 */
class Relatable {
public:
    virtual ~Relatable(){}

    RelationBase* get_relation_field(const std::string& link_id) const {
        StringRelationBaseMap::const_iterator it = relation_fields_.find(link_id);
        if(it == relation_fields_.end()) {
            return nullptr;
        } else {
            return (*it).second;
        }
    }

private:
    void register_relation_field(RelationBase* field, const std::string& link_id) {
        relation_fields_[link_id] = field;
    }

    void unregister_relation_field(const std::string& link_id) {
        relation_fields_.erase(link_id);
    }


    typedef std::map<std::string, RelationBase*> StringRelationBaseMap;

    StringRelationBaseMap relation_fields_;

    friend class RelationBase;
};

class RelationBase {
public:
    RelationBase(Relatable* parent, const std::string& link_id):
        parent_(parent),
        link_id_(link_id) {

        parent_->register_relation_field(this, link_id_);
    }

    virtual ~RelationBase() {
        for(RelationBase* other: opposite_fields_) {
            if(other) {
                other->unset(parent_);
            }
        }

        parent_->unregister_relation_field(link_id_);
    }

    virtual void unset(Relatable* value) = 0;
protected:


    Relatable* parent_;
    std::string link_id_;

    std::vector<RelationBase*> opposite_fields_;

    void register_opposite_field(RelationBase* other) {
        opposite_fields_.push_back(other);
    }

    void unregister_opposite_field(RelationBase* other) {
        opposite_fields_.erase(std::remove(opposite_fields_.begin(), opposite_fields_.end(), other), opposite_fields_.end());
    }
};

template<typename This, typename Remote>
class Relation : public RelationBase {
public:
    /*
     *  The link_id identifies a Relation <-> ReverseRelation pair. By default we
     *  set this to a combination of the parent class, and the relation classes typeid
     *  name. This would limit you to one relation per-class, so the link_id is overridable
     */
    Relation(This* parent, const std::string& link_id=""):
        RelationBase(parent,
            (link_id.empty()) ?
                std::string(typeid(*parent).name()) + std::string(typeid(*this).name())
                       : link_id),
        related_(nullptr) {}

    Relation& operator=(const Remote& value) {
        //First unset any relation
        unset_remote();

        //Then set our related field and tell the reverserelation about it
        set_related((Remote*)&value);

        RelationBase* opposite = opposite_field();
        assert(opposite);

        auto other = dynamic_cast<ReverseRelation<Remote, This>*>(opposite);
        assert(other);

        other->add(dynamic_cast<This*>(parent_));

        return *this;
    }

    Remote& operator()() {
        Remote* result = get();
        if(!result) {
            throw std::logic_error("Unable to get related object");
        }

        return *result;
    }

    Remote* get() const {
        return dynamic_cast<Remote*>(related_);
    }

protected:
    void unset(Relatable*) {
        //We don't need to use the passed argument as we only
        //have one thing to unset
        unset_remote();
        set_related(nullptr);
    }

    void unset_remote() {
        if(related_) {
            auto other_field = dynamic_cast<ReverseRelation<Remote, This>*>(opposite_field());
            if(other_field) {
                other_field->unset(parent_);
            }
        }
    }

private:
    RelationBase* opposite_field() {
        return (opposite_fields_.empty()) ? nullptr : opposite_fields_.at(0);
    }

    void set_related(Relatable* related) {
        if(related_) {
            RelationBase* opposite = related_->get_relation_field(link_id_);
            assert(opposite);

            unregister_opposite_field(opposite);
        }
        related_ = related;
        if(related_) {
            RelationBase* opposite = related_->get_relation_field(link_id_);
            assert(opposite);

            register_opposite_field(opposite);
        }
    }

    Relatable* related_;
};

template<typename This, typename Remote>
class ReverseRelation : public RelationBase {
public:
    ReverseRelation(This* parent, const std::string& link_id=""):
        RelationBase(parent,
            (link_id.empty()) ?
                std::string(typeid(Remote).name()) + std::string(typeid(Relation<Remote, This>).name())
                       : link_id) {}

    std::vector<Remote*> all() const {
        return std::vector<Remote*>(related_.begin(), related_.end());
    }

    std::vector<Remote*> operator()() {
        return all();
    }

private:
    std::set<Remote*> related_;

    void add(Remote* related) {
        related_.insert(related);

        RelationBase* opposite = related->get_relation_field(link_id_);
        assert(opposite);
        register_opposite_field(opposite);
    }

    void unset(Relatable *value) {
        RelationBase* opposite = value->get_relation_field(link_id_);
        assert(opposite);
        unregister_opposite_field(opposite);

        related_.erase(dynamic_cast<Remote*>(value));
    }

    friend class Relation<Remote, This>;
};


}


#endif

/*

class B;

class A {
public:
    Relation<B> b;

    A():
        b(*this, "a_set") {

    }
};


class B : public Relatable {
public:
    ReverseRelation<A> a_set;

    B() {
      register_field(&a_set, "a_set");
    }

};


A a1;
A a2;

B b1;

a1.b.set(&b1);
a2.b.set(&b1);

b1.a_set.all(); -> [ a1*, a2* ]
*/
