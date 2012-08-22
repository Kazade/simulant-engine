#ifndef CREATOR_H
#define CREATOR_H

#include <tr1/memory>

namespace kglt {
namespace generic {

template<typename T>
class Creator {
public:
    static std::tr1::shared_ptr<T> create() {
        return std::tr1::shared_ptr<T>(new T());
    }

    template<typename U>
    static std::tr1::shared_ptr<T> create(U& u) {
        return std::tr1::shared_ptr<T>(new T(u));
    }
};

}

}

#endif // CREATOR_H
