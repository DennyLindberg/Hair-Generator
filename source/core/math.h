#pragma once

#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"

typedef glm::ivec4 Color;
typedef glm::vec4 FColor;

template<typename T>
constexpr T PI = T(3.14159265358979323846264338327950288);

constexpr double PI_d = PI<double>;
constexpr float PI_f = PI<float>;