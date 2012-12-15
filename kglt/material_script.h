#ifndef MATERIAL_SCRIPT_H
#define MATERIAL_SCRIPT_H

#include <stdexcept>
#include <string>
#include <vector>

#include "types.h"

namespace kglt {

class SyntaxError : public std::logic_error {
public:
    SyntaxError(const std::string& what):
        std::logic_error(what) {}
};

/*
 *  Really just to differentiate the constructor from the filename version
 */
class MaterialLanguageText {
public:
    explicit MaterialLanguageText(const std::string& text):
        text_(text) {}

    std::string text() const { return text_; }
private:
    std::string text_;
};

class MaterialScript {
public:
    MaterialScript(Scene& scene, const MaterialLanguageText& text);
    MaterialScript(Scene& scene, const std::string& path);
    MaterialID generate();

private:
    Scene& scene_;
    std::string filename_;
    MaterialLanguageText text_;

    void handle_block(
            MaterialID new_material,
            const std::vector<std::string>& lines,
            uint16_t& current_line,
            const std::string& parent_block_type);

    void handle_technique_set_command(MaterialID new_material, const std::vector<std::string>& args);
    void handle_pass_set_command(MaterialID new_material, const std::vector<std::string>& args);
};

}

#endif // MATERIAL_SCRIPT_H
