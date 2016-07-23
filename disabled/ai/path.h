#ifndef PATH_H
#define PATH_H

#include <vector>
#include "../../types.h"

namespace kglt {
namespace extra {

class Path {

public:
    Path(float radius=2.0, bool cyclic=true);
    void add_point(const kglt::Vec3& point);
    bool empty() const;

    kglt::Vec3 point(uint32_t idx) const;
    uint32_t length() const;
    float radius() const { return radius_; }
    bool cyclic() const { return cyclic_; }
private:
    float radius_;
    bool cyclic_;

    std::vector<kglt::Vec3> points_;
};

}
}
#endif // PATH_H
