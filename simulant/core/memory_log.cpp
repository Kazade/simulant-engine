#include "memory_log.h"
#include "../application.h"

namespace smlt {

std::unique_ptr<MemoryLogger> memory_logger;

MemoryLogger::MemoryLogger(const std::string& path):
    start_(std::chrono::steady_clock::now()) {

    /* "w" truncates/replaces any existing file at this path */
    file_ = fopen(path.c_str(), "w");
    if(file_) {
        fprintf(file_, "time_since_start,total_usage,event,type,id,source,name\n");
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
                        const std::string& source, const std::string& name) {
    if(!file_) {
        return;
    }

    auto elapsed = std::chrono::duration_cast<std::chrono::duration<double>>(
        std::chrono::steady_clock::now() - start_
    ).count();

    auto usage = get_app()->ram_usage_in_bytes();

    fprintf(file_, "%f,%lld,%s,%s,%llu,", elapsed, (long long)usage,
            (event == MEMORY_LOG_EVENT_ALLOC) ? "alloc" : "dealloc", type,
            (unsigned long long)id);
    write_csv_field(file_, source);
    fputc(',', file_);
    write_csv_field(file_, name);
    fputc('\n', file_);

    /* Flushed on every write so the log survives a crash caused by
     * running out of memory - which is exactly when this is needed most. */
    fflush(file_);
}

}
