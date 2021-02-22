
#include "errors.h"
#include "logging.h"

namespace smlt {
namespace _errors {

void log_critical_error(const std::string& msg) {
    S_ERROR(msg);
}

}
}
