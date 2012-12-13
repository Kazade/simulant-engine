
#include "material_script.h"
#include "kazbase/exceptions.h"
#include "kazbase/string_utils.h"

namespace kglt {

MaterialScript::MaterialScript(Scene& scene, const std::string& filename):
    scene_(scene),
    filename_(filename) {

}

void handle_block(const std::vector<std::string>& lines, uint16_t& current_line, const std::string& parent_block_type) {
    std::string line = str::strip(lines[current_line]);

    assert(line.startswith("BEGIN"));

    std::string block_type = str::rstrip(str::split(line, "(")[1], ")");

    if(block_type == "technique" && !parent_block_type.empty()) {
        throw SyntaxError("Line: " + boost::lexical_cast<std::string>(current_line) + ". Technique must be the top level block");
    }

    if(block_type == "pass" && parent_block_type != "technique") {
        throw SyntaxError("Line: " + boost::lexical_cast<std::string>(current_line) + ". Passes must be a child of a technique");
    }

    if(block_type == "fragment" && parent_block_type != "pass") {
        throw SyntaxError("Line: " + boost::lexical_cast<std::string>(current_line) + ". Fragment shaders must be a child of a pass");
    }

    if(block_type == "vertex" && parent_block_type != "pass") {
        throw SyntaxError("Line: " + boost::lexical_cast<std::string>(current_line) + ". Vertex shaders must be a child of a pass");
    }

    for(uint16_t i = current_line; current_line != lines.size(); ++i) {
        line = lines[current_line];

        //If we hit another BEGIN block, process it
        if(line.startswith("BEGIN")) {
            handle_block(lines, current_line, block_type);
        } else if(line.startswith("END")) {
            //If we hit an END block, the type must match the BEGIN
            std::string end_block_type = str::rstrip(str::split(line, "(")[1], ")");
            if(end_block_type != block_type) {
                throw SyntaxError(
                    "Line: " + boost::lexical_cast<std::string>(current_line) +
                            "Expected END(" + block_type + ") but found END(" + end_block_type + ")");
            }
            return; //Exit this function, we are done with this block
        } else if(line.startswith("SET")) {

        } else if(line.startswith("INCLUDE")) {

        }

        current_line++;
    }

}

MaterialID MaterialScript::generate() {
    std::ifstream file_in(filename_.c_str());
    if(!file_in.good()) {
        throw IOError("Couldn't open file: " + filename_);
    }

    std::string line;
    std::vector<std::string> lines;
    while(std::getline(file_in, line)) {
        line = str::strip(line); //Strip any indentation
        lines.push_back(line);
    }

    uint16_t current_line = 0;
    //Go through the lines in the file
    for(std::string line: lines) {
        if(str::startswith(line, "BEGIN")) {
            handle_block(lines, current_line, "");
        }
        current_line++;
    }

}

}
