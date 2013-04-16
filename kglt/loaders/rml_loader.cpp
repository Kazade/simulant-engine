#include <Rocket/Core.h>
#include "rml_loader.h"

#include "../extra/ui/interface.h"

namespace kglt {
namespace loaders {

void RMLLoader::into(Loadable& resource) {
    Loadable* res_ptr = &resource;
    extra::ui::Interface* iface = dynamic_cast<extra::ui::Interface*>(res_ptr);
    assert(iface && "You passed a Resource that is not a Interface to the RML loader");

    iface->_set_document(iface->_context()->LoadDocument(filename_.c_str()));
    if(!iface->_document()) {
        throw IOError("Unable to load the RML document");
    } else {
        iface->_document()->Show();
    }
}

}
}
