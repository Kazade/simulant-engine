#ifndef PATH_H
#define PATH_H

#include <vector>
#include "../../types.h"

namespace kglt {
namespace extra {

class Path {
public:
    Path(float radius=0.5);
    void add_point(const kglt::Vec3& point);
    bool empty() const;

    kglt::Vec3 point(uint32_t idx) const;
    uint32_t length() const;
    float radius() const { return radius_; }
private:
    float radius_;
    std::vector<kglt::Vec3> points_;
};

}
}
#endif // PATH_H
