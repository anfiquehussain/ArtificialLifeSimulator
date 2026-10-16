#pragma once

#include <random>
#include <cstdint>

namespace alife::core {

/**
 * @brief Deterministic pseudo-random number generator wrapping std::mt19937_64.
 * Injected into subsystems to guarantee reproducibility across experiments.
 */
class Rng {
public:
    explicit Rng(uint64_t seed = 42) : engine_(seed), initialSeed_(seed) {}

    void setSeed(uint64_t seed) noexcept {
        initialSeed_ = seed;
        engine_.seed(seed);
    }

    [[nodiscard]] uint64_t getInitialSeed() const noexcept {
        return initialSeed_;
    }

    // Uniform real distribution in [min, max)
    [[nodiscard]] float randomFloat(float min, float max) {
        std::uniform_real_distribution<float> dist(min, max);
        return dist(engine_);
    }

    // Uniform integer distribution in [min, max]
    [[nodiscard]] int randomInt(int min, int max) {
        std::uniform_int_distribution<int> dist(min, max);
        return dist(engine_);
    }

    [[nodiscard]] std::mt19937_64& getEngine() noexcept {
        return engine_;
    }

private:
    std::mt19937_64 engine_;
    uint64_t initialSeed_{42};
};

} // namespace alife::core
