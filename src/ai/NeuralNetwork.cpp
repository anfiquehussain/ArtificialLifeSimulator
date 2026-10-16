#include "alife/ai/NeuralNetwork.hpp"

namespace alife::ai {

NeuralNetwork::NeuralNetwork()
    : NeuralNetwork(6, 8, 2) {}

NeuralNetwork::NeuralNetwork(size_t numInputs, size_t numHidden, size_t numOutputs)
    : numInputs_(numInputs),
      numHidden_(numHidden),
      numOutputs_(numOutputs),
      weightsInputHidden_(numInputs * numHidden, 0.0f),
      biasesHidden_(numHidden, 0.0f),
      weightsHiddenOutput_(numHidden * numOutputs, 0.0f),
      biasesOutput_(numOutputs, 0.0f),
      hiddenBuffer_(numHidden, 0.0f),
      outputBuffer_(numOutputs, 0.0f) {}

void NeuralNetwork::randomize(core::Rng& rng, float range) {
    for (auto& w : weightsInputHidden_) {
        w = rng.randomFloat(-range, range);
    }
    for (auto& b : biasesHidden_) {
        b = rng.randomFloat(-range, range);
    }
    for (auto& w : weightsHiddenOutput_) {
        w = rng.randomFloat(-range, range);
    }
    for (auto& b : biasesOutput_) {
        b = rng.randomFloat(-range, range);
    }
}

void NeuralNetwork::setParameters(std::vector<float> wIH, std::vector<float> bH,
                                  std::vector<float> wHO, std::vector<float> bO) {
    assert(wIH.size() == numInputs_ * numHidden_);
    assert(bH.size() == numHidden_);
    assert(wHO.size() == numHidden_ * numOutputs_);
    assert(bO.size() == numOutputs_);

    for (auto& w : wIH) if (std::isnan(w) || std::isinf(w)) w = 0.0f;
    for (auto& b : bH)  if (std::isnan(b) || std::isinf(b)) b = 0.0f;
    for (auto& w : wHO) if (std::isnan(w) || std::isinf(w)) w = 0.0f;
    for (auto& b : bO)  if (std::isnan(b) || std::isinf(b)) b = 0.0f;

    weightsInputHidden_ = std::move(wIH);
    biasesHidden_ = std::move(bH);
    weightsHiddenOutput_ = std::move(wHO);
    biasesOutput_ = std::move(bO);
}

const std::vector<float>& NeuralNetwork::evaluate(const std::vector<float>& inputs) {
    // 1. Layer 1: Inputs -> Hidden (vectorized stride-1 memory accumulation)
    for (size_t h = 0; h < numHidden_; ++h) {
        hiddenBuffer_[h] = biasesHidden_[h];
    }

    size_t inCount = std::min(numInputs_, inputs.size());
    for (size_t i = 0; i < inCount; ++i) {
        float inVal = inputs[i];
        if (std::isnan(inVal) || std::isinf(inVal)) {
            inVal = 0.0f;
        }
        const float* wRow = &weightsInputHidden_[i * numHidden_];
        for (size_t h = 0; h < numHidden_; ++h) {
            hiddenBuffer_[h] += inVal * wRow[h];
        }
    }

    for (size_t h = 0; h < numHidden_; ++h) {
        hiddenBuffer_[h] = std::tanh(hiddenBuffer_[h]);
        if (std::isnan(hiddenBuffer_[h]) || std::isinf(hiddenBuffer_[h])) {
            hiddenBuffer_[h] = 0.0f;
        }
    }

    // 2. Layer 2: Hidden -> Output (vectorized stride-1 memory accumulation)
    for (size_t o = 0; o < numOutputs_; ++o) {
        outputBuffer_[o] = biasesOutput_[o];
    }

    for (size_t h = 0; h < numHidden_; ++h) {
        float hVal = hiddenBuffer_[h];
        const float* wRow = &weightsHiddenOutput_[h * numOutputs_];
        for (size_t o = 0; o < numOutputs_; ++o) {
            outputBuffer_[o] += hVal * wRow[o];
        }
    }

    for (size_t o = 0; o < numOutputs_; ++o) {
        outputBuffer_[o] = std::tanh(outputBuffer_[o]);
        if (std::isnan(outputBuffer_[o]) || std::isinf(outputBuffer_[o])) {
            outputBuffer_[o] = 0.0f;
        }
    }

    return outputBuffer_;
}

NeuralNetwork NeuralNetwork::mutate(core::Rng& rng,
                                    float mutationProb,
                                    float mutationMagnitude,
                                    float maxWeightVal) const {
    NeuralNetwork child = *this;

    auto mutateParam = [&](float val) -> float {
        if (rng.randomFloat(0.0f, 1.0f) < mutationProb) {
            float delta = rng.randomFloat(-mutationMagnitude, mutationMagnitude);
            float newVal = val + delta;
            if (std::isnan(newVal) || std::isinf(newVal)) return val;
            return std::clamp(newVal, -maxWeightVal, maxWeightVal);
        }
        return val;
    };

    for (auto& w : child.weightsInputHidden_) {
        w = mutateParam(w);
    }
    for (auto& b : child.biasesHidden_) {
        b = mutateParam(b);
    }
    for (auto& w : child.weightsHiddenOutput_) {
        w = mutateParam(w);
    }
    for (auto& b : child.biasesOutput_) {
        b = mutateParam(b);
    }

    return child;
}

} // namespace alife::ai
