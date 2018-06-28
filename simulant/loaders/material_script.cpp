//
//   Copyright (c) 2011-2017 Luke Benstead https://simulant-engine.appspot.com
//
//     This file is part of Simulant.
//
//     Simulant is free software: you can redistribute it and/or modify
//     it under the terms of the GNU General Public License as published by
//     the Free Software Foundation, either version 3 of the License, or
//     (at your option) any later version.
//
//     Simulant is distributed in the hope that it will be useful,
//     but WITHOUT ANY WARRANTY; without even the implied warranty of
//     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//     GNU General Public License for more details.
//
//     You should have received a copy of the GNU General Public License
//     along with Simulant.  If not, see <http://www.gnu.org/licenses/>.
//

#include "material_script.h"
#include "../deps/kazlog/kazlog.h"
#include "../types.h"
#include "../material.h"
#include "../shortcuts.h"
#include "../resource_manager.h"
#include "../utils/gl_thread_check.h"
#include "../renderers/renderer.h"

#ifndef SIMULANT_GL_VERSION_1X
#include "../renderers/gl4x/gpu_program.h"
#endif

namespace smlt {

MaterialScript::MaterialScript(std::shared_ptr<std::istream> data):
    text_(MaterialLanguageText(
        std::string((std::istreambuf_iterator<char>(*data)), std::istreambuf_iterator<char>())
    )) {

}

MaterialScript::MaterialScript(const MaterialLanguageText& text):
    text_(text) {

}

void MaterialScript::handle_header_property_command(Material& mat, const std::vector<unicode> &args) {
    if(args.size() < 2) {
        throw SyntaxError("Wrong number of arguments for PROPERTY command");
    }

    unicode type = unicode(args[0]).upper();
    bool has_value = args.size() == 3;

    // PROPERTY(INT varname [value])
    // PROPERTY(FLOAT varname [value])

    if(args.size() < 3) {
        throw SyntaxError("Wrong number of arguments to PROPERTY()");
    }

    std::string variable_name = unicode(args[1]).strip("\"").encode();

    if(type == "INT") {

        // Create an integer property on the material
        mat.create_int_property(variable_name);

        if(has_value) {
            /* If we have a value, then set it - otherwise just create an uninitialized property with this name */
            int32_t value = 0;
            try {
                value = unicode(args[2]).to_int();
                mat.set_int_property(variable_name, value);
            } catch(std::exception& e) { //FIXME: Should be a specific exception
                throw SyntaxError("Invalid INT value passed to PROPERTY");
            }
        }
    } else {
        throw SyntaxError("Unhandled type passed to PROPERTY");
    }
}

void MaterialScript::handle_pass_set_command(Material& mat, const std::vector<unicode>& args, MaterialPass::ptr pass) {
    if(args.size() < 2) {
        throw SyntaxError("Wrong number of arguments for SET command");
    }

    Renderer* renderer = mat.resource_manager().window->renderer;

    unicode type = unicode(args[0]).upper();
    unicode arg_1 = unicode(args[1]);

    if(type == "TEXTURE_UNIT") {
        unicode arg_2 = unicode(args[2]);

        TextureID tex_id = mat.resource_manager().new_texture_from_file(arg_2.strip("\""));
        pass->set_texture_unit(arg_1.to_int(), tex_id);

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

        if(!renderer->supports_gpu_programs()) {
            throw SyntaxError("ATTRIBUTE commands are not supported on this renderer");
        }

        std::string variable_name = unicode(args[2]).strip("\"").encode();
        if(arg_1 == "POSITION") {
            pass->attributes->register_auto(SP_ATTR_VERTEX_POSITION, variable_name);
        } else if(arg_1 == "TEXCOORD0") {
            pass->attributes->register_auto(SP_ATTR_VERTEX_TEXCOORD0, variable_name);
        } else if(arg_1 == "TEXCOORD1") {
            pass->attributes->register_auto(SP_ATTR_VERTEX_TEXCOORD1, variable_name);
        } else if(arg_1 == "TEXCOORD2") {
            pass->attributes->register_auto(SP_ATTR_VERTEX_TEXCOORD2, variable_name);
        } else if(arg_1 == "TEXCOORD3") {
            pass->attributes->register_auto(SP_ATTR_VERTEX_TEXCOORD3, variable_name);
        } else if(arg_1 == "NORMAL") {
            pass->attributes->register_auto(SP_ATTR_VERTEX_NORMAL, variable_name);
        } else if(arg_1 == "DIFFUSE") {
            pass->attributes->register_auto(SP_ATTR_VERTEX_DIFFUSE, variable_name);
        } else {
            throw SyntaxError(_u("Unhandled attribute: {0}").format(arg_1));
        }

    }  else if(type == "UNIFORM") {
        std::string variable_name = unicode(args[2]).strip("\"").encode();

        if(!renderer->supports_gpu_programs()) {
            throw SyntaxError("UNIFORM commands are not supported on this renderer");
        }

        if(arg_1 == "VIEW_MATRIX") {
            pass->uniforms->register_auto(SP_AUTO_VIEW_MATRIX, variable_name);
        } else if(arg_1 == "MODELVIEW_MATRIX") {
            pass->uniforms->register_auto(SP_AUTO_MODELVIEW_MATRIX, variable_name);
        } else if(arg_1 == "MODELVIEW_PROJECTION_MATRIX") {
            pass->uniforms->register_auto(SP_AUTO_MODELVIEW_PROJECTION_MATRIX, variable_name);
        } else if(arg_1 == "INVERSE_TRANSPOSE_MODELVIEW_PROJECTION_MATRIX" || arg_1 == "NORMAL_MATRIX") {
            pass->uniforms->register_auto(SP_AUTO_INVERSE_TRANSPOSE_MODELVIEW_MATRIX, variable_name);
        } else if(arg_1 == "TEXTURE_MATRIX0") {
            pass->uniforms->register_auto(SP_AUTO_MATERIAL_TEX_MATRIX0, variable_name);
        } else if(arg_1 == "TEXTURE_MATRIX1") {
            pass->uniforms->register_auto(SP_AUTO_MATERIAL_TEX_MATRIX1, variable_name);
        } else if(arg_1 == "TEXTURE_MATRIX2") {
            pass->uniforms->register_auto(SP_AUTO_MATERIAL_TEX_MATRIX2, variable_name);
        } else if(arg_1 == "TEXTURE_MATRIX3") {
            pass->uniforms->register_auto(SP_AUTO_MATERIAL_TEX_MATRIX3, variable_name);
        } else if(arg_1 == "POINT_SIZE") {
            pass->uniforms->register_auto(SP_AUTO_MATERIAL_POINT_SIZE, variable_name);
        } else if(arg_1 == "LIGHT_GLOBAL_AMBIENT") {            
            pass->uniforms->register_auto(SP_AUTO_LIGHT_GLOBAL_AMBIENT, variable_name);
        } else if(arg_1 == "LIGHT_POSITION") {
            pass->uniforms->register_auto(SP_AUTO_LIGHT_POSITION, variable_name);
        } else if(arg_1 == "LIGHT_AMBIENT") {
            pass->uniforms->register_auto(SP_AUTO_LIGHT_AMBIENT, variable_name);
        } else if(arg_1 == "LIGHT_DIFFUSE") {
            pass->uniforms->register_auto(SP_AUTO_LIGHT_DIFFUSE, variable_name);
        } else if(arg_1 == "LIGHT_SPECULAR") {
            pass->uniforms->register_auto(SP_AUTO_LIGHT_SPECULAR, variable_name);
        } else if(arg_1 == "LIGHT_CONSTANT_ATTENUATION") {
            pass->uniforms->register_auto(SP_AUTO_LIGHT_CONSTANT_ATTENUATION, variable_name);
        } else if(arg_1 == "LIGHT_LINEAR_ATTENUATION") {
            pass->uniforms->register_auto(SP_AUTO_LIGHT_LINEAR_ATTENUATION, variable_name);
        } else if(arg_1 == "LIGHT_QUADRATIC_ATTENUATION") {
            pass->uniforms->register_auto(SP_AUTO_LIGHT_QUADRATIC_ATTENUATION, variable_name);
        } else if(arg_1 == "MATERIAL_SHININESS") {
            pass->uniforms->register_auto(SP_AUTO_MATERIAL_SHININESS, variable_name);
        } else if(arg_1 == "MATERIAL_AMBIENT") {
            pass->uniforms->register_auto(SP_AUTO_MATERIAL_AMBIENT, variable_name);
        } else if(arg_1 == "MATERIAL_DIFFUSE") {
            pass->uniforms->register_auto(SP_AUTO_MATERIAL_DIFFUSE, variable_name);
        } else if(arg_1 == "MATERIAL_SPECULAR") {
            pass->uniforms->register_auto(SP_AUTO_MATERIAL_SPECULAR, variable_name);
        } else if(arg_1 == "ACTIVE_TEXTURE_UNITS") {
            pass->uniforms->register_auto(SP_AUTO_MATERIAL_ACTIVE_TEXTURE_UNITS, variable_name);
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
                throw SyntaxError("Invalid argument passed to SET(FLAG BLEND): " + arg_2);
            }
        } else if(arg_1 == "CULL") {
            if(arg_2 == "BACK") {
                pass->set_cull_mode(smlt::CULL_MODE_BACK_FACE);
            } else if(arg_2 == "FRONT") {
                pass->set_cull_mode(smlt::CULL_MODE_FRONT_FACE);
            } else if(arg_2 == "FRONT_AND_BACK") {
                pass->set_cull_mode(smlt::CULL_MODE_FRONT_AND_BACK_FACE);
            } else if(arg_2 == "NONE") {
                pass->set_cull_mode(smlt::CULL_MODE_NONE);
            } else {
                throw SyntaxError("Invalid argument passed to SET(FLAG CULL): " + arg_2);
            }

        } else if(arg_1 == "PREVENT_TEXTURES") {
            // Safety, don't allow code to set a texture unit on this pass
            if(arg_2 == "ON") {
                pass->set_prevent_textures(true);
            } else {
                pass->set_prevent_textures(false);
            }
        } else if(arg_1 == "SHADE_MODEL") {
            if(arg_2 == "FLAT") {
                pass->set_shade_model(SHADE_MODEL_FLAT);
            } else {
                pass->set_shade_model(SHADE_MODEL_SMOOTH);
            }
#ifdef SIMULANT_GL_VERSION_1X
        } else if(arg_1 == "TEXTURING") {
            if(arg_2 == "ON") {
                pass->set_texturing_enabled(true);
            } else {
                pass->set_texturing_enabled(false);
            }
        } else if(arg_1 == "LIGHTING") {
            if(arg_2 == "ON") {
                pass->set_lighting_enabled(true);
            } else {
                pass->set_lighting_enabled(false);
            }
#endif
        } else {
            throw SyntaxError(_u("Invalid argument passed to SET(FLAG): {0}").format(arg_1));
        }
    } else {
        throw SyntaxError(_u("Invalid SET command for pass: {0}").format(type));
    }
}

