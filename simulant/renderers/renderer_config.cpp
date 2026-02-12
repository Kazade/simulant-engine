

#include "renderer_config.h"

#if defined(__DREAMCAST__) || defined(__XBOX__)
    #include "gl1x/gl1x_renderer.h"
#elif defined(__ANDROID__) || defined(__EVERCADE__)
    #include "gl2x/generic_renderer.h"
#elif defined(__PSP__)
    #include "psp/psp_renderer.h"
#else
    #include "gl1x/gl1x_renderer.h"
    #include "gl2x/generic_renderer.h"
#endif

namespace smlt {

Renderer::ptr new_renderer(Window* window, const std::string& name) {
    /*
     * Different platforms return different renderers, the full list of
     * supported renderers is currently:
     *
     * - "gl2x"
     * - "gl1x"
     * - "psp"
     *
     * If a renderer is unsupported a message will be logged and a null pointer
     * returned
     */

    Renderer::ptr NOT_SUPPORTED;

    const char* env = std::getenv("SIMULANT_RENDERER");
    std::string chosen = (env) ? env : name;

    if(chosen.empty()) {
        /* NULL? Then return the default for the platform */
#if defined(__DREAMCAST__) || defined(__XBOX__)
        return std::make_shared<GL1XRenderer>(window);
#elif defined(__PSP__)
        return std::make_shared<PSPRenderer>(window);
#elif defined(__RPI__)
        return std::make_shared<GenericRenderer>(window, /*use_es=*/false);
#elif defined(__EVERCADE__)
        return std::make_shared<GenericRenderer>(window, /*use_es=*/true);
#elif defined(__ANDROID__)
        return std::make_shared<GenericRenderer>(window, /*use_es=*/ true);
#else
        return std::make_shared<GenericRenderer>(window, false);
#endif
    }

    if(chosen == "gl1x") {
#if defined(__ANDROID__) || defined(__PSP__) || defined(__EVERCADE__)
        S_ERROR("{0} is not a supported renderer", name);
        return NOT_SUPPORTED;
#else
        return std::make_shared<GL1XRenderer>(window);
#endif
    } else if(chosen == "gl2x") {
#if defined(__DREAMCAST__) || defined(PSP) || defined(__ANDROID__) || defined(__EVERCADE__) || defined(__XBOX__)
        S_ERROR("{0} is not a supported renderer", name);
        return NOT_SUPPORTED;
#else
        return std::make_shared<GenericRenderer>(window, false);
#endif
    } else if(chosen == "gles2x") {
#if defined(__DREAMCAST__) || defined(PSP) || defined(__WIN32__)|| defined(__XBOX__)
        S_ERROR("{0} is not a supported renderer", name);
        return NOT_SUPPORTED;
#else
        return std::make_shared<GenericRenderer>(window, true);
#endif
    } else if(chosen == "psp") {
#if defined(__PSP__)
        return std::make_shared<PSPRenderer>(window);
#else
        S_ERROR("{0} is not a supported renderer", name);
        return NOT_SUPPORTED;
#endif
    }

    return NOT_SUPPORTED;
}

}
