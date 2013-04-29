#include <locale>
#include "window_base.h"
#include "keyboard.h"
#include "console.h"
#include "ui/interface.h"
#include "input_controller.h"

namespace kglt {

const unicode PROMPT_PREFIX = _u(">>> ");
const unicode CONTINUE_PREFIX = _u("... ");

Console::Console(WindowBase &window):
    window_(window),
    interpreter_(Interpreter::create()),
    active_(false) {

    history_.push_back(_u("KGLT Lua 5.1 Console"));
    history_.push_back(_u("Type \"help\" for more information"));

    history_.push_back(PROMPT_PREFIX);

    //If we can't find an element for the lua console
    if(window_.ui()._("#lua-console").empty()) {
        //...then create one
        window_.ui().append("<div>").id("lua-console");
        assert(!window_.ui()._("#lua-console").empty());

        ui::ElementList l = window_.ui()._("#lua-console");
        l.css("width", "100%");
        l.css("height", "25%");
        l.css("background-color", "#000000DD");
        l.css("color", "white");
        l.css("display", "block");
        l.css("white-space", "pre");
        l.css("padding-left", "2px");
        l.css("overflow-y", "scroll");
        l.css("z-index", "100");
        l.hide();
    }
    update_output();

    window_.keyboard().key_pressed_connect(std::bind(&Console::entry, this, std::tr1::placeholders::_1));
}

void Console::entry(KeyCode code) {
    if(code == KEY_CODE_BACKQUOTE) {
        if(!active_) {
            window_.ui()._("#lua-console").show();
            active_ = true;
        } else {
            window_.ui()._("#lua-console").hide();
            active_ = false;
        }
    }

    if(!active_) {
        return;
    }

    if(code == KEY_CODE_BACKSPACE) {
        unicode line = history_.at(history_.size() - 1);
        if(line.length() > 4) {
            line = line.slice(0, -1);
            history_.at(history_.size() - 1) = line;
        }
    } else if (code == KEY_CODE_RETURN) {
        unicode output;
        LuaResult r = execute(current_command_, output);
        if(r == LUA_RESULT_EOF) {
            history_.push_back(CONTINUE_PREFIX);
        } else if(r == LUA_RESULT_SUCCESS) {
            history_.push_back(output);
            history_.push_back(PROMPT_PREFIX);
            current_command_ = _u();
        } else if(r == LUA_RESULT_ERROR) {
            history_.push_back(output);
            history_.push_back(PROMPT_PREFIX);
            current_command_ = _u();
        }
    } else if(std::isprint(code) && code != KEY_CODE_BACKQUOTE){
        unicode new_char = _u("{0}").format(char(code));
        current_command_ += new_char;
        history_.at(history_.size() - 1) += new_char;
    }

    update_output(); //Make sure the latest content is visible
}

void Console::update_output() {
    unicode past = _u("\n").join(history_);
    window_.ui()._("#lua-console").text(past);
}

LuaResult Console::execute(const unicode &command, unicode &output) {
    std::string out;
    LuaResult res = interpreter_->run_string(command.encode(), out);
    output = _u(out);
    return res;
}

}
