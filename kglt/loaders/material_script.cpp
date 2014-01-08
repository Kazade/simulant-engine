#include "material_script.h"
#include <kazbase/exceptions.h>
#include <kazbase/string.h>
#include <kazbase/unicode.h>
#include <kazbase/logging.h>
#include "../types.h"
#include "../material.h"
#include "../scene.h"
#include "../shortcuts.h"
#include "../resource_manager.h"

namespace kglt {

MaterialScript::MaterialScript(const unicode &filename):
    filename_(filename),
    text_(MaterialLanguageText("")){

}

MaterialScript::MaterialScript(const MaterialLanguageText& text):
    text_(text) {

}

void MaterialScript::handle_technique_set_command(Material& mat, const std::vector<std::string>& args, MaterialTechnique* technique) {
    if(args.size() < 2) {
        throw SyntaxError("Wrong number of arguments for SET command");
    }

    std::string type = str::upper(args[0]);
    throw SyntaxError("Invalid SET command for technique: " + type);
}

void MaterialScript::apply_staged_uniforms(ShaderProgram& program) {
    /*
     *  Because the material script will probably define the uniforms before the
     *  shader code is available, we need to stage any uniform params and apply them
     *  at the end of the pass block.
     */
    for(std::pair<std::string, int32_t> p: staged_integer_uniforms_) {
        program.params().set_int(p.first, p.second);
    }

    staged_integer_uniforms_.clear();
}

void MaterialScript::handle_pass_set_command(Material& mat, const std::vector<std::string>& args, MaterialPass* pass) {
    if(args.size() < 2) {
        throw SyntaxError("Wrong number of arguments for SET command");
    }
    std::string type = str::upper(args[0]);
    std::string arg_1 = str::upper(args[1]);

    if(type == "TEXTURE_UNIT") {
        TextureID tex_id = mat.resource_manager().new_texture_from_file(str::strip(args[1], "\""));
        pass->set_texture_unit(pass->texture_unit_count(), tex_id);
    } else if(type == "ITERATION") {
        if(arg_1 == "ONCE") {
            pass->set_iteration(ITERATE_ONCE, pass->max_iterations());
        } else if(arg_1 == "ONCE_PER_LIGHT") {
            pass->set_iteration(ITERATE_ONCE_PER_LIGHT, pass->max_iterations());
        } else {
            throw SyntaxError("Invalid argument to SET(ITERATION): " + args[1]);
        }
    } else if (type == "MAX_ITERATIONS") {
        int count = unicode(arg_1).to_int();
        pass->set_iteration(pass->iteration(), count);
    } else if(type == "ATTRIBUTE") {
        if(args.size() < 3) {
            throw SyntaxError("Wrong number of arguments for SET(ATTRIBUTE) command");
        }

        {
            ShaderPtr shader = mat.resource_manager().shader(pass->shader_id()).lock();
            std::string variable_name = str::strip(args[2], "\"");
            if(arg_1 == "POSITION") {
                shader->params().register_attribute(SP_ATTR_VERTEX_POSITION, variable_name);
            } else if(arg_1 == "TEXCOORD0") {
                shader->params().register_attribute(SP_ATTR_VERTEX_TEXCOORD0, variable_name);
            } else if(arg_1 == "TEXCOORD1") {
                shader->params().register_attribute(SP_ATTR_VERTEX_TEXCOORD1, variable_name);
            } else if(arg_1 == "TEXCOORD2") {
                shader->params().register_attribute(SP_ATTR_VERTEX_TEXCOORD2, variable_name);
            } else if(arg_1 == "TEXCOORD3") {
                shader->params().register_attribute(SP_ATTR_VERTEX_TEXCOORD3, variable_name);
            } else if(arg_1 == "NORMAL") {
                shader->params().register_attribute(SP_ATTR_VERTEX_NORMAL, variable_name);
            } else if(arg_1 == "DIFFUSE") {
                shader->params().register_attribute(SP_ATTR_VERTEX_DIFFUSE, variable_name);
            } else {
                throw SyntaxError("Unhandled attribute: " + arg_1);
            }
        }
    } else if(type == "UNIFORM") {
        if(args.size() < 4) {
            throw SyntaxError("Wrong number of arguments to SET(UNIFORM)");
        }

        std::string variable_name = str::strip(args[2], "\"");

        if(arg_1 == "INT") {
            int32_t value = 0;
            try {
                value = boost::lexical_cast<int>(args[3]);
            } catch(boost::bad_lexical_cast& e) {
                throw SyntaxError("Invalid INT value passed to SET(UNIFORM)");
            }

            stage_uniform(variable_name, value);
        } else {
            throw SyntaxError("Unhandled type passed to SET(UNIFORM)");
        }

    } else if(type == "AUTO_UNIFORM") {
        ShaderPtr shader = mat.resource_manager().shader(pass->shader_id()).lock();
        std::string variable_name = str::strip(args[2], "\"");

        if(arg_1 == "VIEW_MATRIX") {
            shader->params().register_auto(SP_AUTO_VIEW_MATRIX, variable_name);
        } else if(arg_1 == "MODELVIEW_MATRIX") {
            shader->params().register_auto(SP_AUTO_MODELVIEW_MATRIX, variable_name);
        } else if(arg_1 == "MODELVIEW_PROJECTION_MATRIX") {
            shader->params().register_auto(SP_AUTO_MODELVIEW_PROJECTION_MATRIX, variable_name);
        } else if(arg_1 == "INVERSE_TRANSPOSE_MODELVIEW_PROJECTION_MATRIX" || arg_1 == "NORMAL_MATRIX") {
            shader->params().register_auto(SP_AUTO_INVERSE_TRANSPOSE_MODELVIEW_MATRIX, variable_name);
        } else if(arg_1 == "TEXTURE_MATRIX0") {
            shader->params().register_auto(SP_AUTO_MATERIAL_TEX_MATRIX0, variable_name);
        } else if(arg_1 == "TEXTURE_MATRIX1") {
            shader->params().register_auto(SP_AUTO_MATERIAL_TEX_MATRIX1, variable_name);
        } else if(arg_1 == "TEXTURE_MATRIX2") {
            shader->params().register_auto(SP_AUTO_MATERIAL_TEX_MATRIX2, variable_name);
        } else if(arg_1 == "TEXTURE_MATRIX3") {
            shader->params().register_auto(SP_AUTO_MATERIAL_TEX_MATRIX3, variable_name);
        } else if(arg_1 == "LIGHT_GLOBAL_AMBIENT") {            
            shader->params().register_auto(SP_AUTO_LIGHT_GLOBAL_AMBIENT, variable_name);
        } else if(arg_1 == "LIGHT_POSITION") {
            shader->params().register_auto(SP_AUTO_LIGHT_POSITION, variable_name);
        } else if(arg_1 == "LIGHT_AMBIENT") {
            shader->params().register_auto(SP_AUTO_LIGHT_AMBIENT, variable_name);
        } else if(arg_1 == "LIGHT_DIFFUSE") {
            shader->params().register_auto(SP_AUTO_LIGHT_DIFFUSE, variable_name);
        } else if(arg_1 == "LIGHT_SPECULAR") {
            shader->params().register_auto(SP_AUTO_LIGHT_SPECULAR, variable_name);
        } else if(arg_1 == "LIGHT_CONSTANT_ATTENUATION") {
            shader->params().register_auto(SP_AUTO_LIGHT_CONSTANT_ATTENUATION, variable_name);
        } else if(arg_1 == "LIGHT_LINEAR_ATTENUATION") {
            shader->params().register_auto(SP_AUTO_LIGHT_LINEAR_ATTENUATION, variable_name);
        } else if(arg_1 == "LIGHT_QUADRATIC_ATTENUATION") {
            shader->params().register_auto(SP_AUTO_LIGHT_QUADRATIC_ATTENUATION, variable_name);
        } else if(arg_1 == "MATERIAL_SHININESS") {
            shader->params().register_auto(SP_AUTO_MATERIAL_SHININESS, variable_name);
        } else if(arg_1 == "MATERIAL_AMBIENT") {
            shader->params().register_auto(SP_AUTO_MATERIAL_AMBIENT, variable_name);
        } else if(arg_1 == "MATERIAL_DIFFUSE") {
            shader->params().register_auto(SP_AUTO_MATERIAL_DIFFUSE, variable_name);
        } else if(arg_1 == "MATERIAL_SPECULAR") {
            shader->params().register_auto(SP_AUTO_MATERIAL_SPECULAR, variable_name);
        } else if(arg_1 == "ACTIVE_TEXTURE_UNITS") {
            shader->params().register_auto(SP_AUTO_MATERIAL_ACTIVE_TEXTURE_UNITS, variable_name);
        } else {
            throw SyntaxError("Unhandled auto-uniform: " + arg_1);
        }
    } else if (type == "FLAG") {
        std::string arg_2 = str::upper(args[2]);

        if(arg_1 == "DEPTH_WRITE") {
            if(arg_2 == "ON") {
                pass->set_depth_write_enabled(true);
            } else {
                pass->set_depth_write_enabled(false);
            }
        } else if(arg_1 == "DEPTH_CHECK") {
            if(arg_2 == "ON") {
                pass->set_depth_test_enabled(true);
            } else {
                pass->set_depth_test_enabled(false);
            }
        } else if(arg_1 == "BLEND") {
            if(arg_2 == "NONE") {
                pass->set_blending(BLEND_NONE);
            } else if (arg_2 == "ADD") {
                pass->set_blending(BLEND_ADD);
            } else if (arg_2 == "MODULATE") {
                pass->set_blending(BLEND_MODULATE);
            } else if (arg_2 == "COLOUR") {
                pass->set_blending(BLEND_COLOUR);
            } else if (arg_2 == "ALPHA") {
                pass->set_blending(BLEND_ALPHA);
            } else if (arg_2 == "ONE_ONE_MINUS_ALPHA") {
                pass->set_blending(BLEND_ONE_ONE_MINUS_ALPHA);
            } else {
                throw SyntaxError("Invalid argument passed to SET(FLAG BLEND):" + arg_2);
            }
        } else {
            throw SyntaxError("Invalid argument passed to SET(FLAG): " + arg_1);
        }
    } else {
        throw SyntaxError("Invalid SET command for pass: " + type);
    }
}

void MaterialScript::handle_data_block(Material& mat, const std::string& data_type, const std::vector<std::string>& lines, MaterialPass* pass) {
    ShaderPtr shader = mat.resource_manager().shader(pass->shader_id()).lock();

    if(str::upper(data_type) == "VERTEX") {
        std::string source = str::join(lines, "\n");
        shader->add_and_compile(SHADER_TYPE_VERTEX, source);
    } else if(str::upper(data_type) == "FRAGMENT") {
        std::string source = str::join(lines, "\n");
        shader->add_and_compile(SHADER_TYPE_FRAGMENT, source);
    } else {
        throw SyntaxError("Invalid BEGIN_DATA block: " + data_type);
    }
}

void MaterialScript::handle_block(Material& mat,
        const std::vector<std::string>& lines,
        uint16_t& current_line,
        const std::string& parent_block_type,
        MaterialTechnique* current_technique, MaterialPass* current_pass) {

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
        uint32_t pass_number = current_technique->new_pass(mat.resource_manager().new_shader());
        current_pass = &current_technique->pass(pass_number);
    }

