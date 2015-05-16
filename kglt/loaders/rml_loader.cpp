#include <Rocket/Core.h>
#include "rml_loader.h"

#include "../ui_stage.h"
#include "../ui/ui_private.h"
#include "../ui/interface.h"

namespace kglt {
namespace loaders {

void RMLLoader::into(Loadable& resource, const LoaderOptions &options) {
    Loadable* res_ptr = &resource;

    UIStage* stage = dynamic_cast<UIStage*>(res_ptr);
    ui::Interface* iface = dynamic_cast<ui::Interface*>(res_ptr);

    //First, check to see if this is a UI stage
    if(stage) {
        // If so, then get the internal interface pointer from there
        iface = stage->__interface();
    }

    assert(iface && "You passed a Resource that is not a Interface to the RML loader");

    auto str = data_->str();
    if(str.empty()) {
        throw IOError("Couldn't load specified RML");
    }



    ui::set_active_impl(iface->impl());
    auto doc = iface->impl()->context_->LoadDocumentFromMemory(str.c_str());
    ui::set_active_impl(nullptr);

    if(!doc) {
        throw IOError("Unable to load specified document");
    }

    iface->impl()->document_ = dynamic_cast<kglt::ui::CustomDocument*>(doc);

    if(!iface->impl()->document_) {
        throw IOError("Unable to load the RML document");
    } else {
        iface->impl()->document_->Show();
    }
}

}
}
