//
//   Copyright (c) 2011-2017 Luke Benstead https://simulant-engine.appspot.com
//
//     This file is part of Simulant.
//
//     Simulant is free software: you can redistribute it and/or modify
//     it under the terms of the GNU General Public License as published by
//     the Free Software Foundation, either version 3 of the License, or
//     (at your option) any later version.
//
//     Simulant is distributed in the hope that it will be useful,
//     but WITHOUT ANY WARRANTY; without even the implied warranty of
//     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//     GNU General Public License for more details.
//
//     You should have received a copy of the GNU General Public License
//     along with Simulant.  If not, see <http://www.gnu.org/licenses/>.
//

#ifdef __APPLE__
#include <OpenAL/al.h>
#include <OpenAL/alc.h>
#else
#include <AL/al.h>
#include <AL/alc.h>
#endif

#include "../deps/kazlog/kazlog.h"

namespace ALChecker {
void al_check_and_log_error() {
    ALuint error = alGetError();
    if(error != AL_NO_ERROR) {
        std::string error_string;
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

        L_ERROR(_F("An OpenAL error occurred: {0}").format(error));
        throw std::runtime_error(_F("AL ERROR: {0}").format(error_string));
    }
}
}
