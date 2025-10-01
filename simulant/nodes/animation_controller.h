#pragma once

#include "../utils/limited_string.h"
#include "builtins.h"
#include "locators/node_locator.h"
#include "stage_node.h"
#include <cstdint>
#include <vector>

namespace smlt {
enum AnimationPath {
    ANIMATION_PATH_TRANSLATION,
    ANIMATION_PATH_ROTATION,
    ANIMATION_PATH_SCALE,
    ANIMATION_PATH_WEIGHTS,
};

enum AnimationInterpolation {
    ANIMATION_INTERPOLATION_LINEAR,
    ANIMATION_INTERPOLATION_STEP,
    ANIMATION_INTERPOLATION_CUBIC_SPLINE,
};

template<typename T>
struct LinearInterpolator {
    LinearInterpolator(const T& t0, const T& t1) :
        t0(t0), t1(t1) {}

    T value(float t) const {
        return t0 + ((t1 - t0) * t);
    }

private:
    T t0;
    T t1;
};

template<>
struct LinearInterpolator<Quaternion> {
    LinearInterpolator(const Quaternion& t0, const Quaternion& t1) :
        t0(t0), t1(t1) {}

    Quaternion value(float t) const {
        return t0.slerp(t1, t);
    }

private:
    Quaternion t0;
    Quaternion t1;
};

/* This does a transformation element by element so as not to increase
 * ram usage. */
template<typename T>
inline void transform_to_scalars(std::vector<T>&& input,
                                 std::vector<float>& output);

template<>
inline void transform_to_scalars<Vec3>(std::vector<Vec3>&& input,
                                       std::vector<float>& output) {
    while(!input.empty()) {
        output.push_back(input[0].x);
        output.push_back(input[0].y);
        output.push_back(input[0].z);
        input.erase(input.begin());
    }
}

template<>
inline void transform_to_scalars<Quaternion>(std::vector<Quaternion>&& input,
                                             std::vector<float>& output) {
    while(!input.empty()) {
        output.push_back(input[0].x);
        output.push_back(input[0].y);
        output.push_back(input[0].z);
        output.push_back(input[0].w);
        input.erase(input.begin());
    }
}

template<>
inline void transform_to_scalars<float>(std::vector<float>&& input,
                                        std::vector<float>& output) {
    output = std::move(input);
}

class AnimationData {
public:
    template<typename T>
    AnimationData(const std::vector<float>& times, std::vector<T>&& output) :
        times_(times) {

        transform_to_scalars<T>(std::move(output), output_);

        max_time_ = *std::max_element(times.begin(), times.end());
        min_time_ = *std::min_element(times.begin(), times.end());
    }

    float max_time() const {
        return max_time_;
    }

    float min_time() const {
        return min_time_;
    }

    /**
     * @brief find_times_indices
     *
     * Given a global time `t` this returns the indices of the
     * times before and after. If `t` is less than 0, then 0,0 is returned
     * and if `t` is greater than the max time, then (max,max)
     * is returned.
     * @param t
     * @return The pair of time indices.
     */
    std::pair<std::size_t, std::size_t> find_times_indices(float t) const {
        if(t < 0.0f) {
            return std::make_pair(0u, 1u);
        }

        for(std::size_t i = 0u; i < times_.size(); ++i) {
            float time = times_[i];
            if(time > t) {
                return std::make_pair((i > 0) ? i - 1 : 0, i);
            }
        }

        return std::make_pair(times_.size() - 2, times_.size() - 1);
    }

    template<typename T>
    T interpolated_value(AnimationInterpolation i, float t) {
        t = clamp(t, times_[0], times_.back());

        auto indexes = find_times_indices(t);

        auto t0 = *(((const T*)&output_[0]) + indexes.first);
        auto t1 = *(((const T*)&output_[0]) + indexes.second);

        auto nt = (t - times_[indexes.first]) /
                  (times_[indexes.second] - times_[indexes.first]);

        // FIXME: Only linear interpolation atm
        _S_UNUSED(i);
        return LinearInterpolator(t0, t1).value(nt);
    }

    bool finished(float t) const {
        return t >= max_time();
    }

private:
    float max_time_ = 0.0f;
    float min_time_ = 0.0f;

