#ifndef VERSION_H
#define VERSION_H

#define MAJOR_VERSION 1
#define ANDROID_BUILD 1
#define LINUX_BUILD 1
#define OSX_BUILD 1
#define WINDOWS_BUILD 1

#include <kazbase/exceptions.h>
#include <kazbase/unicode.h>

class Version {
public:
    static unicode version_number() {
#ifdef __LINUX__
        return _u("{0}.{1}.L").format(MAJOR_VERSION, LINUX_BUILD);
#elif __ANDROID__
        return _u("{0}.{1}.A").format(MAJOR_VERSION, ANDROID_BUILD);
#elif __WIN32__
        return _u("{0}.{1}.W").format(MAJOR_VERSION, WINDOWS_BUILD);
#elif __DARWIN__
        return _u("{0}.{1}.O").format(MAJOR_VERSION, OSX_BUILD);
#else
        throw NotImplementedError();
#endif
    }
};

#endif // VERSION_H
