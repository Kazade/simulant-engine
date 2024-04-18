#include <list>
#include <string>

#include "profiler.h"

ProfileSection* ProfileSection::current = nullptr;

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
        it->second.first++;
        it->second.second += ms;
    } else {
        times_.insert(std::make_pair(fin, std::make_pair(1, ms)));
    }
}

void ProfileRecord::dump() {
    for(auto& p: times_) {
        float t = double(p.second.second) / p.second.first;
        S_INFO("{0}\t{1}", p.first, t);
    }

    times_.clear();
}
