#include "Transform.h"
#include <cmath>

namespace {
constexpr float kDegToRad = 0.0174532925f;
}

vmath::mat4 Transform::getModelMatrix() const {
    return vmath::translate(position[0], position[1], position[2])
        * vmath::rotate(rotation[1], 0.0f, 1.0f, 0.0f)
        * vmath::rotate(rotation[0], 1.0f, 0.0f, 0.0f)
        * vmath::rotate(rotation[2], 0.0f, 0.0f, 1.0f)
        * vmath::scale(scale[0], scale[1], scale[2]);
}

vmath::vec3 Transform::forward() const {
    const float yawRad = rotation[1] * kDegToRad;
    return vmath::vec3(std::sin(yawRad), 0.0f, -std::cos(yawRad));
}

vmath::vec3 Transform::right() const {
    const float yawRad = rotation[1] * kDegToRad;
    return vmath::vec3(std::cos(yawRad), 0.0f, std::sin(yawRad));
}

vmath::vec3 Transform::up() const {
    return vmath::vec3(0.0f, 1.0f, 0.0f);
}
