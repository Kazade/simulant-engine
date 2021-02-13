#pragma once

#include <istream>
#include <string>

namespace smlt {

/* On some platforms, std::getline is really slow. On Dreamcast
 * it can take several seconds when running over dc-load. I have
 * no idea why so if someone knows please file an issue! */
bool readline(std::istream& in, std::string& out);

}
