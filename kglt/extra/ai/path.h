#ifndef PATH_H
#define PATH_H

#include <vector>
#include "../../types.h"

namespace kglt {
namespace extra {

class Path {
public:
    Path();
    void add_point(const kglt::Vec3& point);
    bool empty() const;

    kglt::Vec3 point(uint32_t idx) const;
    uint32_t length() const;
private:
    std::vector<kglt::Vec3> points_;
};

}
}
#endif // PATH_H
