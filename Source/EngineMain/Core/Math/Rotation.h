#pragma once

#include "CoreMathTypedefs.h"

#include <glm/ext/vector_float3.hpp>

class Rotation
{
private:
    glm::vec3 value;

    Rotation(const glm::vec3& rotValue);

public:
    Rotation();
    Rotation(const float& r, const float& p, const float& y);
    explicit Rotation(const float& allValue);
    Rotation(const Rotation& other);
    Rotation(Rotation&& other);
    void operator=(const Rotation& other);
    void operator=(Rotation&& other);

    float& roll();
    float& pitch();
    float& yaw();
    float roll() const;
    float pitch() const;
    float yaw() const;

public:
    bool operator==(const Rotation& b) const;
    // Component wise operations
    Rotation operator*(const Rotation& b) const;
    void operator*=(const Rotation& b);
    Rotation operator/(const Rotation& b) const;
    void operator/=(const Rotation& b);
    Rotation operator-(const Rotation& b) const;
    void operator-=(const Rotation& b);
    Rotation operator+(const Rotation& b) const;
    void operator+=(const Rotation& b);
    Rotation operator*(const float& scalar) const;
    void operator*=(const float& scalar);
    Rotation operator/(const float& scalar) const;
    void operator/=(const float& scalar);
    Rotation operator-(const float& scalar) const;
    void operator-=(const float& scalar);
    Rotation operator+(const float& scalar) const;
    void operator+=(const float& scalar);
    bool isSame(const Rotation& b, float epsilon = SMALL_EPSILON) const;
};
