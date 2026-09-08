#include "simulant/c/application_ext.h"

#include "simulant/application.h"
#include "simulant/window.h"

namespace {

class CApplication: public smlt::Application {
public:
    CApplication(const smlt::AppConfig& config, void* platform_state,
                smlt_application_vtable_t vtable, void* user_data) :
        Application(config, platform_state),
        vtable_(vtable),
        user_data_(user_data) {}

    void set_user_data(void* user_data) {
        user_data_ = user_data;
    }

    void* user_data() const {
        return user_data_;
    }

protected:
    bool init() override {
        if(vtable_.init) {
            return vtable_.init(self(), user_data_);
        }
        return true;
    }

    bool pre_init() override {
        if(vtable_.pre_init) {
            return vtable_.pre_init(self(), user_data_);
        }
        return true;
    }

    void fixed_update(float dt) override {
        if(vtable_.fixed_update) {
            vtable_.fixed_update(self(), dt, user_data_);
        }
    }

    void update(float dt) override {
        if(vtable_.update) {
            vtable_.update(self(), dt, user_data_);
        }
    }

    void late_update(float dt) override {
        if(vtable_.late_update) {
            vtable_.late_update(self(), dt, user_data_);
        }
    }

    void clean_up() override {
        if(vtable_.clean_up) {
            vtable_.clean_up(self(), user_data_);
        }
    }

private:
    smlt_application_t* self() {
        return reinterpret_cast<smlt_application_t*>(this);
    }

    smlt_application_vtable_t vtable_;
    void* user_data_;
};

} // namespace

extern "C" {

smlt_app_config_t* smlt_app_config_create_default(void) {
    return reinterpret_cast<smlt_app_config_t*>(new smlt::AppConfig());
}

smlt_application_t* smlt_application_create_custom(const smlt_app_config_t* config,
                                                    void* platform_state,
                                                    const smlt_application_vtable_t* vtable,
                                                    void* user_data) {
    auto* real_config = reinterpret_cast<const smlt::AppConfig*>(config);
    smlt_application_vtable_t vtable_copy = vtable ? *vtable : smlt_application_vtable_t{};
    auto* app = new CApplication(*real_config, platform_state, vtable_copy, user_data);
    return reinterpret_cast<smlt_application_t*>(app);
}

void smlt_application_set_user_data(smlt_application_t* self, void* user_data) {
    auto* app = dynamic_cast<CApplication*>(reinterpret_cast<smlt::Application*>(self));
    if(app) {
        app->set_user_data(user_data);
    }
}

void* smlt_application_get_user_data(const smlt_application_t* self) {
    auto* app =
        dynamic_cast<const CApplication*>(reinterpret_cast<const smlt::Application*>(self));
    return app ? app->user_data() : nullptr;
}

} // extern "C"
