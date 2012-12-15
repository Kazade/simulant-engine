
#include "material_script.h"
#include "kazbase/exceptions.h"
#include "kazbase/string.h"
#include "types.h"
#include "material.h"
#include "scene.h"
#include "shortcuts.h"

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

void MaterialScript::handle_technique_set_command(MaterialID new_material, const std::vector<std::string>& args, MaterialTechnique* technique) {
    if(args.size() < 2) {
        throw SyntaxError("Wrong number of arguments for SET command");
    }

    std::string type = str::upper(args[0]);
    throw SyntaxError("Invalid SET command for technique: " + type);
}

void MaterialScript::handle_pass_set_command(MaterialID new_material, const std::vector<std::string>& args, MaterialPass* pass) {
    if(args.size() < 2) {
        throw SyntaxError("Wrong number of arguments for SET command");
    }
    std::string type = str::upper(args[0]);
    if(type == "TEXTURE_UNIT") {
        TextureID tex_id = kglt::create_texture_from_file(scene_.window(), args[1]);
        pass->set_texture_unit(pass->texture_unit_count(), tex_id);
    } else {
        throw SyntaxError("Invalid SET command for pass: " + type);
    }
}

void MaterialScript::handle_data_block(MaterialID new_material, const std::string& data_type, const std::vector<std::string>& lines, MaterialPass* pass) {
    ShaderProgram& shader = scene_.shader(pass->shader());

    if(str::upper(data_type) == "VERTEX") {
        std::string source = str::join(lines, "\n");
        shader.add_and_compile(SHADER_TYPE_VERTEX, source);
    } else if(str::upper(data_type) == "FRAGMENT") {
        std::string source = str::join(lines, "\n");
        shader.add_and_compile(SHADER_TYPE_FRAGMENT, source);
    } else {
        throw SyntaxError("Invalid BEGIN_DATA block: " + data_type);
    }
}

void MaterialScript::handle_block(MaterialID new_material,
        const std::vector<std::string>& lines,
        uint16_t& current_line,
        const std::string& parent_block_type,
        MaterialTechnique* current_technique, MaterialPass* current_pass) {

    Material& mat = scene_.material(new_material);

    std::string line = str::strip(lines[current_line]);
    current_line++;

    assert(str::starts_with(line, "BEGIN"));

    std::vector<std::string> block_args = str::split(str::strip(str::split(line, "(")[1], ")"), " ");
    std::string block_type = str::upper(block_args[0]);

    const std::vector<std::string> VALID_BLOCKS = {
        "PASS",
        "TECHNIQUE"
    };

    if(!container::contains(VALID_BLOCKS, block_type)) {
        throw SyntaxError("Line: " + boost::lexical_cast<std::string>(current_line) + ". Invalid block type: " + block_type);
    }

    if(block_type == "TECHNIQUE") {
        if(!parent_block_type.empty()) {
            throw SyntaxError("Line: " + boost::lexical_cast<std::string>(current_line) + ". Technique must be the top level block");
        }

        if(block_args.size() != 2) {
            throw SyntaxError("Line: " + boost::lexical_cast<std::string>(current_line) + ". Techniques require a name argument");
        }

        std::string technique_name = str::strip(block_args[1], "\"");
        if(!mat.has_technique(technique_name)) {
            //Create the technique, strip quotes from the name
            mat.new_technique(technique_name);
        }
        current_technique = &mat.technique(technique_name);

    } else if (block_type == "PASS") {
        if(parent_block_type != "TECHNIQUE") {
            throw SyntaxError("Line: " + boost::lexical_cast<std::string>(current_line) + ". Passes must be a child of a technique");
        }

        if(block_args.size() > 1) {
            throw SyntaxError("Line: " + boost::lexical_cast<std::string>(current_line) + ". Wrong number of arguments");
        }

        assert(current_technique); //Shouldn't happen, should be caught by parent check above

        //Create the pass with the default shader
        uint32_t pass_number = current_technique->new_pass(scene_.new_shader());
        current_pass = &current_technique->pass(pass_number);
    }

    for(uint16_t i = current_line; current_line != lines.size(); ++i) {
        line = lines[current_line];

        //If we hit another BEGIN block, process it
        if(str::starts_with(line, "BEGIN_DATA")) {
            L_DEBUG("Found BEGIN_DATA block");
            std::string data_type = str::strip(str::split(line, "(")[1], ")");

            //FIXME: Read all lines up to the end into a single string and then pass that
            std::vector<std::string> data;

            while(current_line < lines.size()) {
                current_line++;
                std::string new_line = lines[current_line];
                if(str::starts_with(new_line, "END_DATA")) {
                    //FIXME: Check END_DATA type is the same as the start
                    break;
                } else {
                    data.push_back(new_line);
                }
            }

            if(block_type == "PASS") {
                handle_data_block(new_material, data_type, data, current_pass);
            } else {
                throw SyntaxError("Line: " + current_line + std::string(". Block does not accept BEGIN_DATA commands"));
            }
        } else if(str::starts_with(line, "BEGIN")) {
            L_DEBUG("Found BEGIN block");
            handle_block(new_material, lines, current_line, block_type, current_technique, current_pass);
        } else if(str::starts_with(line, "END")) {
            L_DEBUG("Found END block");
            //If we hit an END block, the type must match the BEGIN
            std::string end_block_type = str::upper(str::strip(str::split(line, "(")[1], ")"));

            if(end_block_type != block_type) {
                throw SyntaxError(
                    "Line: " + boost::lexical_cast<std::string>(current_line) +
                            ". Expected END(" + block_type + ") but found END(" + end_block_type + ")");
            }

            if(end_block_type == "PASS") {
                //At the end of the pass, relink the shader
                scene_.shader(current_pass->shader()).relink();
            }
            return; //Exit this function, we are done with this block
        } else if(str::starts_with(line, "SET")) {
            L_DEBUG("Found SET command block");
            std::string args_part = str::strip(str::split(line, "(")[1], ")");
            std::vector<std::string> args = str::split(args_part, " ");

            if(block_type == "TECHNIQUE") {
                handle_technique_set_command(new_material, args, current_technique);
            } else if (block_type == "PASS") {
                handle_pass_set_command(new_material, args, current_pass);
            } else {
                throw SyntaxError("Line: " + current_line + std::string(". Block does not accept SET commands"));
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
            for(uint16_t i = 0; i < lines.size(); ++i) {
                lines[i] = str::strip(lines[i]);
            }
        } else {
            throw std::logic_error("Filename or text must be specified");
        }

        //FIXME: Parse INCLUDE commands and insert associated files into the lines

        uint16_t current_line = 0;
        //Go through the lines in the file
        while(current_line < lines.size()) {
            std::string line = lines[current_line];
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
