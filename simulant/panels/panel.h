#pragma once

namespace smlt {

class Panel {
public:
    virtual ~Panel() {}

    const bool is_active() const { return is_active_; }

    void activate() {
        if(is_active_) return;

        do_activate();
        is_active_ = true;
    }

    void deactivate() {
        if(!is_active_) return;

        do_deactivate();
        is_active_ = false;
    }

private:
    bool is_active_ = false;

    virtual void do_activate() = 0;
    virtual void do_deactivate() = 0;
};


}
