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

#include <sys/time.h>
#include <boost/shared_ptr.hpp>
#include <map>
#include "kaztimer.h"

class Timer {
public:
    Timer():
        step_(-1),
        accumulator_(0.0f) {

        set_game_timer();
    }

    void set_fixed(int step) {
        step_ = step;
        is_fixed_ = true;
        last_time_ = get_current_time_in_ms();
        accumulator_ = 0.0f;
    }

    void set_game_timer() {
        step_ = -1;
        is_fixed_ = false;
        last_time_ = get_current_time_in_ms();
    }
    
    void update_frame_time() {
        frame_time_ = get_elapsed_time();
        if(frame_time_ > 0.25) {
            frame_time_ = 0.25;
        }        
        
        if(is_fixed_) {
            accumulator_ += frame_time_;
        }         
    }

    bool can_update() {
        if(!is_fixed_) {            
            return true;
        }
        
        double fixed_step = get_fixed_step();
        if(accumulator_ >= fixed_step) {
            accumulator_ -= fixed_step;
            return true;
        }
        
        return false;
    }

    double get_fixed_step() {
        return 1.0 / double(step_);
    }

    double get_delta_time() {                    
        if(is_fixed_) {                                    
            return get_fixed_step();
        }

        return frame_time_;
    }

    double get_elapsed_time() {
        long current_time = get_current_time_in_ms();
        double elapsed = double(current_time - last_time_) * 0.001;
        last_time_ = current_time;
        return elapsed;
    }

    long get_current_time_in_ms() {
#ifdef WIN32
        return timeGetTime();
#else
        struct timeval tv;
        struct timezone tz;
        gettimeofday(&tv, &tz);
        return (long) (tv.tv_sec * 1000) + (tv.tv_usec / 1000);
#endif
    }

private:
    int step_;
    bool is_fixed_;

    long last_time_;
    double accumulator_;
    double frame_time_;
};

static KTIuint bound_timer_id_ = 0;
static KTIuint current_timer_id_ = 0;

std::map<KTIuint, boost::shared_ptr<Timer> >& timers() {
    static std::map<KTIuint, boost::shared_ptr<Timer> > timers_;
    return timers_;
}

KTIuint get_next_timer_id() {
    return ++current_timer_id_;
}

Timer* get_bound_timer() {
    if(timers().find(bound_timer_id_) == timers().end()) {
        return NULL;
    }

    return timers()[bound_timer_id_].get();
}

void ktiGenTimers(KTIsizei n, KTIuint* names) {
    for(KTIuint i = 0; i < n; ++i) {
        KTIuint new_id = get_next_timer_id();
        timers()[new_id].reset(new Timer());
        names[i] = new_id;
    }
}

void ktiBindTimer(KTIuint name) {
    if(timers().find(name) == timers().end()) {
        bound_timer_id_ = 0;
    }

    bound_timer_id_ = name;
}

void ktiStartFixedStepTimer(KTIint steps_per_second) {
    Timer* timer = get_bound_timer();
    if(!timer) {
        return;
    }

    timer->set_fixed(steps_per_second);
}

void ktiStartGameTimer() {
    Timer* timer = get_bound_timer();
    if(!timer) {
        return;
    }

    timer->set_game_timer();
}

KTIbool ktiTimerCanUpdate() {
    Timer* timer = get_bound_timer();
    if(!timer) {
        return false;
    }

    return timer->can_update();
}

void ktiUpdateFrameTime() {
    Timer* timer = get_bound_timer();
    if(!timer) {
        return;
    }    
    
    timer->update_frame_time();
}

KTIdouble ktiGetDeltaTime() {
    Timer* timer = get_bound_timer();
    if(!timer) {
        return 0.0;
    }

    return timer->get_delta_time();
}

void ktiDeleteTimers(KTIsizei n, const KTIuint* names) {
    for(KTIuint i = 0; i < n; ++i) {
        timers().erase(names[i]);
    }
}
