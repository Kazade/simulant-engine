#include <list>
#include <string>

#include "profiler.h"

ProfileSection* ProfileSection::current = nullptr;
ProfileSection* ProfileSection::current_parent = nullptr;

void ProfileRecord::log(uint64_t ms, ProfileSection* which) {
    std::list<const char*> parts;
    while(which) {
        parts.push_front(which->name());
        which = which->parent();
    }

    std::string fin;
    std::size_t i = 0;
    for(auto& part: parts) {
        fin += part;
        if(++i != parts.size()) {
            fin += "/";
        }
    }

    auto it = times_.find(fin);
    if(it != times_.end()) {
        it->second.count++;
        it->second.nanoseconds += ms;
    } else {
        Stats new_stats;
        new_stats.counter = counter_++;
        new_stats.count = 1;
        new_stats.nanoseconds = ms;
        times_.insert(std::make_pair(fin, new_stats));
    }
}

void ProfileRecord::dump() {
    struct StatsWithName {
        std::string name;
        Stats stats;

        bool operator<(const StatsWithName& other) const {
            return stats.counter < other.stats.counter;
        }
    };

    std::set<StatsWithName> sorted_stats;

    std::size_t longest = 0;
    for(auto& p: times_) {
        StatsWithName s;
        s.name = p.first;
        s.stats = p.second;
        sorted_stats.insert(s);
        if(s.name.length() > longest) {
            longest = s.name.length();
        }
    }

    fprintf(stdout, "\nProfile Information\n\n");
    for(auto& s: sorted_stats) {
        double t = (double(s.stats.nanoseconds) / s.stats.count) / 1000.0;

        auto name = s.name;
        while(name.length() < longest) {
            name.push_back(' ');
        }

        fprintf(stdout, "%s\t\t\t\t%4fms\n", name.c_str(), t);
    }

    times_.clear();
}
