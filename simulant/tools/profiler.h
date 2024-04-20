#pragma once

/** Usage:
 *
 *  void my_function() {
 *    S_PROFILE_SECTION("First");
 *    do_some_stuff();
 *    S_PROFILE_SECTION("Second");
 *    do_some_other_stuff();
 *    S_PROFILE_SUBSECTION("Nested");
 *    do_stuff_again();
 *    S_PROFILE_SECTION("Still Nested");
 *    do_stuff();
 *  }
 *
 *
 *  // Later
 *
 *  S_PROFILE_DUMP_TO_STDOUT();
 *
 *  ---
 *
 *  First                             XXus
 *  Second                            XXus
 *  Second / Nested                   XXus
 *  Second / Nested / Still Nested    XXus
 */

#include "../application.h"
#include "../time_keeper.h"

#include <cstdint>
#include <map>
#include <string>

class ProfileSection;

class ProfileRecord {
public:
    static ProfileRecord* get() {
        static ProfileRecord record;
        return &record;
    }

    void dump();

    void reset_counter() {
        counter_ = 0;
    }

private:
    friend class ProfileSection;

    void log(uint64_t ms, ProfileSection* which);

    struct Stats {
        std::size_t counter = 0;
        std::size_t count = 0;
        uint64_t nanoseconds = 0;
    };

    std::size_t counter_ = 0;
    std::map<std::string, Stats> times_;
};

class ProfileSection {
public:
    ProfileSection(const char* name, bool nest, const char* func_name,
                   const int line) :
        name_(name) {

        _S_UNUSED(func_name);
        _S_UNUSED(line);

        parent_ = current_parent;

        if(current) {
            if(nest) {
                current_parent = this;
            } else {
                // Finish any existing current section, if we're
                // not nesting (if we are nesting then we wait for the
                // destructor of the parent to finish */
                current->finish();
            }
        }

        current = this;

        start_ = smlt::get_app()->time_keeper->now_in_us();
    }

    ~ProfileSection() {
        finish();

        /* Sections are destroyed in reverse order, so if we're destroying
         * the "current" one then we must've finished the whole nested
         * section. Then any further ones within this subsection will be
         * destroyed (but this logic won't happen) */
        if(current == this) {
            current = current->parent_;
        }

        if(current_parent == this) {
            current_parent = parent_;
        }
    }

    const char* name() const {
        return name_;
    }

    ProfileSection* parent() const {
        return parent_;
    }

private:
    void finish() {
        if(is_finished_) {
            return;
        }
        auto end = smlt::get_app()->time_keeper->now_in_us();
        ProfileRecord::get()->log(end - start_, this);
        is_finished_ = true;
    }

    const char* name_;
    uint64_t start_ = 0;
    ProfileSection* parent_ = nullptr;
    static ProfileSection* current;
    static ProfileSection* current_parent;
    bool is_finished_ = false;
};

#define _S_CONCAT(a, b) _S_CONCAT_INNER(a, b)
#define _S_CONCAT_INNER(a, b) a##b
#define _S_UNIQUE_NAME(prefix) _S_CONCAT(prefix, __COUNTER__)

#define S_PROFILE_SECTION(name)                                                \
    auto _S_UNIQUE_NAME(_section) =                                            \
        ProfileSection((name), false, __FUNCTION__, __LINE__);

#define S_PROFILE_SUBSECTION(name)                                             \
    auto _S_UNIQUE_NAME(_section) =                                            \
        ProfileSection((name), true, __FUNCTION__, __LINE__);

/* Same as above, but switched based on profiling_enabled() (E.g. internal) */
#ifdef SIMULANT_PROFILE
#define _S_PROFILE_SECTION(name)                                               \
    S_PROFILE_SECTION(name)
#else
#define _S_PROFILE_SECTION(name)                                               \
    do {                                                                       \
    } while(0)
#endif

#ifdef SIMULANT_PROFILE
#define _S_PROFILE_SUBSECTION(name)                                            \
    S_PROFILE_SUBSECTION(name)
#else
#define _S_PROFILE_SUBSECTION(name)                                            \
    do {                                                                       \
    } while(0)
#endif

#define S_PROFILE_START_FRAME() ProfileRecord::get()->reset_counter()
#define S_PROFILE_DUMP_TO_STDOUT() ProfileRecord::get()->dump()
