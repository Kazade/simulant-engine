#include <AL/al.h>
#include <AL/alc.h>

#include <kazbase/logging.h>
#include <kazbase/exceptions.h>
#include <kazbase/unicode.h>

namespace ALChecker {
void al_check_and_log_error() {
    ALuint error = alGetError();
    if(error != AL_NO_ERROR) {
        unicode error_string;
        switch(error) {
        case AL_INVALID_NAME:
            error_string = "AL_INVALID_NAME";
            break;
        case AL_INVALID_ENUM:
            error_string = "AL_INVALID_ENUM";
            break;
        case AL_INVALID_VALUE:
            error_string = "AL_INVALID_VALUE";
            break;
        case AL_INVALID_OPERATION:
            error_string = "AL_INVALID_OPERATION";
            break;
        case AL_OUT_OF_MEMORY:
            error_string = "AL_OUT_OF_MEMORY";
            break;
        }

        L_ERROR(_u("An OpenAL error occurred: {0}").format(error));
        throw RuntimeError(_u("AL ERROR: {0}").format(error_string).encode());
    }
}
}
