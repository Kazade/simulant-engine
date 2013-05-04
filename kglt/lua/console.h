#ifndef CONSOLE_H
#define CONSOLE_H

#include <tr1/memory>
#include "../generic/managed.h"
#include "../kazbase/unicode.h"
#include "types.h"

namespace kglt {

class WindowBase;
class Interpreter;
struct KeyEvent;

class Console:
    public Managed<Console> {

public:
    Console(WindowBase& window);

    Interpreter& lua();

    void entry(const kglt::KeyEvent& code);

private:
    WindowBase& window_;

    std::vector<unicode> history_;
    unicode current_command_;

    std::shared_ptr<Interpreter> interpreter_;

    bool active_;

    void update_output();

    LuaResult execute(const unicode& command, unicode& output);
};

}

#endif // CONSOLE_H
