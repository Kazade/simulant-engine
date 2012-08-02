#ifndef VISITOR_H
#define VISITOR_H

namespace kglt {
namespace generic {

template<typename Derived>
class Visitable {
public:
    template<typename T>
    void accept(T& visitor) {
        visitor.visit(static_cast<Derived&>(*this));
    }
};

class BaseVisitor {
public:
    virtual ~BaseVisitor() {}
};

template<typename T>
class Visitor {
public:
    virtual void visit(T& /* visitable */) = 0;
};

}
}

#endif // VISITOR_H
