#ifndef MATERIAL_SCRIPT_H
#define MATERIAL_SCRIPT_H

#include <stdexcept>
#include <string>
#include <vector>

#include "../generic/managed.h"
#include "../material.h"
#include "../types.h"
#include "../loader.h"

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

class MaterialScript :
    public Managed<MaterialScript> {
public:
    MaterialScript(const MaterialLanguageText& text);
    MaterialScript(const unicode& path);
    void generate(Material& material);

private:
    unicode filename_;
    MaterialLanguageText text_;

    void handle_block(
            Material& mat,
            const std::vector<std::string>& lines,
            uint16_t& current_line,
            const std::string& parent_block_type,
            MaterialTechnique* current_technique=nullptr,
            MaterialPass* current_pass=nullptr);

    void handle_technique_set_command(Material& mat , const std::vector<std::string>& args, MaterialTechnique* technique);
    void handle_pass_set_command(Material& mat, const std::vector<std::string>& args, MaterialPass* pass);
    void handle_data_block(Material& mat, const std::string& data_type, const std::vector<std::string>& lines, MaterialPass* pass);

    void stage_uniform(const std::string& variable, int32_t value) {
        //Stage the uniform for application later
        staged_integer_uniforms_.insert(std::make_pair(variable, value));
    }

    void apply_staged_uniforms(ShaderProgram& program);
    std::map<std::string, int32_t> staged_integer_uniforms_;
};

namespace loaders {

class MaterialScriptLoader:
    public Loader {

public:
    MaterialScriptLoader(const unicode& filename):
        Loader(filename) {
        parser_ = MaterialScript::create(filename);
    }

    void into(Loadable& resource, const LoaderOptions& options) override {
        Material* mat = dynamic_cast<Material*>(&resource);
        assert(mat && "You passed a Resource that is not a material to the Material loader");
        parser_->generate(*mat);
    }

private:
    MaterialScript::ptr parser_;
};

class MaterialScriptLoaderType : public LoaderType {
public:
    unicode name() { return "material_loader"; }
    bool supports(const unicode& filename) const {
        return filename.lower().contains(".kglm");
    }

    Loader::ptr loader_for(const unicode& filename) const {
        return Loader::ptr(new MaterialScriptLoader(filename));
    }
};

}
}

#endif // MATERIAL_SCRIPT_H
