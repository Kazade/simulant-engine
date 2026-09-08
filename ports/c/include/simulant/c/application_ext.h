#pragma once

/* Lets C code define its own Application "subclass", the same way
 * stage_node_ext.h does for StageNode: a vtable of callbacks invoked at
 * the same points a C++ subclass would override `init()`/`update()`/etc,
 * plus a user_data pointer standing in for member state.
 *
 * Application::init() is a pure virtual in C++ (every subclass must
 * implement it, typically to register scenes) -- vtable.init is the only
 * callback that's meaningfully required; a NULL init just does nothing
 * and returns true.
 *
 * Usage:
 *
 *   smlt_app_config_t* config = smlt_app_config_create_default();
 *   smlt_app_config_set_width(config, 1280);
 *
 *   smlt_application_vtable_t vtable = {0};
 *   vtable.init = my_init;
 *   vtable.update = my_update;
 *
 *   smlt_application_t* app = smlt_application_create_custom(config, NULL, &vtable, my_state);
 *   smlt_app_config_destroy(config); // Application copies the config
 *   int32_t code = smlt_application_run(app);
 *   smlt_application_destroy(app);
 */

#include "simulant/c/types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    /* Called once during startup, before the main loop; return false to
     * abort startup. NULL is equivalent to "do nothing, return true". */
    bool (*init)(smlt_application_t* self, void* user_data);

    /* Called before window construction; return false to abort startup.
     * NULL is equivalent to "do nothing, return true". */
    bool (*pre_init)(smlt_application_t* self, void* user_data);

    void (*fixed_update)(smlt_application_t* self, float dt, void* user_data);
    void (*update)(smlt_application_t* self, float dt, void* user_data);
    void (*late_update)(smlt_application_t* self, float dt, void* user_data);

    /* Called once during shutdown. */
    void (*clean_up)(smlt_application_t* self, void* user_data);
} smlt_application_vtable_t;

/* Allocates an AppConfig with the engine's normal defaults (matches
 * `smlt::AppConfig config;` in C++). AppConfig has no user-declared
 * constructor, so cgen.py can't safely assume one is synthesizable; this
 * is hand-verified instead. Release it with smlt_app_config_destroy(). */
smlt_app_config_t* smlt_app_config_create_default(void);

/* `vtable` and `config` are both copied by value -- neither needs to
 * outlive this call. */
smlt_application_t* smlt_application_create_custom(const smlt_app_config_t* config,
                                                    void* platform_state,
                                                    const smlt_application_vtable_t* vtable,
                                                    void* user_data);

/* Release with the ordinary smlt_application_destroy() (Application is a
 * plain heap object, not manager-owned like StageNode/Asset). */

void smlt_application_set_user_data(smlt_application_t* self, void* user_data);
void* smlt_application_get_user_data(const smlt_application_t* self);

/* Application::scenes/window are C++ Property<> smart members; the
 * generator's scanner recognizes Property<> fields directly and
 * mechanically wraps them (see smlt_application_scenes()/_window() in the
 * generated application.h), so no hand-written equivalent lives here. */

#ifdef __cplusplus
}
#endif
