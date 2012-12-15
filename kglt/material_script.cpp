
#include "material_script.h"
#include "kazbase/exceptions.h"
#include "kazbase/string.h"
#include "types.h"
#include "material.h"
#include "scene.h"

namespace kglt {

MaterialScript::MaterialScript(Scene& scene, const std::string& filename):
    scene_(scene),
    filename_(filename),
    text_(MaterialLanguageText("")){

}

MaterialScript::MaterialScript(Scene& scene, const MaterialLanguageText& text):
    scene_(scene),
    text_(text) {

}

void MaterialScript::handle_technique_set_command(MaterialID new_material, const std::vector<std::string>& args) {

}

void MaterialScript::handle_pass_set_command(MaterialID new_material, const std::vector<std::string>& args) {

}

void MaterialScript::handle_block(
        MaterialID new_material,
        const std::vector<std::string>& lines,
        uint16_t& current_line,
        const std::string& parent_block_type) {

    Material& mat = scene_.material(new_material);

    std::string line = str::strip(lines[current_line]);

    assert(str::starts_with(line, "BEGIN"));

    std::vector<std::string> block_args = str::split(str::strip(str::split(line, "(")[1], ")"), " ");
    std::string block_type = block_args[0];

    if(block_type == "TECHNIQUE") {
        if(!parent_block_type.empty()) {
            throw SyntaxError("Line: " + boost::lexical_cast<std::string>(current_line) + ". Technique must be the top level block");
        }

        if(block_args.size() != 2) {
            throw SyntaxError("Line: " + boost::lexical_cast<std::string>(current_line) + ". Techniques require a name argument");
        }

        //Create the technique, strip quotes from the name
        mat.new_technique(str::strip(block_args[1], "\""));
    }

    if(block_type == "PASS" && parent_block_type != "TECHNIQUE") {
        throw SyntaxError("Line: " + boost::lexical_cast<std::string>(current_line) + ". Passes must be a child of a technique");
    }

    for(uint16_t i = current_line; current_line != lines.size(); ++i) {
        line = lines[current_line];

        //If we hit another BEGIN block, process it
        if(str::starts_with(line, "BEGIN")) {
            handle_block(new_material, lines, current_line, block_type);
        } else if(str::starts_with(line, "END")) {
            //If we hit an END block, the type must match the BEGIN
            std::string end_block_type = str::strip(str::split(line, "(")[1], ")");
            if(end_block_type != block_type) {
                throw SyntaxError(
                    "Line: " + boost::lexical_cast<std::string>(current_line) +
                            "Expected END(" + block_type + ") but found END(" + end_block_type + ")");
            }
            return; //Exit this function, we are done with this block
        } else if(str::starts_with(line, "SET")) {
            std::string args_part = str::strip(str::split(line, "(")[1], ")");
            std::vector<std::string> args = str::split(args_part, " ");

            if(block_type == "TECHNIQUE") {
                handle_technique_set_command(new_material, args);
            } else if (block_type == "PASS") {
                handle_pass_set_command(new_material, args);
            } else {
                throw SyntaxError("Line: " + current_line + std::string(". Block does not accept SET commands"));
            }

        } else if(str::starts_with(line, "BEGIN_DATA")) {
            std::string data_type = str::strip(str::split(line, "(")[1], ")");

            //FIXME: Read all lines up to the end into a single string and then pass that
            std::string data;
            if(block_type == "PASS") {
                handle_data_block(new_material, data_type, data);
            } else {
                throw SyntaxError("Line: " + current_line + std::string(". Block does not accept BEGIN_DATA commands"));
            }
        }

        current_line++;
    }

}

MaterialID MaterialScript::generate() {
    std::vector<std::string> lines;

    MaterialID material = scene_.new_material();
    try {
        if(!filename_.empty()) {
            std::ifstream file_in(filename_.c_str());
            if(!file_in.good()) {
                throw IOError("Couldn't open file: " + filename_);
            }

            std::string line;
            while(std::getline(file_in, line)) {
                line = str::strip(line); //Strip any indentation
                lines.push_back(line);
            }
        } else if (!text_.text().empty()) {
            lines = str::split(text_.text(), "\n");
        } else {
            throw std::logic_error("Filename or text must be specified");
        }

        //FIXME: Parse INCLUDE commands and insert associated files into the lines

        uint16_t current_line = 0;
        //Go through the lines in the file
        for(std::string line: lines) {
            if(str::starts_with(line, "BEGIN")) {
                handle_block(material, lines, current_line, "");
            }
            current_line++;
        }
    } catch(...) {
        //Delete the material if we couldn't load the file
        scene_.delete_material(material);
        throw;
    }

    return material;
}

}
