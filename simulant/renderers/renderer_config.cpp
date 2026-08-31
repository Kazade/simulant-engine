

#include "renderer_config.h"

#ifdef __DREAMCAST__
    #include "gl1x/gl1x_renderer.h"
    #include "pvr/pvr_renderer.h"
#elif defined(__ANDROID__) || defined(__EVERCADE__)
    #include "gl2x/generic_renderer.h"
#elif defined(__PSP__)
    #include "psp/psp_renderer.h"
#else
    #include "gl1x/gl1x_renderer.h"
    #include "gl2x/generic_renderer.h"
#endif

namespace smlt {

static Renderer::ptr default_renderer(Window* window) {
#if defined(__DREAMCAST__)
    return std::make_shared<PVRRenderer>(window);
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

Renderer::ptr new_renderer(Window* window, const std::string& name) {
    /*
     * Different platforms return different renderers, the full list of
     * supported renderers is currently:
     *
     * - "gl2x"
     * - "gl1x"
     * - "psp"
     * - "pvr"
     *
     * If a renderer is unsupported, or unrecognized, a message is logged
     * and the platform's default renderer is returned instead.
     */

    const char* env = std::getenv("SIMULANT_RENDERER");
    std::string chosen = (env) ? env : name;

    if(chosen.empty()) {
        return default_renderer(window);
    }

    if(chosen == "gl1x") {
#if defined(__ANDROID__) || defined(__PSP__) || defined(__EVERCADE__)
        S_WARN("{0} is not a supported renderer on this platform, falling back to the default", name);
        return default_renderer(window);
#else
        return std::make_shared<GL1XRenderer>(window);
#endif
    } else if(chosen == "gl2x") {
#if defined(__DREAMCAST__) || defined(PSP) || defined(__ANDROID__) || defined(__EVERCADE__)
        S_WARN("{0} is not a supported renderer on this platform, falling back to the default", name);
        return default_renderer(window);
#else
        return std::make_shared<GenericRenderer>(window, false);
#endif
    } else if(chosen == "gles2x") {
#if defined(__DREAMCAST__) || defined(PSP) || defined(__WIN32__)
        S_WARN("{0} is not a supported renderer on this platform, falling back to the default", name);
        return default_renderer(window);
#else
        return std::make_shared<GenericRenderer>(window, true);
#endif
    } else if(chosen == "psp") {
#if defined(__PSP__)
        return std::make_shared<PSPRenderer>(window);
#else
        S_WARN("{0} is not a supported renderer on this platform, falling back to the default", name);
        return default_renderer(window);
#endif
    } else if(chosen == "pvr") {
#if defined(__DREAMCAST__)
        return std::make_shared<PVRRenderer>(window);
#else
        S_WARN("{0} is not a supported renderer on this platform, falling back to the default", name);
        return default_renderer(window);
#endif
    }

    S_WARN("{0} is not a recognized renderer, falling back to the default", name);
    return default_renderer(window);
}

}
