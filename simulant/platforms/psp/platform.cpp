#include "platform.h"
#include <psputility_sysparam.h>
#include <pspsdk.h>
#include <cstdlib>

namespace smlt {

uint64_t PSPPlatform::available_ram_in_bytes() const {
    return pspSdkTotalFreeUserMemSize();
}

User PSPPlatform::current_user() const {
    char nickname[17] = {0};

    int result = sceUtilityGetSystemParamString(
        PSP_SYSTEMPARAM_ID_STRING_NICKNAME, nickname, sizeof(nickname)
    );
    const char* user = std::getenv("USER");
    unicode username = (result == PSP_SYSTEMPARAM_RETVAL_OK) ? unicode(nickname) : (user) ? user : "";
    return User{username};
}

} // namespace smlt
