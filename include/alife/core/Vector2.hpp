#pragma once

#include <cmath>

namespace alife::core {

struct Vector2f {
    float x{0.0f};
    float y{0.0f};

    constexpr Vector2f() noexcept = default;
    constexpr Vector2f(float xVal, float yVal) noexcept : x(xVal), y(yVal) {}

    constexpr Vector2f operator+(const Vector2f& rhs) const noexcept {
        return {x + rhs.x, y + rhs.y};
    }

    constexpr Vector2f operator-(const Vector2f& rhs) const noexcept {
        return {x - rhs.x, y - rhs.y};
    }

    constexpr Vector2f operator*(float scalar) const noexcept {
        return {x * scalar, y * scalar};
    }

    Vector2f& operator+=(const Vector2f& rhs) noexcept {
        x += rhs.x;
        y += rhs.y;
        return *this;
    }

    [[nodiscard]] float lengthSquared() const noexcept {
        return x * x + y * y;
    }

    [[nodiscard]] float length() const noexcept {
        return std::sqrt(lengthSquared());
    }

    [[nodiscard]] Vector2f normalized() const noexcept {
        float len = length();
        if (len > 1e-6f) {
            return {x / len, y / len};
        }
        return {0.0f, 0.0f};
    }
};

} // namespace alife::core
