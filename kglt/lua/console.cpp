#include <locale>
#include <functional>

#include "console.h"

#include "../window_base.h"
#include "../scene.h"
#include "../keyboard.h"
#include "../ui/interface.h"
#include "../input_controller.h"

namespace kglt {

const unicode PROMPT_PREFIX = _u(">>> ");
const unicode CONTINUE_PREFIX = _u("... ");

Console::Console(WindowBase &window):
    window_(window),
    interpreter_(Interpreter::create()),
    active_(false) {

    history_.push_back(_u("KGLT {0} Console").format(LUA_RELEASE));
    history_.push_back(_u("Type \"help\" for more information"));

    history_.push_back(PROMPT_PREFIX);

    init_widget();

    KeyCallback cb = std::bind(&Console::entry, this, std::placeholders::_1);
    window_.keyboard().key_pressed_connect(cb);

    interpreter_->register_class<WindowBase>();
    interpreter_->register_class<Scene>();

    interpreter_->add_global("window", window_);
}

void Console::init_widget() {
    //If we can't find an element for the lua console
    if(window_.ui()._("#lua-console").empty()) {
        //...then create one
        window_.ui().append("<div>").id("lua-console");
        assert(!window_.ui()._("#lua-console").empty());

        ui::ElementList l = window_.ui()._("#lua-console");
        l.css("position", "absolute");
        l.css("width", "100%");
        l.css("height", "100px");
        l.css("background-color", "#00000088");
        l.css("color", "white");
        l.css("display", "block");
        l.css("white-space", "pre");
        l.css("padding-left", "2px");
        l.css("overflow-y", "scroll");
        l.css("z-index", "100");
        l.css("font-family", "Ubuntu Mono");
        l.css("font-weight", "normal");
        l.hide();
    }
    update_output();
}

void Console::entry(const kglt::KeyEvent& event) {
    if(event.code == KEY_CODE_BACKQUOTE) {
        if(!active_) {
            init_widget(); //Make sure there is something to show!

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

    if(event.code == KEY_CODE_BACKSPACE) {
        unicode line = history_.at(history_.size() - 1);
        if(line.length() > 4) {
            line = line.slice(0, -1);
            history_.at(history_.size() - 1) = line;
        }
    } else if (event.code == KEY_CODE_RETURN) {
        current_command_ += history_.at(history_.size() - 1).slice(4, nullptr);
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
    } else if(std::isprint(event.code) && event.code != KEY_CODE_BACKQUOTE){
        unicode new_char = unicode(1, (char16_t) event.unicode);
        history_.at(history_.size() - 1) += new_char;
    }

    update_output(); //Make sure the latest content is visible
}

void Console::update_output() {
    unicode past = _u("\n").join(history_);
    window_.ui()._("#lua-console").text(past);
    window_.ui()._("#lua-console").scroll_to_bottom();
}

LuaResult Console::execute(const unicode &command, unicode &output) {
    LuaResult res = interpreter_->run_string(command.encode(), output);
    return res;
}

}