    std::vector<float> times_;
    std::vector<float> output_;
};

typedef std::shared_ptr<AnimationData> AnimationDataPtr;

struct Channel {
    FindResult<StageNode> target;
    AnimationInterpolation interpolation;
    AnimationDataPtr data;
    AnimationPath path;
};

struct Animation {
    LimitedString<64> name;
    std::vector<Channel> channels;
};

enum AnimationState {
    ANIMATION_STATE_PAUSED,
    ANIMATION_STATE_PLAYING,
};

#define ANIMATION_LOOP_FOREVER -1

class AnimationController: public StageNode {
public:
    S_DEFINE_STAGE_NODE_META(STAGE_NODE_TYPE_ANIMATION_CONTROLLER,
                             "animation_controller");

    AnimationController(Scene* owner) :
        StageNode(owner, Meta::node_type) {}

    bool on_create(Params params) override {
        if(!clean_params<AnimationController>(params)) {
            return false;
        }

        return true;
    }

    bool play(const std::string& animation, int32_t loop_count = 1) {
        auto index = find_animation_index(animation);
        if(index) {
            current_animation_ = index.value();
            time_ = 0.0f;
            state_ = ANIMATION_STATE_PLAYING;
            loop_count_ = (loop_count > 0) ? loop_count - 1 : loop_count;
            std::queue<std::size_t> empty;
            std::swap(animation_queue_, empty);
            return true;
        }
        return false;
    }

    bool queue(const std::string& name) {
        auto index = find_animation_index(name);
        if(index) {
            animation_queue_.push(index.value());
            return true;
        }
        return false;
    }

    void pause() {
        state_ = ANIMATION_STATE_PAUSED;
    }

    void resume() {
        state_ = ANIMATION_STATE_PLAYING;
    }

    void on_update(float dt) override {
        if(state_ == ANIMATION_STATE_PAUSED) {
            return;
        }

        if(current_animation_ >= animations_.size()) {
            return;
        }

        time_ += dt;

        auto& anim = animations_[current_animation_];
        int unfinished = 0;
        for(auto& channel: anim.channels) {
            if(!channel.target) {
                continue;
            }

            auto& data = channel.data;

            if(channel.path == ANIMATION_PATH_TRANSLATION) {
                auto interp = data->interpolated_value<Vec3>(
                    channel.interpolation, time_);
                channel.target->transform->set_translation(interp);
            } else if(channel.path == ANIMATION_PATH_ROTATION) {
                auto interp = data->interpolated_value<Quaternion>(
                    channel.interpolation, time_);
                channel.target->transform->set_rotation(interp);
            } else if(channel.path == ANIMATION_PATH_SCALE) {
                auto interp = data->interpolated_value<Vec3>(
                    channel.interpolation, time_);
                channel.target->transform->set_scale_factor(interp);
            } else if(channel.path == ANIMATION_PATH_WEIGHTS) {
                S_WARN_ONCE("Animation of weights is not yet implemented");
            }

            unfinished += !channel.data->finished(time_);
        }

        if(!unfinished) {
            // We've finished! Either loop, or move onto the next
            // animation.

            if(loop_count_ > 0) {
                // Restart
                time_ = 0.0f;
                loop_count_--;
            } else if(loop_count_ == 0) {
                if(!animation_queue_.empty()) {
                    // Move to the next animation
                    current_animation_ = animation_queue_.front();
                    animation_queue_.pop();
                    time_ = 0.0f;
                }
            } else if(loop_count_ == -1) {
                time_ = 0.0f;
            }
        }
    }

    void push_animation(const Animation& a) {
        animations_.push_back(a);
    }

    std::vector<std::string> animation_names() const {
        std::vector<std::string> ret;
        for(auto& anim: animations_) {
            ret.push_back(anim.name.str());
        }

        return ret;
    }

private:
    optional<std::size_t>
        find_animation_index(const std::string& animation) const {
        auto it = std::find_if(animations_.begin(), animations_.end(),
                               [&](const Animation& a) {
            return a.name == animation;
        });
        if(it == animations_.end()) {
            return no_value;
        }

        return std::distance(animations_.begin(), it);
    }

    std::vector<Animation> animations_;
    std::size_t current_animation_ = 0;
    std::queue<std::size_t> animation_queue_;
    AnimationState state_ = ANIMATION_STATE_PLAYING;
    float time_ = 0.0f;
    int32_t loop_count_ = 1;
};

} // namespace smlt
