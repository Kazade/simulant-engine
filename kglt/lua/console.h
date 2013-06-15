#ifndef CONSOLE_H
#define CONSOLE_H

#include <tr1/memory>
#include "../generic/managed.h"
#include "../kazbase/unicode.h"
#include "types.h"
#include "../types.h"

namespace kglt {

class WindowBase;
class Interpreter;
struct KeyEvent;

class Console:
    public Managed<Console> {

public:
    Console(WindowBase& window);

    Interpreter& lua();

    bool entry(const kglt::KeyEvent& code);
private:
    UIStageID ui_stage_;
    CameraID ui_camera_;
    PipelineID ui_pipeline_;

    void init_widget();

    WindowBase& window_;

    std::vector<unicode> history_;
    std::vector<unicode> command_history_;

    unicode current_command_;

    std::shared_ptr<Interpreter> interpreter_;

    bool active_;

    void update_output();

    LuaResult execute(const unicode& command, unicode& output);
};

}

#endif // CONSOLE_H
