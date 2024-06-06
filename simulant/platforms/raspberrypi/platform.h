#pragma once

#include "../linux/platform.h"

namespace smlt {

class RaspberryPiPlatform : public LinuxPlatform
{
public:
    std::string name() const override { return "raspberry-pi"; }
};
}
