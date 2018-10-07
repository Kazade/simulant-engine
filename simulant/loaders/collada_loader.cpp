#include <functional>

#include "../deps/tinyxml/tinyxml.h"
#include "../nodes/actor.h"
#include "collada_loader.h"
#include "../stage.h"
#include "../deps/tinyxml/tinyxml.h"

namespace smlt {
namespace loaders {

TiXmlNode* search_attr(TiXmlNode* node, const std::string& attr, const std::string& value, bool recursive) {
    std::function<TiXmlNode* (TiXmlNode*)> search = [&search, recursive, attr, &value](TiXmlNode* node) -> TiXmlNode* {
        auto attribute = node->ToElement()->Attribute(attr);
        if(attribute && *attribute == value) {
            return node;
        }

        if(recursive) {
            auto child = node->FirstChild();
            for(; child; child = child->NextSibling()) {
                if(!child->ToElement()) {
                    continue;
                }

                auto ret = search(child->ToElement());
                if(ret) {
                    return ret;
                }
            }
        }

        return nullptr;
    };

    return search(node);
}

TiXmlNode* find_xml_node(TiXmlNode* node, const std::string& path) {
    /** Finds an xml node using the lookups defined in the collada spec */

    auto search_sid_path = [](TiXmlNode* start, std::string path) -> TiXmlNode* {
        TiXmlNode* ret = start;
        std::string part;
        for(uint32_t i = 0; i < path.size(); ++i) {
            if(path[i] == '.' && path[i + 1] == '/') {
                // If we hit ./ then do nothing except skip the slash
                i += 1;
                continue;
            }

            if(path[i] != '/') {
                part += path[i];
            }

            if(path[i] == '/' || i == path.size() - 1) {
                ret = search_attr(ret, "sid", part, false);
                part = "";
            }
        }

        return ret;
    };

    if(path[0] == '#') {
        // Easy, it's an ID
        std::string id(path.begin() + 1, path.end());
        return search_attr(node, "id", id, true);
    } else if(path[0] == '.' && path[1] == '/') {
        // Relative path of sids
        return search_sid_path(node, path);
    } else {
        // First part is an ID, so find the first slash
        auto slash_index = path.find_first_of("/");
        std::string id(path.begin(), path.begin() + slash_index);

        TiXmlNode* root = search_attr(node, "id", id, true);
        return search_sid_path(root, path.substr(slash_index + 1));
    }
}

void ColladaLoader::into(Loadable& resource, const LoaderOptions& options) {
    Stage* scene = dynamic_cast<Stage*>(&resource);
    Mesh* mesh = dynamic_cast<Mesh*>(&resource);

    if(scene) {
        load_into_stage(scene, options);
    } else if(mesh) {
        load_into_mesh(mesh, options);
    } else {
        throw std::logic_error("Tried to load a collada file into something that wasn't a mesh or scene");
    }
}

void ColladaLoader::handle_geometry(TiXmlElement* geometry, Stage* stage) {
    std::string id = geometry->Attribute("id");

    TiXmlElement* mesh = geometry->FirstChildElement("mesh");
    TiXmlElement* vertices = mesh->FirstChildElement("vertices");


    /* Continue here:
     * - Store vertices etc. as they may be reused, also account that
     * - Vertex, texcoord, normal etc. indices may differ so unique combinations
     * should be added to mesh vertex data and keyed */

}

void ColladaLoader::handle_node(TiXmlElement* pnode, Stage* stage, StageNode* parent) {
    TiXmlElement* node = pnode->FirstChildElement();
    for(; node; node = node->NextSiblingElement()) {
        Actor* actor = stage->new_actor();

        if(parent) {
            parent->add_child(actor);
        }

        std::string value = node->Value();

        if(value == "node") {
            handle_node(node, stage, actor);
        } else if(value == "instance_geometry") {
            TiXmlElement* geometry = find_xml_node(pnode->GetDocument()->RootElement(), node->Attribute("url"))->ToElement();
            std::string geom_id = geometry->Attribute("id");
            if(meshes_.count(geom_id)) {
                actor->set_mesh(meshes_.at(geom_id));
            } else {
                handle_geometry(geometry, stage);
            }
        } else if(value == "translate") {

        } else if(value == "rotate") {

        } else if(value == "scale") {

        } else {
            L_DEBUG(_F("Unhandled node: {0}").format(node->Value()));
        }
    }
}

void ColladaLoader::load_into_stage(Stage* stage, const LoaderOptions& options) {
    TiXmlDocument document;
    (*data_) >> document;

    // All collada docs have one <scene>
    TiXmlElement* scene = document.RootElement()->FirstChild("scene")->ToElement();
    TiXmlElement* instance_visual_scene = scene->FirstChild("instance_visual_scene")->ToElement();
    TiXmlElement* visual_scene = find_xml_node(document.RootElement(), instance_visual_scene->Attribute("url"))->ToElement();

    handle_node(visual_scene, stage, stage);
}

void ColladaLoader::load_into_mesh(Mesh* mesh, const LoaderOptions& options) {

}

}
}
