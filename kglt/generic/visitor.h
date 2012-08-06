#ifndef VISITOR_H
#define VISITOR_H

#include <cstddef>
#include <vector>

#include "loki/sequence.h"

namespace kglt {
namespace generic {

template<typename Base>
struct TagCounter {
    static size_t counter;
};

template<typename Base>
size_t TagCounter<Base>::counter;

template<typename Visitable, typename Base>
struct TagHolder {
    static size_t tag;
};

template<typename Visitable, typename Base>
size_t get_tag() {
    size_t& tag = TagHolder<const Visitable, const Base>::tag;
    if(tag == 0) {
        tag = ++TagCounter<const Base>::counter;
    }
    return tag;
}

template<typename Visitable, typename Base>
size_t TagHolder<Visitable, Base>::tag = get_tag<Visitable, Base>();

template<typename Base>
struct VisitableBase {
    template<typename Visitable>
    size_t get_tag_helper(const Visitable* /*this*/) const {
        return get_tag<Visitable, Base>();
    }
};

#define VIS_DEFINE_VISITABLE()          \
    virtual size_t tag() const {        \
        return get_tag_helper(this);    \
    }

template<typename Base, typename Func>
class VTable {
    std::vector<Func> table_;
public:
    template<typename Visitable>
    void add(Func f) {
        size_t index = get_tag<Visitable, Base>();
        if(index >= table_.size()) {
            const size_t base_tag = get_tag<Visitable, Base>();
            Func default_function = (base_tag >= table_.size()) ? 0 : table_[base_tag];
            table_.resize(index+1, default_function);
        }
        table_[index] = f;
    }

    Func operator[](size_t index) const {
        if(index >= table_.size()) {
            index = get_tag<Base, Base>();
        }
        return table_[index];
    }
};

template<typename Base, typename ReturnType=void>
struct Visitor {
    template<typename VisitorImpl, typename Visitable>
    ReturnType thunk(Base& b) {
        VisitorImpl& visitor = static_cast<VisitorImpl&>(*this);
        Visitable& visitable = static_cast<Visitable&>(b);
        //FIXME: if ReturnType is not void, this should return the result of visit
        VisitInvoker::invoke(visitor, visitable);
    }

    typedef ReturnType (Visitor::*Func) ( Base& );

    typedef VTable<const Base, Func> vtable_type;
    typedef Base base_type;

    const vtable_type* vtable_;

    ReturnType operator()(Base& b) {
        Func thunk = (*vtable_)[b.tag()];
        return (this->*thunk)(b);
    }    

    struct VisitInvoker {
        template<typename VisitorImpl, typename Visitable>
        static ReturnType invoke(VisitorImpl& visitor, Visitable& visitable) {
            /*check_member_function< ReturnType, VisitorImpl, Visitable >
               ( &Visitor::print ); // compile time assertion*/
            visitor.visit(visitable);
        }
    };
};



template<typename TypeList, typename Action>
void apply(const TypeList&, Action& action) {
    typename TypeList::Head head(nullptr);
    action(&head);
    apply(typename TypeList::Tail(), action);
}

template<typename Action>
void apply(Loki::NullType, Action&) {}


/*
template<typename Typelist, typename Action>
void apply(const Typelist&, Action& action) {
    typename Typelist::Head head;
    action(&head);
    apply(typename Typelist::Tail(), action);
}*/

template< typename Visitor, typename VisitedList >
struct CreateVtable {
    typename Visitor::vtable_type vtable_; /* vtable object */

    template< typename Visitable >
    void operator () ( const Visitable* /* hint */ ) {
        vtable_.template add<Visitable>(
            &Visitor::template thunk<Visitor, Visitable>
        );
    }

    CreateVtable() {
        // add Base's visit function first
        (*this) (static_cast<typename Visitor::base_type*>(0));

        // add visit function for each type in VisitedList
        apply(VisitedList(), *this);
    }
};

template< typename Visitor, typename VisitedList >
struct GetStaticVtable {
    // provide conversion operator
    operator const typename Visitor::vtable_type*() const {
        static CreateVtable<Visitor, VisitedList> table;
        return &table.vtable_;
    }
};

template<typename Visitor, typename VisitedList>
void Visits( Visitor& visitor, const VisitedList& ) {
    // instantiate the static vtable and set the vtable pointer
    visitor.vtable_ = GetStaticVtable<Visitor, VisitedList>();
}

}
}

#endif // VISITOR_H
