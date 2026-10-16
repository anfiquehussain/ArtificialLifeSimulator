#pragma once

#include "alife/entities/Organism.hpp"
#include <unordered_map>
#include <cstdint>

namespace alife::biology {

/**
 * @brief Persistent lightweight record of an organism's life and lineage.
 * Retained even after the active organism dies so lineage and reproductive success
 * can be analyzed without keeping heavy runtime state in memory.
 */
struct LineageRecord {
    entities::EntityId id{0};
    entities::EntityId parentId{0};
    uint32_t generation{0};
    double birthTime{0.0};
    double deathTime{-1.0}; // -1 if still alive
    uint32_t offspringCount{0};
    Traits traits;
    entities::DeathCause deathCause{entities::DeathCause::Unknown};
};

/**
 * @brief Lineage tracker maintaining historical parent-child relationships and offspring counts.
 */
class LineageTracker {
public:
    void recordBirth(entities::EntityId id,
                     entities::EntityId parentId,
                     uint32_t generation,
                     double birthTime,
                     const Traits& traits) {
        LineageRecord rec;
        rec.id = id;
        rec.parentId = parentId;
        rec.generation = generation;
        rec.birthTime = birthTime;
        rec.traits = traits;
        records_[id] = rec;

        if (parentId != 0) {
            auto it = records_.find(parentId);
            if (it != records_.end()) {
                ++it->second.offspringCount;
            }
        }
    }

    void recordDeath(entities::EntityId id, double deathTime, entities::DeathCause cause = entities::DeathCause::Unknown) {
        auto it = records_.find(id);
        if (it != records_.end()) {
            it->second.deathTime = deathTime;
            it->second.deathCause = cause;
        }
    }

    void reset() noexcept {
        records_.clear();
    }

    [[nodiscard]] const std::unordered_map<entities::EntityId, LineageRecord>& getRecords() const noexcept {
        return records_;
    }

    [[nodiscard]] size_t getTotalRecorded() const noexcept {
        return records_.size();
    }

    [[nodiscard]] double getAverageLifespan() const noexcept {
        double totalLifespan = 0.0;
        size_t count = 0;
        for (const auto& [id, rec] : records_) {
            if (rec.deathTime >= 0.0 && rec.deathTime >= rec.birthTime) {
                totalLifespan += (rec.deathTime - rec.birthTime);
                ++count;
            }
        }
        return count > 0 ? (totalLifespan / count) : 0.0;
    }

    [[nodiscard]] float getReproductiveFraction() const noexcept {
        if (records_.empty()) return 0.0f;
        size_t breeders = 0;
        for (const auto& [id, rec] : records_) {
            if (rec.offspringCount > 0) {
                ++breeders;
            }
        }
        return static_cast<float>(breeders) / static_cast<float>(records_.size());
    }

    [[nodiscard]] float getMeanOffspringCount() const noexcept {
        if (records_.empty()) return 0.0f;
        uint64_t totalOffspring = 0;
        for (const auto& [id, rec] : records_) {
            totalOffspring += rec.offspringCount;
        }
        return static_cast<float>(totalOffspring) / static_cast<float>(records_.size());
    }

    [[nodiscard]] uint32_t getMaxOffspringCount() const noexcept {
        uint32_t maxCount = 0;
        for (const auto& [id, rec] : records_) {
            if (rec.offspringCount > maxCount) {
                maxCount = rec.offspringCount;
            }
        }
        return maxCount;
    }

    [[nodiscard]] uint64_t getDeathsByCause(entities::DeathCause cause) const noexcept {
        uint64_t count = 0;
        for (const auto& [id, rec] : records_) {
            if (rec.deathCause == cause) {
                ++count;
            }
        }
        return count;
    }

private:
    std::unordered_map<entities::EntityId, LineageRecord> records_;
};

} // namespace alife::biology
