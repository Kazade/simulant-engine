#pragma once

#include <type_traits>
#include <functional>

namespace smlt {

template<typename Callable, typename Signature>
void check_signature() {
    static_assert(
        std::is_convertible<Callable&&, std::function<Signature>>::value,
        "Provided function has an invalid signature"
    );
}

}
