/*
Copyright (c) 2011, Luke Benstead.
All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright notice,
      this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright notice,
      this list of conditions and the following disclaimer in the documentation
      and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef KAZTIMER_H_INCLUDED
#define KAZTIMER_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

typedef unsigned int KTIsizei;
typedef unsigned int KTIuint;
typedef int KTIint;
typedef double KTIdouble;
typedef unsigned char KTIbool;

void ktiGenTimers(KTIsizei n, KTIuint* names);
void ktiBindTimer(KTIuint name);
void ktiStartFixedStepTimer(KTIint steps_per_second);
void ktiStartGameTimer();
void ktiUpdateFrameTime();
KTIbool ktiTimerCanUpdate();
KTIdouble ktiGetDeltaTime();
KTIdouble ktiGetAccumulatorValue();
void ktiDeleteTimers(KTIsizei n, const KTIuint* names);

#ifdef __cplusplus
}
#endif

/**
    USAGE:
        KTIuint timers[2];
        ktiGenTimers(2, timers);

        ktiBindTimer(timers[0]);
        ktiStartFixedStepTimer(30);

        while(ktiTimerCanUpdate()) {
            KSfloat dt = ktiGetDeltaTime(); //Will always return a fixed value
            //Do fixed step stuff (e.g. physics)
        }

        ktiStartGameTimer();

        KSfloat deltatime = ktiGetDeltaTime(); //Variable return value
        //ktiTimerCanUpdate() will always return true
        //Update based on elapsed time
*/
#endif // KAZTIMER_H_INCLUDED
