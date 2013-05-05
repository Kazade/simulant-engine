#include <Rocket/Core.h>
#include "rml_loader.h"

#include "../ui/ui_private.h"
#include "../ui/interface.h"

namespace kglt {
namespace loaders {

void RMLLoader::into(Loadable& resource, const LoaderOptions &options) {
    Loadable* res_ptr = &resource;
    ui::Interface* iface = dynamic_cast<ui::Interface*>(res_ptr);
    assert(iface && "You passed a Resource that is not a Interface to the RML loader");

    iface->impl()->document_ = iface->impl()->context_->LoadDocument(filename_.encode().c_str());
    if(!iface->impl()->document_) {
        throw IOError("Unable to load the RML document");
    } else {
        iface->impl()->document_->Show();
    }
}

}
}
