
#include "md2_loader.h"
#include "../mesh.h"

namespace kglt {
namespace loaders{

void MD2Loader::into(Loadable &resource, const LoaderOptions &options) {
    Mesh* mesh = loadable_to<Mesh>(resource);


}


} //loaders
} //kglt