void MaterialScript::handle_data_block(Material& mat, const unicode& data_type, const std::vector<unicode> &lines, MaterialPass::ptr pass) {
    if(data_type.upper() == "VERTEX") {
        current_vert_shader_ = _u("\n").join(lines).encode();
    } else if(data_type.upper() == "FRAGMENT") {
        current_frag_shader_ = _u("\n").join(lines).encode();
    } else {
        throw SyntaxError(_u("Invalid BEGIN_DATA block: ") + data_type);
    }
}

void MaterialScript::handle_block(Material& mat,
        const std::vector<unicode>& lines,
        uint16_t& current_line,
        const unicode &parent_block_type,
        MaterialPass::ptr current_pass) {

    Renderer* renderer = mat.resource_manager().window->renderer;
    assert(renderer && "No renderer?");

    unicode line = unicode(lines[current_line]).strip();
    current_line++;

    assert(unicode(line).starts_with("BEGIN"));

    std::vector<unicode> block_args = line.split("(")[1].strip(")").split(" ");
    unicode block_type = block_args[0].upper();

    const std::vector<unicode> VALID_BLOCKS = {
        "HEADER",
        "PASS"
    };

    if(std::find(VALID_BLOCKS.begin(), VALID_BLOCKS.end(), block_type) == VALID_BLOCKS.end()) {
        throw SyntaxError(_u("Line: {0}. Invalid block type: {1}").format(current_line, block_type));
    }

    L_DEBUG(_F("Reading material block: {0}").format(block_type));

    if(block_type == "PASS" || block_type == "HEADER") {
        if(!parent_block_type.empty()) {
            throw SyntaxError(_u("Line: {0}. Unexpected {1} block").format(current_line, block_type));
        }

        if(block_args.size() > 1) {
            throw SyntaxError(_u("Line: {0}. Wrong number of arguments.").format(current_line));
        }

        if(block_type == "PASS") {
            //Create the pass with the default shader
            uint32_t pass_number = mat.new_pass();
            current_pass = mat.pass(pass_number);

            L_DEBUG("Material pass created");
        }
    }

    for(uint16_t i = current_line; current_line != lines.size(); ++i) {
        assert(current_line < lines.size());

        line = lines[current_line].strip();

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
            L_DEBUG("Found END block");
            //If we hit an END block, the type must match the BEGIN
            auto parts = line.split("(");

            if(parts.size() < 2) {
                throw SyntaxError("Malformed END statement");
            }

            L_DEBUG("Parsing END type");
            parts[1] = parts[1].strip(")");
            unicode end_block_type = parts[1].upper();

            if(end_block_type != block_type) {
                throw SyntaxError(
                    _u("Line: {0}. Expected END({1}) but found END({2})").format(current_line, block_type, end_block_type));
            }

            if(end_block_type == "PASS" && renderer->supports_gpu_programs()) {
                L_DEBUG("Setting up GPU program on material");

                current_pass->set_gpu_program_id(
                    renderer->new_or_existing_gpu_program(current_vert_shader_, current_frag_shader_)
                );
            }

            L_DEBUG("Finished reading material block");
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

        } else if(line.starts_with("PROPERTY")) {
            unicode args_part = line.split("(")[1].strip(")");
            std::vector<unicode> args = args_part.split(" ");

            if(block_type == "HEADER") {
                handle_header_property_command(mat, args);
            }
        } else if(!line.empty()) {
            L_DEBUG(_F("Unhandled line: ").format(line));
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
}

}
}
