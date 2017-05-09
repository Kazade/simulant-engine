#pragma once

namespace utils {

namespace static_if_detail {
    template<bool Condition>
    struct statement {
        template<typename Function>
        void then(const Function& f) {
            f();
        }

        template<typename Function>
        void otherwise(const Function&) {}
    };

    template<>
    struct statement<false> {
        template<typename Function>
        void then(const Function&) {}

        template<typename Function>
        void otherwise(const Function& f) {
            f();
        }
    };
}

template<bool Condition, typename Function>
static_if_detail::statement<Condition> static_if(const Function& f) {
    static_if_detail::statement<Condition> if_;
    if_.then(f);
    return if_;
}

}
