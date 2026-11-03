#include <pwd.h>
#include <unistd.h>

#include "./posix.h"

namespace smlt {
namespace posix {

User current_user() {
    unicode full_name, login_name;

    if (const passwd* pw = getpwuid(getuid())) {
        std::string gecos = pw->pw_gecos ? pw->pw_gecos : "";
        std::string pwname = pw->pw_name ? pw->pw_name : "";
        full_name = unicode(gecos, "utf-8");
        full_name = full_name.substr(0, full_name.find(','));
        login_name = unicode(pwname, "utf-8");
    }

    if(login_name.empty()) {
        const char* user = std::getenv("USER");
        login_name = unicode((user) ? user : "", "utf-8");
    }

    return User{login_name, full_name};
}

}
}
