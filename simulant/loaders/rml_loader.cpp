#include "rml_loader.h"

#include "../overlay.h"
#include "../ui/ui_private.h"
#include "../ui/interface.h"

namespace smlt {
namespace loaders {

void RMLLoader::into(Loadable& resource, const LoaderOptions &options) {
    Loadable* res_ptr = &resource;

    Overlay* stage = dynamic_cast<Overlay*>(res_ptr);
    ui::Interface* iface = dynamic_cast<ui::Interface*>(res_ptr);

    //First, check to see if this is a UI stage
    if(stage) {
        // If so, then get the internal interface pointer from there
        iface = stage->__interface();
    }

    assert(iface && "You passed a Resource that is not a Interface to the RML loader");

}

}
}
