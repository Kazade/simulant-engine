#pragma once

namespace smlt {

struct Degrees;

struct Radians {
private:
    float value_ = 0.0f;

public:
    Radians():
        value_(0) {}

    explicit Radians(float value):
        value_(value) {}

    Radians(const Degrees& rhs);

    template<typename T>
    bool operator<(T value) const {
        return value_ < value;
    }

    template<typename T>
    bool operator<=(T value) const {
        return value_ <= value;
    }

    template<typename T>
    bool operator>(T value) const {
        return value_ > value;
    }

    template<typename T>
    bool operator>=(T value) const {
        return value_ >= value;
    }

    float to_float() const {
        return value_;
    }

    Degrees to_degrees() const;

    Radians operator*(float scalar) const {
        return Radians(to_float() * scalar);
    }

    Radians& operator*=(float scalar) {
        value_ *= scalar;
        return *this;
    }
};

typedef Radians Rad;
}

smlt::Radians operator""_rad(long double v);
