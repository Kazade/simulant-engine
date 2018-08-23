#ifndef PROFILER_H
#define PROFILER_H

/* Really basic heirarchial profiler. Usage:
 *
 * Profiler profiler("main");
 *
 * ... stuff ...
 *
 * profile.checkpoint("loading");
 *
 *
 * profile.checkpoint("rendering");
 *
 * {
 *     Profiler other("something");
 *
 *     other.checkpoint("something_else");
 * }
 *
 * profile.print_stats();
 *
 * OUTPUT:
 *
 * main.loading                                 0.15ms     100ms
 * main.rendering                               0.23ms     230ms
 *     something.something_else                 xxxx
 */

#include <string>
#include <map>
#include <deque>


namespace smlt {

struct ProfilerResult {
    unsigned long total_us = 0;
    unsigned long total_calls = 0;
};

class Profiler;

class RootProfiler {
public:
    RootProfiler() {}

    void print_stats();
    std::string generate_current_path();

private:
    std::deque<Profiler*> stack_;
    std::map<std::string, ProfilerResult> results_;

    friend class Profiler;
};


class Profiler {
public:
    Profiler(const std::string& name);
    ~Profiler();

    void checkpoint(const std::string& name);

    RootProfiler* get_root() const {
        return root_;
    }

    std::string name() const { return name_; }
private:
    static RootProfiler* root_;

    void init_root();

    std::string name_;
    uint64_t start_time_ = 0;
    uint64_t current_time_in_us() const;
};

}

#endif // PROFILER_H
