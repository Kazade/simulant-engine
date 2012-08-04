#ifndef VISITOR_H
#define VISITOR_H

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
            Func default_function = (base_tag >= table_.size) ? 0 : table_[base_tag];
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
class Visitor {
    template<typename VisitorImpl, typename Visitable>
    ReturnType thunk(Base& b) {
        VisitorImpl& visitor = static_cast<VisitorImpl&>(*this);
        Visitable& visitable = static_cast<Visitable&>(b);
        return visitor.visit(visitable);
    }

    typedef ReturnType (Visitor::*Func) ( Base& );

    const VTable<const Base, Func>* vtable_;

protected:
    ReturnType operator()(Base& b) {
        Func thunk = (*vtable_)[b.tag()];
        return (this->*thunk)(b);
    }
};



}
}

#endif // VISITOR_H
