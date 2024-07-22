#pragma once

#include "../types.h"
#include <vector>

namespace smlt {

/** An object that represents the path to a stage node
 *  in a node tree. Each element in the path is the unique
 *  node ID */
class StageNodePath {
public:
    template<typename... Parts>
    StageNodePath(Parts... parts) {
        set_path(path_, std::forward<Parts>(parts)...);
    }

    bool starts_with(const StageNodePath& other) const {
        for(std::size_t i = 0; i < other.path_.size(); ++i) {
            if(i >= path_.size()) {
                return false;
            } else if(path_[i] != other.path_[i]) {
                return false;
            }
        }

        return true;
    }

    bool operator!=(const StageNodePath& other) const {
        return !((*this) == other);
    }

    bool operator<(const StageNodePath& other) const {
        for(std::size_t i = 0; i < path_.size(); ++i) {
            if(i >= other.path_.size()) {
                // This is longer than other but all parts
                // up to this point matched, so not less
                return false;
            } else if(path_[i] > other.path_[i]) {
                return false;
            } else if(path_[i] < other.path_[i]) {
                return true;
            }
        }

        return path_.size() < other.path_.size();
    }

    bool operator==(const StageNodePath& other) const {
        return path_ == other.path_;
    }

    std::string to_string() const {
        std::string fpath = "";
        for(auto& part: path_) {
            fpath += std::to_string(part);
            fpath += "/";
        }

        if(!fpath.empty()) {
            fpath.pop_back();
        }

        return fpath;
    }

private:
    friend class StageNode;

    StageNodePath(const std::vector<StageNodeID>& parts) :
        path_(parts) {}

    template<typename... Parts>
    void set_path(std::vector<StageNodeID>& fpath, StageNodeID part,
                  Parts... parts) {
        fpath.push_back(part);
        set_path(fpath, std::forward<Parts>(parts)...);
    }

    void set_path(StageNodeID part) {
        path_.push_back(part);
    }

    std::vector<StageNodeID> path_;
};

std::ostream& operator<<(std::ostream& stream, const StageNodePath& path);

} // namespace smlt
