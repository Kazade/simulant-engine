#include <locale>
#include <functional>
#include <iostream>

#include "console.h"

#include "../window_base.h"
#include "../scene.h"
#include "../stage.h"
#include "../ui/interface.h"
#include "../input_controller.h"
#include "../procedural/geom_factory.h"
#include "../camera.h"
#include "../render_sequence.h"
#include "../ui_stage.h"

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

    GlobalKeyCallback cb = std::bind(&Console::key_down, this, std::placeholders::_1);
    TextInputCallback inp = std::bind(&Console::entry, this, std::placeholders::_1);
    window_.keyboard().key_pressed_connect(cb);
    window_.keyboard().text_input_connect(inp);

    export_lua_api(interpreter_->state());

    interpreter_->add_global("window", window_);
    interpreter_->add_global("scene", window_.scene());

    lua_register(interpreter_->state(), "print", print);
}

void Console::init_widget() {
    if(ui_stage_) {
        return;
    }

    Scene& scene = window_.scene();

    ui_stage_ = scene.new_ui_stage();
    ui_camera_ = scene.new_camera();

    scene.camera(ui_camera_).set_orthographic_projection(
        0, window_.width(), window_.height(), 0, -1, 1
    );

    ui_pipeline_ = scene.render_sequence().new_pipeline(
        ui_stage_, ui_camera_,
        ViewportID(), TextureID(), 101
    );

    ProtectedPtr<UIStage> stage = scene.ui_stage(ui_stage_);

    //If we can't find an element for the lua console
    if(stage->$("#lua-console").empty()) {
        //...then create one
        stage->append("<div>").id("lua-console");
        assert(!stage->$("#lua-console").empty());

        ui::ElementList l = stage->$("#lua-console");
        l.css("position", "absolute");
        l.css("width", "100%");
        l.css("height", "200px");
        l.css("background-color", "#00003388");
        l.css("color", "white");
        l.css("display", "block");
        l.css("white-space", "pre");
        l.css("padding-left", "2px");
        l.css("overflow-y", "scroll");
        l.css("overflow-x", "none");
        l.css("z-index", "100");
        l.css("font-family", "Ubuntu Mono");
        l.css("font-weight", "normal");
        l.hide();
    }
    update_output();
}

bool Console::key_down(SDL_Keysym key) {
    SDL_Scancode code = key.scancode;

    if(code == SDL_SCANCODE_GRAVE) {
        ProtectedPtr<UIStage> stage = window_.scene().ui_stage(ui_stage_);

        if(!active_) {
            init_widget(); //Make sure there is something to show!

            stage->$("#lua-console").show();
            active_ = true;
            SDL_StartTextInput();
            ignore_next_input_ = true; //Dirty hack to preven the GRAVE appearing in the text
        } else {
            stage->$("#lua-console").hide();
            active_ = false;
            SDL_StopTextInput();
        }
        return true;
    }

    if(code == SDL_SCANCODE_BACKSPACE) {
        unicode line = history_.at(history_.size() - 1);
        if(line.length() > 4) {
            line = line.slice(0, -1);
            history_.at(history_.size() - 1) = line;
        }
        update_output();
        return true;
    } else if (code == SDL_SCANCODE_RETURN) {
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
        update_output();
        return true;
    }

    return false;
}

void Console::entry(SDL_TextInputEvent event) {
    if(!active_) {
        return;
    }

    if(ignore_next_input_) {
        ignore_next_input_ = false;
        return;
    }

    unicode new_char = unicode(event.text);
    history_.at(history_.size() - 1) += new_char;

    update_output(); //Make sure the latest content is visible
}

void Console::update_output() {
    ProtectedPtr<UIStage> stage = window_.scene().ui_stage(ui_stage_);

    unicode past = _u("\n").join(history_);
    stage->$("#lua-console").text(past);
    stage->$("#lua-console").scroll_to_bottom();
}

LuaResult Console::execute(const unicode &command, unicode &output) {  
    std::streambuf* std_out;
    std_out = std::cout.rdbuf();

    std::ostringstream tmp_out;
    std::cout.rdbuf(tmp_out.rdbuf());

    LuaResult res = interpreter_->run_string(command.encode(), output);

    std::clog.flush();
    std::cout.flush();
    std::cerr.flush();

    std::cout.rdbuf(std_out);
    output += unicode(tmp_out.str());

    return res;
}

}
