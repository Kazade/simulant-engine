#pragma once

#include <cstdint>
#include <functional>
#include <typeindex>

namespace smlt {

template<typename T>
constexpr const T &T_max(const T &a, const T &b) {
    return a > b ? a : b;
}

template<typename T, typename... Ts>
struct max_sizeof {
    static const size_t value = T_max(sizeof(T), max_sizeof<Ts...>::value);
};

template<typename T>
struct max_sizeof<T> {
    static const size_t value = sizeof(T);
};

template < typename Tp, typename... List >
struct contains_type : std::true_type {};

template < typename Tp, typename Head, typename... Rest >
struct contains_type<Tp, Head, Rest...>
: std::conditional< std::is_same<Tp, Head>::value,
    std::true_type,
    contains_type<Tp, Rest...>
>::type {};

template < typename Tp >
struct contains_type<Tp> : std::false_type {};

/* Dirty hacky trash-all-the-things variant because std:variant and std::any are too slow */
template<typename... Args>
struct FastVariant {
    typedef FastVariant<Args...> this_type;

    std::type_index type_code = typeid(bool);

    /* Allocate data to hold the largest type */
    char data[max_sizeof<Args...>::value];

    std::function<void (this_type*)> destroy;
    std::function<void (this_type*, const this_type*)> copy;

    FastVariant() {
        set<bool>(false);
    }

    FastVariant(const this_type& rhs) {
        if(rhs.copy) {
            rhs.copy(this, &rhs);
        }
    }

    this_type& operator=(const this_type& rhs) {
        if(rhs.copy) {
            rhs.copy(this, &rhs);
        }

        return *this;
    }

    ~FastVariant() {
        if(destroy) {
            destroy(this);
        }
    }

    template<typename T>
    const T& get() const {
        assert(std::type_index(typeid(T)) == type_code);
        auto ret = reinterpret_cast<const T*>(data);
        return *ret;
    }

    template<typename T>
    void set(const T& val) {
        static_assert(contains_type<T, Args...>::value, "Set invalid type on FastVariant");

        if(destroy) {
            destroy(this);
        }

        new (data) T(val);

        type_code = typeid(T);

        destroy = [](this_type* _this) {
            T* thing = (T*) _this->data;
            thing->~T();
        };

        copy = [](this_type* _this, const this_type* rhs) {
            _this->set(rhs->get<T>());
        };
    }
};


}
