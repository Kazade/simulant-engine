#include "material_script.h"
#include <kazbase/exceptions.h>
#include <kazbase/unicode.h>
#include <kazbase/logging.h>
#include "../types.h"
#include "../material.h"
#include "../shortcuts.h"
#include "../resource_manager.h"
#include "../utils/gl_thread_check.h"

namespace kglt {

MaterialScript::MaterialScript(std::shared_ptr<std::stringstream> data):
    text_(MaterialLanguageText(data->str())) {

}

MaterialScript::MaterialScript(const MaterialLanguageText& text):
    text_(text) {

}

void MaterialScript::handle_technique_set_command(Material& mat, const std::vector<std::string>& args, MaterialTechnique* technique) {
    if(args.size() < 2) {
        throw SyntaxError("Wrong number of arguments for SET command");
    }

    unicode type = unicode(args[0]).upper();
    throw SyntaxError(_u("Invalid SET command for technique: {0}").format(type));
}

void MaterialScript::handle_pass_set_command(Material& mat, const std::vector<unicode>& args, MaterialPass* pass) {
    if(args.size() < 2) {
        throw SyntaxError("Wrong number of arguments for SET command");
    }

    unicode type = unicode(args[0]).upper();
    unicode arg_1 = unicode(args[1]);

    if(type == "TEXTURE_UNIT") {
        TextureID tex_id = mat.resource_manager().new_texture_from_file(arg_1.strip("\""));
        pass->set_texture_unit(pass->texture_unit_count(), tex_id);
    } else if(type == "ITERATION") {
        if(arg_1 == "ONCE") {
            pass->set_iteration(ITERATE_ONCE, pass->max_iterations());
        } else if(arg_1 == "ONCE_PER_LIGHT") {
            pass->set_iteration(ITERATE_ONCE_PER_LIGHT, pass->max_iterations());
        } else {
            throw SyntaxError(_u("Invalid argument to SET(ITERATION): ") + args[1]);
        }
    } else if (type == "MAX_ITERATIONS") {
        int count = unicode(arg_1).to_int();
        pass->set_iteration(pass->iteration(), count);
    } else if(type == "ATTRIBUTE") {
        if(args.size() < 3) {
            throw SyntaxError("Wrong number of arguments for SET(ATTRIBUTE) command");
        }

        {
            auto shader = pass->program();

            std::string variable_name = unicode(args[2]).strip("\"").encode();
            if(arg_1 == "POSITION") {
                shader->attributes().register_auto(SP_ATTR_VERTEX_POSITION, variable_name);
            } else if(arg_1 == "TEXCOORD0") {
                shader->attributes().register_auto(SP_ATTR_VERTEX_TEXCOORD0, variable_name);
            } else if(arg_1 == "TEXCOORD1") {
                shader->attributes().register_auto(SP_ATTR_VERTEX_TEXCOORD1, variable_name);
            } else if(arg_1 == "TEXCOORD2") {
                shader->attributes().register_auto(SP_ATTR_VERTEX_TEXCOORD2, variable_name);
            } else if(arg_1 == "TEXCOORD3") {
                shader->attributes().register_auto(SP_ATTR_VERTEX_TEXCOORD3, variable_name);
            } else if(arg_1 == "NORMAL") {
                shader->attributes().register_auto(SP_ATTR_VERTEX_NORMAL, variable_name);
            } else if(arg_1 == "DIFFUSE") {
                shader->attributes().register_auto(SP_ATTR_VERTEX_DIFFUSE, variable_name);
            } else {
                throw SyntaxError(_u("Unhandled attribute: {0}").format(arg_1));
            }
        }
    } else if(type == "UNIFORM") {
        if(args.size() < 4) {
            throw SyntaxError("Wrong number of arguments to SET(UNIFORM)");
        }

        std::string variable_name = unicode(args[2]).strip("\"").encode();

        if(arg_1 == "INT") {
            int32_t value = 0;
            try {
                value = unicode(args[3]).to_int();
            } catch(std::exception& e) { //FIXME: Should be a specific exception
                throw SyntaxError("Invalid INT value passed to SET(UNIFORM)");
            }

            pass->stage_uniform(variable_name, value);
        } else {
            throw SyntaxError("Unhandled type passed to SET(UNIFORM)");
        }

    } else if(type == "AUTO_UNIFORM") {
        std::string variable_name = unicode(args[2]).strip("\"").encode();

        if(arg_1 == "VIEW_MATRIX") {
            pass->program()->uniforms().register_auto(SP_AUTO_VIEW_MATRIX, variable_name);
        } else if(arg_1 == "MODELVIEW_MATRIX") {
            pass->program()->uniforms().register_auto(SP_AUTO_MODELVIEW_MATRIX, variable_name);
        } else if(arg_1 == "MODELVIEW_PROJECTION_MATRIX") {
            pass->program()->uniforms().register_auto(SP_AUTO_MODELVIEW_PROJECTION_MATRIX, variable_name);
        } else if(arg_1 == "INVERSE_TRANSPOSE_MODELVIEW_PROJECTION_MATRIX" || arg_1 == "NORMAL_MATRIX") {
            pass->program()->uniforms().register_auto(SP_AUTO_INVERSE_TRANSPOSE_MODELVIEW_MATRIX, variable_name);
        } else if(arg_1 == "TEXTURE_MATRIX0") {
            pass->program()->uniforms().register_auto(SP_AUTO_MATERIAL_TEX_MATRIX0, variable_name);
        } else if(arg_1 == "TEXTURE_MATRIX1") {
            pass->program()->uniforms().register_auto(SP_AUTO_MATERIAL_TEX_MATRIX1, variable_name);
        } else if(arg_1 == "TEXTURE_MATRIX2") {
            pass->program()->uniforms().register_auto(SP_AUTO_MATERIAL_TEX_MATRIX2, variable_name);
        } else if(arg_1 == "TEXTURE_MATRIX3") {
            pass->program()->uniforms().register_auto(SP_AUTO_MATERIAL_TEX_MATRIX3, variable_name);
        } else if(arg_1 == "POINT_SIZE") {
            pass->program()->uniforms().register_auto(SP_AUTO_MATERIAL_POINT_SIZE, variable_name);
        } else if(arg_1 == "LIGHT_GLOBAL_AMBIENT") {            
            pass->program()->uniforms().register_auto(SP_AUTO_LIGHT_GLOBAL_AMBIENT, variable_name);
        } else if(arg_1 == "LIGHT_POSITION") {
            pass->program()->uniforms().register_auto(SP_AUTO_LIGHT_POSITION, variable_name);
        } else if(arg_1 == "LIGHT_AMBIENT") {
            pass->program()->uniforms().register_auto(SP_AUTO_LIGHT_AMBIENT, variable_name);
        } else if(arg_1 == "LIGHT_DIFFUSE") {
            pass->program()->uniforms().register_auto(SP_AUTO_LIGHT_DIFFUSE, variable_name);
        } else if(arg_1 == "LIGHT_SPECULAR") {
            pass->program()->uniforms().register_auto(SP_AUTO_LIGHT_SPECULAR, variable_name);
        } else if(arg_1 == "LIGHT_CONSTANT_ATTENUATION") {
            pass->program()->uniforms().register_auto(SP_AUTO_LIGHT_CONSTANT_ATTENUATION, variable_name);
        } else if(arg_1 == "LIGHT_LINEAR_ATTENUATION") {
            pass->program()->uniforms().register_auto(SP_AUTO_LIGHT_LINEAR_ATTENUATION, variable_name);
        } else if(arg_1 == "LIGHT_QUADRATIC_ATTENUATION") {
            pass->program()->uniforms().register_auto(SP_AUTO_LIGHT_QUADRATIC_ATTENUATION, variable_name);
        } else if(arg_1 == "MATERIAL_SHININESS") {
            pass->program()->uniforms().register_auto(SP_AUTO_MATERIAL_SHININESS, variable_name);
        } else if(arg_1 == "MATERIAL_AMBIENT") {
            pass->program()->uniforms().register_auto(SP_AUTO_MATERIAL_AMBIENT, variable_name);
        } else if(arg_1 == "MATERIAL_DIFFUSE") {
            pass->program()->uniforms().register_auto(SP_AUTO_MATERIAL_DIFFUSE, variable_name);
        } else if(arg_1 == "MATERIAL_SPECULAR") {
            pass->program()->uniforms().register_auto(SP_AUTO_MATERIAL_SPECULAR, variable_name);
        } else if(arg_1 == "ACTIVE_TEXTURE_UNITS") {
            pass->program()->uniforms().register_auto(SP_AUTO_MATERIAL_ACTIVE_TEXTURE_UNITS, variable_name);
        } else {
            throw SyntaxError(_u("Unhandled auto-uniform: {0}").format(arg_1));
        }
    } else if (type == "FLAG") {
        std::string arg_2 = unicode(args[2]).upper().encode();

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
            throw SyntaxError(_u("Invalid argument passed to SET(FLAG): {0}").format(arg_1));
        }
    } else {
        throw SyntaxError(_u("Invalid SET command for pass: {0}").format(type));
    }
}

