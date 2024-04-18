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

private:
    friend class ProfileSection;

    void log(uint64_t ms, ProfileSection* which);

    std::map<std::string, std::pair<std::size_t, uint64_t>> times_;
};

class ProfileSection {
public:
    ProfileSection(const char* name, bool nest, const char* func_name,
                   const int line) :
        name_(name) {

        _S_UNUSED(func_name);
        _S_UNUSED(line);

        if(current) {
            if(nest) {
                parent_ = current;
            } else {
                parent_ = current->parent_;

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
    bool is_finished_ = false;
};

#define S_PROFILE_SECTION(name)                                                \
    auto _section##__COUNTER__ =                                               \
        ProfileSection((name), false, __FUNCTION__, __LINE__);

#define S_PROFILE_SUBSECTION(name)                                             \
    auto _section##__COUNTER__ =                                               \
        ProfileSection((name), true, __FUNCTION__, __LINE__);

/* Same as above, but switched based on profiling_enabled() (E.g. internal) */
#ifndef NDEBUG
#define _S_PROFILE_SECTION(name)                                               \
    if(smlt::get_app()->profiling_enabled())                                   \
    S_PROFILE_SECTION(name)
#else
#define _S_PROFILE_SECTION(name)                                               \
    do {                                                                       \
    } while(0)
#endif

#ifndef NDEBUG
#define _S_PROFILE_SUBSECTION(name)                                            \
    if(smlt::get_app()->profiling_enabled())                                   \
    S_PROFILE_SUBSECTION(name)
#else
#define _S_PROFILE_SUBSECTION(name)                                            \
    do {                                                                       \
    } while(0)
#endif

#define S_PROFILE_DUMP_TO_STDOUT() ProfileRecord::get()->dump();
