#ifndef CLONEABLE_H
#define CLONEABLE_H

namespace kglt {

template<typename IDType>
class Cloneable {
public:
    virtual ~Cloneable() {}

    IDType clone() {
        return do_clone();
    }

private:
    virtual IDType do_clone() = 0;
};

}

#endif // CLONABLE_H
