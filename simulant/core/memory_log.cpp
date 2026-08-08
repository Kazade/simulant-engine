#include "memory_log.h"
#include "../application.h"

namespace smlt {

std::unique_ptr<MemoryLogger> memory_logger;

MemoryLogger::MemoryLogger(const std::string& path):
    start_(std::chrono::steady_clock::now()) {

    /* "w" truncates/replaces any existing file at this path */
    file_ = fopen(path.c_str(), "w");
    if(file_) {
        fprintf(file_, "time_since_start_ms,total_usage,event,type,id,size,source,name\n");
        fflush(file_);
    }
}

MemoryLogger::~MemoryLogger() {
    if(file_) {
        fclose(file_);
        file_ = nullptr;
    }
}

static void write_csv_field(FILE* file, const std::string& value) {
    if(value.find_first_of(",\"\n") == std::string::npos) {
        fputs(value.c_str(), file);
        return;
    }

    fputc('"', file);
    for(char c: value) {
        if(c == '"') {
            fputc('"', file);
        }
        fputc(c, file);
    }
    fputc('"', file);
}

void MemoryLogger::log(MemoryLogEvent event, const char* type, uint64_t id,
                        uint64_t size, const std::string& source,
                        const std::string& name) {
    if(!file_) {
        return;
    }

    auto elapsed_ms = std::chrono::duration_cast<std::chrono::duration<double, std::milli>>(
        std::chrono::steady_clock::now() - start_
    ).count();

    auto usage = get_app()->ram_usage_in_bytes();

    /* id is zero-padded to a fixed width so rows line up as plain text
     * (e.g. when eyeballing the file or diffing two runs) */
    fprintf(file_, "%.3f,%lld,%s,%s,%08llu,%llu,", elapsed_ms, (long long)usage,
            (event == MEMORY_LOG_EVENT_ALLOC) ? "alloc" : "dealloc", type,
            (unsigned long long)id, (unsigned long long)size);
    write_csv_field(file_, source);
    fputc(',', file_);
    write_csv_field(file_, name);
    fputc('\n', file_);

    /* Flushed on every write so the log survives a crash caused by
     * running out of memory - which is exactly when this is needed most. */
    fflush(file_);
}

}
