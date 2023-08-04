#pragma once

#include <cstdint>

namespace smlt {

class StageNode;

struct StageNodeTypeInfo {
    uint32_t type;
    std::size_t size_in_bytes;
    std::size_t alignment;
};

class StageNodeFactory {
private:
    template<typename T>
    static StageNode* standard_new(void* mem) {
        return new (mem) T();
    }

    template<typename T>
    static void standard_delete(void *mem) {
        ((T*) mem)->~T();
    }

public:
    template<typename T, typename... >
    T* create_node(Args&& args...) {
        auto params = typename T::params_type(std::forward(args)...);
        return create_node(T::node_id, &params);
    }

    template<typename T>
    void register_stage_node() {
        register_stage_node(
            typename T::node_type,
            sizeof(T), alignof(T),
            StageNodeFactory::standard_new<T>,
            StageNodeFactory::standard_delete<T>);
    }

    void register_stage_node(
        StageNodeType type,
        std::size_t size_in_bytes,
        std::size_t alignment,
        StageNode* (new_func*)(void*),
        void (delete_func*)(void*)) {

    }
};

}
