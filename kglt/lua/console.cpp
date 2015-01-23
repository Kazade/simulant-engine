#include <locale>
#include <functional>
#include <iostream>

#include "console.h"

#include "../window_base.h"
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


}

bool Console::init() {
    commands_.push_back("");
    current_line_type_ = LINE_TYPE_PROMPT;

    buffer_.push_back(std::make_pair(LINE_TYPE_RESULT, _u("KGLT {0} Console").format(LUA_RELEASE)));
    //buffer_.push_back(std::make_pair(LINE_TYPE_RESULT, _u("Type \"help\" for more information")));

    init_widget();

    GlobalKeyCallback cb = std::bind(&Console::key_down, this, std::placeholders::_1);
    TextInputCallback inp = std::bind(&Console::entry, this, std::placeholders::_1);
    window_.keyboard().key_pressed_connect(cb);
    window_.keyboard().text_input_connect(inp);

    export_lua_api(interpreter_->state());

    interpreter_->add_global("window", window_);

    lua_register(interpreter_->state(), "print", print);

    return true;
}

void Console::set_stats_fps(float fps) {
    UIStagePtr stage = window_.ui_stage(ui_stage_);
    auto elem = stage->$("#stats-fps");
    elem.text(_u("FPS: {0}").format(fps));
}

void Console::set_stats_subactors_rendered(int count) {
    UIStagePtr stage = window_.ui_stage(ui_stage_);
    auto elem = stage->$("#stats-subactors");
    elem.text(_u("Subactors: {0}").format(count));
}

void Console::init_widget() {
    if(ui_stage_) {
        return;
    }

    ui_stage_ = window_.new_ui_stage();
    ui_camera_ = window_.new_camera();

    window_.camera(ui_camera_)->set_orthographic_projection(
        0, window_.width(), window_.height(), 0, -1, 1
    );

    ui_pipeline_ = window_.render(ui_stage_, ui_camera_).with_priority(kglt::RENDER_PRIORITY_FOREGROUND);

    UIStagePtr stage = window_.ui_stage(ui_stage_);

    assert(stage);

    //If we can't find an element for the lua console
    if(stage->$("#lua-console").empty()) {
        //...then create one
        stage->append("<div>").id("lua-console");
        assert(!stage->$("#lua-console").empty());

        ui::ElementList l = stage->$("#lua-console");
        l.css("position", "absolute");
        l.css("width", "100%");
        l.css("height", _u("{0}px").format(window_.height() * 0.75).encode());
        l.css("background-color", "#000033BB");
        l.css("color", "white");
        l.css("display", "block");
        l.css("white-space", "pre");
        l.css("padding-left", "2px");
        l.css("overflow-y", "scroll");
        l.css("overflow-x", "hidden");
        l.css("z-index", "100");
        l.css("font-family", "Ubuntu Mono");
        l.css("font-weight", "normal");
        l.hide();

        // Append our stats div
        stage->append("<div>").id("stats");

        auto stats = stage->$("#stats");

        stats.css("width", "100px");
        stats.css("position", "absolute");
        stats.css("right", "0px");
        stats.css("top", "0px");
        stats.css("font-family", "Ubuntu Mono");
        stats.css("z-index", "90");
        stats.css("font-size", "1.2em");

        stats[0].append("<div>").id("stats-fps");
        stats[0].append("<div>").id("stats-subactors");

        set_stats_fps(0);
        set_stats_subactors_rendered(0);
    }
    update_output();
}

void Console::show_stats() {
    UIStagePtr stage = window_.ui_stage(ui_stage_);
    stage->$("#stats").show();
}

void Console::hide_stats() {
    UIStagePtr stage = window_.ui_stage(ui_stage_);
    stage->$("#stats").hide();
}

bool Console::key_down(SDL_Keysym key) {
    SDL_Scancode code = key.scancode;

    if(code == SDL_SCANCODE_GRAVE) {
        UIStagePtr stage = window_.ui_stage(ui_stage_);

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
        unicode line = commands_.at(command_being_edited_);
        if(line.length()) {
            line = line.slice(0, -1);
            commands_.at(command_being_edited_) = line;
            update_output();
        }        
        return true;
    } else if (code == SDL_SCANCODE_RETURN) {
        if(command_being_edited_ != commands_.size() - 1) {
            commands_.push_back(commands_.at(command_being_edited_));
        }

        unicode output;
        unicode command = commands_.at(command_being_edited_);
        LuaResult r = execute(command, output);

        buffer_.push_back(std::make_pair(current_line_type_, command));
        if(!command.empty()) {
            commands_.push_back(""); //Add a blank line for the current command
            command_being_edited_ = commands_.size() - 1;
        }

        if(r == LUA_RESULT_EOF) {
            current_line_type_ = LINE_TYPE_CONTINUATION;
        } else if(r == LUA_RESULT_SUCCESS || r == LUA_RESULT_ERROR) {
            for(unicode line: output.split("\n")) {
                buffer_.push_back(std::make_pair(LINE_TYPE_RESULT, line));
            }
            current_line_type_ = LINE_TYPE_PROMPT;
        } else {
            throw ValueError("Unknown result type");
        }
        update_output();
        return true;
    } else if(code == SDL_SCANCODE_UP) {
        if(command_being_edited_ > 0) {
            command_being_edited_ -= 1;
            update_output();
        }
        return true;
    } else if(code == SDL_SCANCODE_DOWN) {
        if(command_being_edited_ < commands_.size() - 1) {
            command_being_edited_ += 1;
            update_output();
        }
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
    commands_.at(command_being_edited_) += new_char;

    update_output(); //Make sure the latest content is visible
}

void Console::update_output() {
    UIStagePtr stage = window_.ui_stage(ui_stage_);

    std::vector<unicode> lines;
    for(auto p: buffer_) {
        if(p.first == LINE_TYPE_PROMPT) {
            lines.push_back(_u("{0}{1}").format(PROMPT_PREFIX, p.second));
        } else if(p.first == LINE_TYPE_CONTINUATION) {
            lines.push_back(_u("{0}{1}").format(CONTINUE_PREFIX, p.second));
        } else {
            lines.push_back(p.second);
        }
    }

    if(current_line_type_ == LINE_TYPE_PROMPT) {
        lines.push_back(_u("{0}{1}").format(PROMPT_PREFIX, commands_.at(command_being_edited_)));
    } else if(current_line_type_ == LINE_TYPE_CONTINUATION){
        lines.push_back(_u("{0}{1}").format(CONTINUE_PREFIX, commands_.at(command_being_edited_)));
    } else {
        lines.push_back(commands_.at(command_being_edited_));
    }

    unicode past = _u("\n").join(lines);
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