    for(uint16_t i = current_line; current_line != lines.size(); ++i) {
        line = lines[current_line];

        //If we hit another BEGIN block, process it
        if(str::starts_with(line, "BEGIN_DATA")) {
            //L_DEBUG("Found BEGIN_DATA block");
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
                handle_data_block(mat, data_type, data, current_pass);
            } else {
                throw SyntaxError(unicode("Line: {0}. Block does not accept BEGIN_DATA commands").format(current_line).encode());
            }
        } else if(str::starts_with(line, "BEGIN")) {
           // L_DEBUG("Found BEGIN block");
            handle_block(mat, lines, current_line, block_type, current_technique, current_pass);
        } else if(str::starts_with(line, "END")) {
           // L_DEBUG("Found END block");
            //If we hit an END block, the type must match the BEGIN
            std::string end_block_type = str::upper(str::strip(str::split(line, "(")[1], ")"));

            if(end_block_type != block_type) {
                throw SyntaxError(
                    "Line: " + boost::lexical_cast<std::string>(current_line) +
                            ". Expected END(" + block_type + ") but found END(" + end_block_type + ")");
            }

            if(end_block_type == "PASS") {
                ShaderPtr shader = mat.resource_manager().shader(current_pass->shader_id()).lock();

                //Apply any staged uniforms
                apply_staged_uniforms(*shader);

                //At the end of the pass, relink the shader
                shader->relink();
            }
            return; //Exit this function, we are done with this block
        } else if(str::starts_with(line, "SET")) {
         //   L_DEBUG("Found SET command block");
            std::string args_part = str::strip(str::split(line, "(")[1], ")");
            std::vector<std::string> args = str::split(args_part, " ");

            if(block_type == "TECHNIQUE") {
                handle_technique_set_command(mat, args, current_technique);
            } else if (block_type == "PASS") {
                handle_pass_set_command(mat, args, current_pass);
            } else {
                throw SyntaxError(unicode("Line: {0}. Block does not accept SET commands").format(current_line).encode());
            }

        }
        current_line++;
    }

}

