#pragma once

#include <chrono>
#include <cstdint>
#include <cstdio>
#include <memory>
#include <string>

namespace smlt {

enum MemoryLogEvent {
    MEMORY_LOG_EVENT_ALLOC,
    MEMORY_LOG_EVENT_DEALLOC
};

/* Writes a CSV log of asset and stage node allocations/deallocations.
 * Only instantiated when AppConfig::Development::memory_log_path is set -
 * see log_memory_event() below for the zero-cost path taken when memory
 * logging is disabled. */
class MemoryLogger {
public:
    explicit MemoryLogger(const std::string& path);
    ~MemoryLogger();

    /* `id` is the AssetID/StageNodeID of the object involved - logged so
     * an alloc row can still be matched up with its corresponding dealloc
     * row. `size` is the approximate number of bytes of RAM the object's
     * data occupies (0 if unknown/not applicable, e.g. a dealloc event for
     * an object type that doesn't report a size). */
    void log(MemoryLogEvent event, const char* type, uint64_t id,
              uint64_t size, const std::string& source,
              const std::string& name);

private:
    FILE* file_ = nullptr;
    std::chrono::steady_clock::time_point start_;
};

/* Non-null only while memory logging is active (i.e. Application has
 * created a MemoryLogger because memory_log_path was set). Owned by
 * Application. */
extern std::unique_ptr<MemoryLogger> memory_logger;

/* Checking the pointer above is the only cost paid at each asset/node
 * alloc/dealloc when memory logging is disabled. */
inline void log_memory_event(MemoryLogEvent event, const char* type,
                              uint64_t id, uint64_t size,
                              const std::string& source,
                              const std::string& name) {
    if(memory_logger) {
        memory_logger->log(event, type, id, size, source, name);
    }
}

}
