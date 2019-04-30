#pragma once

#include "../deps/kazsignal/kazsignal.h"
#include "../deps/kazlog/kazlog.h"

namespace smlt {

template<typename T>
class NotifiesDestruction {
    typedef sig::signal<void (T*)> destruction_signal;

    DEFINE_SIGNAL(destruction_signal, signal_destruction);

public:
    virtual ~NotifiesDestruction() {
        try {
            signal_destruction_((T*) this);
        } catch(...) {
            L_WARN(_F("Exception while signaling destruction of object {0} at {1}").format(
                typeid(T).name(), this
            ));
        }
    }
};

}
