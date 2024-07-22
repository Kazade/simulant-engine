#include "stage_node_path.h"

#include <ostream>

namespace smlt {

std::ostream& operator<<(std::ostream& stream, const StageNodePath& path) {
    stream << path.to_string();
    return stream;
}

} // namespace smlt
