#pragma once

#include <functional>

namespace smlt {
namespace raii {

class Finally {
public:
    Finally(const std::function<void ()>& f):
        f_(f) {}

    ~Finally() {
        f_();
    }
private:
    std::function<void ()> f_;
};


}
}
