#include <mach/mach.h>
#include <os/proc.h>

#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>

#include "platform.h"

namespace smlt {

std::string IOSPlatform::name() const {
    return "ios";
}

Resolution IOSPlatform::native_resolution() const {
    Resolution native;

    @autoreleasepool {
        UIScreen* screen = [UIScreen mainScreen];

        /* In pixels, and always as if the device were held upright */
        CGRect bounds = screen.nativeBounds;
        native.width = (uint16_t) bounds.size.width;
        native.height = (uint16_t) bounds.size.height;
        native.refresh_rate = (uint16_t) screen.maximumFramesPerSecond;
    }

    return native;
}

uint64_t IOSPlatform::total_ram_in_bytes() const {
    return [NSProcessInfo processInfo].physicalMemory;
}

uint64_t IOSPlatform::available_ram_in_bytes() const {
    /* How much more this app can use before iOS kills it, which is more
     * useful than what's free on the device */
    return os_proc_available_memory();
}

uint64_t IOSPlatform::process_ram_usage_in_bytes(ProcessID process_id) const {
    _S_UNUSED(process_id);

    /* What iOS counts against the app's memory limit */
    task_vm_info_data_t info;
    mach_msg_type_number_t count = TASK_VM_INFO_COUNT;
    if(task_info(mach_task_self(), TASK_VM_INFO, (task_info_t) &info, &count) != KERN_SUCCESS) {
        return MEMORY_VALUE_UNAVAILABLE;
    }

    return info.phys_footprint;
}

}
