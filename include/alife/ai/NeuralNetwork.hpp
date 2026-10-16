#pragma once

#include "alife/core/Rng.hpp"
#include <vector>
#include <cmath>
#include <algorithm>
#include <cassert>

namespace alife::ai {

/**
 * @brief Lightweight feedforward neural network with fixed topology (Input -> Hidden -> Output).
 * Evaluates activations using std::tanh to keep outputs bounded in [-1, 1].
 * Zero heap allocations during forward pass (inference uses pre-allocated buffers).
 */
class NeuralNetwork {
public:
    // Default constructor: creates network with default 6 inputs, 8 hidden, 2 outputs
    NeuralNetwork();

    // Construct network with specified layer sizes and initialize weights/biases to 0
    NeuralNetwork(size_t numInputs, size_t numHidden, size_t numOutputs);

    // Randomize weights and biases uniformly in [-range, range]
    void randomize(core::Rng& rng, float range = 1.0f);

    // Forward propagation: evaluates inputs and populates output buffer
    [[nodiscard]] const std::vector<float>& evaluate(const std::vector<float>& inputs);

    // Mutation: each weight/bias mutates with given probability by +/- magnitude
    [[nodiscard]] NeuralNetwork mutate(core::Rng& rng,
                                       float mutationProb,
                                       float mutationMagnitude,
                                       float maxWeightVal = 4.0f) const;

    // Accessors
    [[nodiscard]] size_t getNumInputs() const noexcept { return numInputs_; }
    [[nodiscard]] size_t getNumHidden() const noexcept { return numHidden_; }
    [[nodiscard]] size_t getNumOutputs() const noexcept { return numOutputs_; }
    [[nodiscard]] const std::vector<float>& getWeightsInputHidden() const noexcept { return weightsInputHidden_; }
    [[nodiscard]] const std::vector<float>& getBiasesHidden() const noexcept { return biasesHidden_; }
    [[nodiscard]] const std::vector<float>& getWeightsHiddenOutput() const noexcept { return weightsHiddenOutput_; }
    [[nodiscard]] const std::vector<float>& getBiasesOutput() const noexcept { return biasesOutput_; }
    [[nodiscard]] const std::vector<float>& getLastOutputs() const noexcept { return outputBuffer_; }

    // Direct setter for deterministic testing
    void setParameters(std::vector<float> wIH, std::vector<float> bH,
                       std::vector<float> wHO, std::vector<float> bO);

private:
    size_t numInputs_{6};
    size_t numHidden_{8};
    size_t numOutputs_{2};

    // Flattened weights:
    // weightsInputHidden: numInputs_ * numHidden_
    // biasesHidden: numHidden_
    // weightsHiddenOutput: numHidden_ * numOutputs_
    // biasesOutput: numOutputs_
    std::vector<float> weightsInputHidden_;
    std::vector<float> biasesHidden_;
    std::vector<float> weightsHiddenOutput_;
    std::vector<float> biasesOutput_;

    // Pre-allocated inference buffers
    mutable std::vector<float> hiddenBuffer_;
    mutable std::vector<float> outputBuffer_;
};

} // namespace alife::ai
