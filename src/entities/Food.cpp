#include "alife/entities/Food.hpp"
#include <algorithm>

namespace alife::entities {

Food::Food(FoodId id, const core::Vector2f& position, float energyValue)
    : id_(id),
      position_(position),
      energyValue_(std::max(0.0f, energyValue)),
      isConsumed_(false) {}

float Food::consume() noexcept {
    if (isConsumed_) {
        return 0.0f;
    }
    isConsumed_ = true;
    return energyValue_;
}

} // namespace alife::entities
