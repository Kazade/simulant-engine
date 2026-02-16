#include <nxdk/mount.h>
#include <nxdk/path.h>

#include "platform.h"

namespace smlt {

XBOXPlatform::XBOXPlatform() {
    CHAR targetPath[MAX_PATH];
    nxGetCurrentXbeNtPath(targetPath);
    char *filenameStr = strrchr(targetPath, '\\');
    assert(filenameStr != NULL);
    *(filenameStr + 1) = '\0';

    // Mount the obtained path as D:
    auto success = nxMountDrive('D', targetPath);
    if(!success) {
        FATAL_ERROR(ERROR_CODE_FAILED_TO_MOUNT_FILESYSTEM, "Couldn't mount D:\\");
    }
}

uint64_t XBOXPlatform::available_ram_in_bytes() const {
    return MEMORY_VALUE_UNAVAILABLE;
}

} // namespace smlt
