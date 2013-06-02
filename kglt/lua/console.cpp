#include <locale>
#include <functional>

#include "console.h"

#include "../window_base.h"
#include "../scene.h"
#include "../stage.h"
#include "../keyboard.h"
#include "../ui/interface.h"
#include "../input_controller.h"
#include "../procedural/geom_factory.h"

#include "interpreter.h"
#include "api.h"

namespace kglt {

const unicode PROMPT_PREFIX = _u(">>> ");
const unicode CONTINUE_PREFIX = _u("... ");

int print(lua_State* L) {
    int arg_count = lua_gettop(L);
    lua_getglobal(L, "tostring");

    std::string output;

    for(int i = 1; i <= arg_count; ++i) {
        const char* s;
        lua_pushvalue(L, -1);
        lua_pushvalue(L, i);
        lua_call(L, 1, 1);
        s = lua_tostring(L, -1);
        if(s == NULL) {
            return luaL_error(L, LUA_QL("tostring") " must return a string to ", LUA_QL("print"));
        }
        if(i > 1) {
            output.append("\t");
        }

        output.append(s);
        lua_pop(L, 1);
    }
    output.append("\n");

    std::cout << output << std::endl;
    return 0;
}

Console::Console(WindowBase &window):
    window_(window),
    interpreter_(Interpreter::create()),
    active_(false) {

    history_.push_back(_u("KGLT {0} Console").format(LUA_RELEASE));
    history_.push_back(_u("Type \"help\" for more information"));

    history_.push_back(PROMPT_PREFIX);

    init_widget();

    GlobalKeyCallback cb = std::bind(&Console::entry, this, std::placeholders::_1);
    window_.keyboard().key_pressed_connect(cb);

    export_lua_api(interpreter_->state());

    interpreter_->add_global("window", window_);
    interpreter_->add_global("scene", window_.scene());

    lua_register(interpreter_->state(), "print", print);
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
        l.css("height", "200px");
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

bool Console::entry(const kglt::KeyEvent& event) {
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
        return false; //Allow key signals to propagate
    }

    if(event.code == KEY_CODE_BACKSPACE) {
        unicode line = history_.at(history_.size() - 1);
        if(line.length() > 4) {
            line = line.slice(0, -1);
            history_.at(history_.size() - 1) = line;
        }
    } else if (event.code == KEY_CODE_RETURN) {
        current_command_ += history_.at(history_.size() - 1).slice(4, nullptr);        
        command_history_.push_back(current_command_);

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
    } else {
        //Unhandled key press, we should pass this on to the code-specific signals
        return false;
    }

    update_output(); //Make sure the latest content is visible

    //We are active so don't propagate keyboard signals
    return true;
}

void Console::update_output() {
    unicode past = _u("\n").join(history_);
    window_.ui()._("#lua-console").text(past);
    window_.ui()._("#lua-console").scroll_to_bottom();
}

LuaResult Console::execute(const unicode &command, unicode &output) {  
    std::streambuf* stdout = std::cout.rdbuf();
    std::ostringstream tmp_out;
    std::cout.rdbuf(tmp_out.rdbuf());

    LuaResult res = interpreter_->run_string(command.encode(), output);

    std::clog.flush();
    std::cout.flush();
    std::cerr.flush();

    std::cout.rdbuf(stdout);
    output += unicode(tmp_out.str());

    return res;
}

}
