#pragma once

#include <vector>
#include <functional>

#include "../generic/managed.h"

namespace smlt {

typedef std::function<void (InputAxis*)> EachAxisCallback;
typedef std::vector<InputAxis*> AxisList;

class InputAxis;

class InputManager:
    public Managed<InputManager> {

public:
    InputAxis* new_axis(const std::string& name);
    AxisList axises(const std::string& name) const;
    void each_axis(EachAxisCallback callback);
    void delete_axises(const std::string& name);
    void delete_axis(InputAxis* axis);

    float axis_value(const std::string& name) const;

private:
    std::vector<std::shared_ptr<InputAxis>> axises_;
};

}
