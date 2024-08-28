#pragma once

#include "../generic/optional.h"
#include <string>

namespace smlt {

optional<std::string> base64_decode(const std::string& input);

}
