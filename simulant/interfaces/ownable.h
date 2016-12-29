#pragma once

namespace smlt {

/**
 * @brief The Owned class
 *
 * An interface that describes objects that are owned by a parent
 */
class Ownable {
public:
    virtual ~Ownable() {}
    virtual void ask_owner_for_destruction() = 0;
};

}
