
/* Many platforms don't support exceptions, so we try to
 * return optional<T> or pair<T, bool> to indicate whether a function
 * succeeded or not.
 *
 * If it is not possible to recover, or an error occurs in a constructor/destructor
 * then we call FATAL_ERROR() which will terminate with a code and log a message
 */

#pragma once

#include <cstdlib>
#include <string>
#include "utils/formatter.h"

namespace smlt {

/* List of all fatal error codes, add to this list when you throw a new
 * error */
enum ErrorCode {
    ERROR_CODE_SUCCESS = 0,
    ERROR_CODE_MUTEX_INIT_FAILED,
    ERROR_CODE_THREAD_SPAWN_FAILED,
    ERROR_CODE_THREAD_JOIN_FAILED,
    ERROR_CODE_SDL_INIT_FAILED,
    ERROR_CODE_INVALID_TYPE_ERROR,
    ERROR_CODE_MISSING_ASSET_ERROR,
    ERROR_CODE_AUDIO_SUBSYSTEM_ERROR,
};

namespace _errors {

void log_critical_error(const std::string& msg);

}

}

#define FATAL_ERROR(code, msg) \
    do { \
        smlt::_errors::log_critical_error(_F("FATAL ERROR ({0}): {1}").format((int)code, msg)); \
        smlt::_errors::log_critical_error(_F(">>> {0}:{1}").format(__FILE__, __LINE__)); \
        ::exit((int) code); \
    } while(0)
