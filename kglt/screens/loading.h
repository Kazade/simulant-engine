#ifndef LOADING_H
#define LOADING_H

#include <vector>

#include "../generic/managed.h"
#include "../types.h"
#include "screen.h"

namespace kglt {


namespace screens {

class Loading:
    public Screen<Loading> {

public:
    Loading(WindowBase& window):
        Screen<Loading>(window, "loading") {}

private:
    void do_activate() override;
    void do_deactivate() override;

    void do_load() override;
    void do_unload() override;

    UIStageID stage_;
    CameraID camera_;
    PipelineID pipeline_;
};

}
}

#endif // LOADING_H