void MaterialScript::generate(Material& material) {
    std::vector<std::string> lines;

    if(!filename_.empty()) {
        std::ifstream file_in(filename_.encode().c_str());
        if(!file_in.good()) {
            throw IOError("Couldn't open file: " + filename_.encode());
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
}

namespace loaders {

void MaterialScriptLoader::into(Loadable& resource, const LoaderOptions& options) {
    Material* mat = loadable_to<Material>(resource);
    parser_->generate(*mat);

    //FIXME: Need to handle resource manager deletion and material deletion
    std::shared_ptr<MaterialReloader> reloader(new MaterialReloader(
        mat->resource_manager(),
        mat->id())
    );

    mat->resource_manager().window().watcher().watch(
        filename_,
        std::bind(&MaterialReloader::reload, reloader, std::placeholders::_1, std::placeholders::_2)
    );
}

MaterialReloader::MaterialReloader(ResourceManager& rm, MaterialID material):
    rm_(rm),
    material_(material) {

}

void MaterialReloader::reload(const unicode& path, WatchEvent evt) {
    try {
        rm_.window().loader_for(path.encode())->into(rm_.material(material_));
    } catch(SyntaxError& e) {
        L_WARN("Unable to reload material as the syntax is incorrect");
    } catch(RuntimeError& e) {
        L_WARN("Unable to reload material as there was an error");
    }
}

}
}