void MaterialScript::handle_data_block(Material& mat, const unicode& data_type, const std::vector<unicode> &lines, MaterialPass* pass) {
    if(data_type.upper() == "VERTEX") {
        unicode source = _u("\n").join(lines);
        pass->program()->set_shader_source(SHADER_TYPE_VERTEX, source);
    } else if(data_type.upper() == "FRAGMENT") {
        unicode source = _u("\n").join(lines);
        pass->program()->set_shader_source(SHADER_TYPE_FRAGMENT, source);
    } else {
        throw SyntaxError(_u("Invalid BEGIN_DATA block: ") + data_type);
    }
}

void MaterialScript::handle_block(Material& mat,
        const std::vector<unicode>& lines,
        uint16_t& current_line,
        const unicode &parent_block_type,
        MaterialPass* current_pass) {

    unicode line = unicode(lines[current_line]).strip();
    current_line++;

    assert(unicode(line).starts_with("BEGIN"));

    std::vector<unicode> block_args = line.split("(")[1].strip(")").split(" ");
    unicode block_type = block_args[0].upper();

    const std::vector<unicode> VALID_BLOCKS = {
        "PASS"
    };

    if(!container::contains(VALID_BLOCKS, block_type)) {
        throw SyntaxError(_u("Line: {0}. Invalid block type: {1}").format(current_line, block_type));
    }

    if (block_type == "PASS") {
        if(!parent_block_type.empty()) {
            throw SyntaxError(_u("Line: {0}. Unexpected PASS block").format(current_line));
        }

        if(block_args.size() > 1) {
            throw SyntaxError(_u("Line: {0}. Wrong number of arguments.").format(current_line));
        }

        //Create the pass with the default shader
        uint32_t pass_number = mat.new_pass();
        current_pass = &mat.pass(pass_number);
    }

    for(uint16_t i = current_line; current_line != lines.size(); ++i) {
        line = lines[current_line];

        //If we hit another BEGIN block, process it
        if(line.starts_with("BEGIN_DATA")) {
            //L_DEBUG("Found BEGIN_DATA block");
            unicode data_type = line.split("(")[1].strip(")");

            //FIXME: Read all lines up to the end into a single string and then pass that
            std::vector<unicode> data;

            while(current_line < lines.size()) {
                current_line++;
                unicode new_line = lines[current_line];
                if(new_line.starts_with("END_DATA")) {
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
        } else if(line.starts_with("BEGIN")) {
           // L_DEBUG("Found BEGIN block");
            handle_block(mat, lines, current_line, block_type, current_pass);
        } else if(line.starts_with("END")) {
           // L_DEBUG("Found END block");
            //If we hit an END block, the type must match the BEGIN
            unicode end_block_type = line.split("(")[1].strip(")").upper();

            if(end_block_type != block_type) {
                throw SyntaxError(
                    _u("Line: {0}. Expected END({1}) but found END({2})").format(current_line, block_type, end_block_type));
            }

            if(end_block_type == "PASS") {
                //L_DEBUG(_u("Shader pass added {0}").format(mat.id()));
            }
            return; //Exit this function, we are done with this block
        } else if(line.starts_with("SET")) {
         //   L_DEBUG("Found SET command block");
            unicode args_part = line.split("(")[1].strip(")");
            std::vector<unicode> args = args_part.split(" ");

            if (block_type == "PASS") {
                handle_pass_set_command(mat, args, current_pass);
            } else {
                throw SyntaxError(unicode("Line: {0}. Block does not accept SET commands").format(current_line).encode());
            }

        }
        current_line++;
    }

}

void MaterialScript::generate(Material& material) {
    std::vector<unicode> lines;

    if(!text_.text().empty()) {
        lines = _u(text_.text()).split("\n");
        for(uint16_t i = 0; i < lines.size(); ++i) {
            lines[i] = lines[i].strip();
        }
    } else {
        throw std::logic_error("Text must be specified");
    }

    //FIXME: Parse INCLUDE commands and insert associated files into the lines

    uint16_t current_line = 0;
    //Go through the lines in the file
    while(current_line < lines.size()) {
        unicode line = lines[current_line];
        if(line.starts_with("BEGIN")) {
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
/*
    mat->resource_manager().window().watcher().watch(
        filename_,
        std::bind(&MaterialReloader::reload, reloader, std::placeholders::_1, std::placeholders::_2)
    );*/
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
