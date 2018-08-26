
#include "renderer_config.h"

#ifdef _arch_dreamcast
    #include "gl1x/gl1x_renderer.h"
#elif defined(__ANDROID__)
    #include "gl2x/generic_renderer.h"
#else
    #include "gl1x/gl1x_renderer.h"
    #include "gl2x/generic_renderer.h"
#endif

namespace smlt {

Renderer::ptr new_renderer(Window* window, const char* name) {
    /*
     * Different platforms return different renderers, the full list of supported renderers is
     * currently:
     *
     * - "gl2x"
     * - "gl1x"
     *
     * If a renderer is unsupported a message will be logged and a null pointer returned
     */

    Renderer::ptr NOT_SUPPORTED;

    if(!name) {
        /* NULL? Then return the default for the platform */
#ifdef _arch_dreamcast
        return std::make_shared<GL1Renderer>(window);
#else
        return std::make_shared<GenericRenderer>(window);
#endif
    }

    if(std::string("gl1x") == name) {
#ifdef __ANDROID__
        L_ERROR(_F("{0} is not a supported renderer").format(name));
        return NOT_SUPPORTED;
#else
        return std::make_shared<GL1XRenderer>(window);
#endif
    } else if(std::string("gl2x") == name) {
#ifdef _arch_dreamcast
        L_ERROR(_F("{0} is not a supported renderer").format(name));
        return NOT_SUPPORTED;
#else
        return std::make_shared<GenericRenderer>(window);
#endif
    }

    return NOT_SUPPORTED;
}

}
