#ifndef CREATOR_H
#define CREATOR_H

#include <memory>

namespace kglt {
namespace generic {

template<typename T>
class Creator {
public:
    static std::shared_ptr<T> create() {
        return std::shared_ptr<T>(new T());
    }

    template<typename U>
    static std::shared_ptr<T> create(U& u) {
        return std::shared_ptr<T>(new T(u));
    }
};

}

}

#endif // CREATOR_H
