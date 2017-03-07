/* *   Copyright (c) 2011-2017 Luke Benstead https://simulant-engine.appspot.com
 *
 *     This file is part of Simulant.
 *
 *     Simulant is free software: you can redistribute it and/or modify
 *     it under the terms of the GNU General Public License as published by
 *     the Free Software Foundation, either version 3 of the License, or
 *     (at your option) any later version.
 *
 *     Simulant is distributed in the hope that it will be useful,
 *     but WITHOUT ANY WARRANTY; without even the implied warranty of
 *     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *     GNU General Public License for more details.
 *
 *     You should have received a copy of the GNU General Public License
 *     along with Simulant.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef MATERIAL_SCRIPT_H
#define MATERIAL_SCRIPT_H

#include <stdexcept>
#include <string>
#include <vector>

#include "../generic/managed.h"
#include "../material.h"
#include "../types.h"
#include "../loader.h"
#include "../watcher.h"

namespace smlt {

class SyntaxError : public std::logic_error {
public:
    SyntaxError(const unicode& what):
        std::logic_error(what.encode()) {}
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
    MaterialScript(std::shared_ptr<std::stringstream> data);
    void generate(Material& material);

private:
    unicode filename_;
    MaterialLanguageText text_;

    void handle_block(Material& mat,
            const std::vector<unicode> &lines,
            uint16_t& current_line,
            const unicode& parent_block_type,
            MaterialPass::ptr current_pass=MaterialPass::ptr());

    void handle_header_property_command(Material& mat, const std::vector<unicode> &args);
    void handle_pass_set_command(Material& mat, const std::vector<unicode> &args, MaterialPass::ptr pass);
    void handle_data_block(Material& mat, const unicode &data_type, const std::vector<unicode>& lines, MaterialPass::ptr pass);

    std::string current_vert_shader_;
    std::string current_frag_shader_;
};

namespace loaders {

class MaterialReloader {
public:
    MaterialReloader(ResourceManager& rm, MaterialID material);
    void reload(const unicode& path, WatchEvent evt);

private:
    ResourceManager& rm_;
    MaterialID material_;
};

class MaterialScriptLoader:
    public Loader {

public:
    MaterialScriptLoader(const unicode& filename, std::shared_ptr<std::stringstream> data):
        Loader(filename, data) {
        parser_ = MaterialScript::create(data);
    }

    void into(Loadable& resource, const LoaderOptions& options) override;

private:
    MaterialScript::ptr parser_;
};

class MaterialScriptLoaderType : public LoaderType {
public:
    unicode name() { return "material"; }
    bool supports(const unicode& filename) const {
        return filename.lower().contains(".kglm");
    }

    Loader::ptr loader_for(const unicode& filename, std::shared_ptr<std::stringstream> data) const {
        return Loader::ptr(new MaterialScriptLoader(filename, data));
    }
};

}
}

#endif // MATERIAL_SCRIPT_H
