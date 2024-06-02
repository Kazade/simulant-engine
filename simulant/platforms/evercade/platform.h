#pragma once

#include "../linux/platform.h"

namespace smlt {

class EvercadePlatform : public LinuxPlatform
{
public:
    std::string name() const override { return "evercade"; }
};
}
