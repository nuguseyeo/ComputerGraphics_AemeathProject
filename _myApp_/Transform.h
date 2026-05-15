#pragma once

#include <vmath.h>

class Transform {
public:
    vmath::vec3 position = vmath::vec3(0.0f, 0.0f, 0.0f);
    vmath::vec3 rotation = vmath::vec3(0.0f, 0.0f, 0.0f);
    vmath::vec3 scale = vmath::vec3(1.0f, 1.0f, 1.0f);

    vmath::mat4 getModelMatrix() const;
    vmath::vec3 forward() const;
    vmath::vec3 right() const;
    vmath::vec3 up() const;
};
