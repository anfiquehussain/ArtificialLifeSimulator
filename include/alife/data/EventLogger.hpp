#pragma once

#include "alife/entities/Organism.hpp"
#include "alife/biology/Species.hpp"
#include <vector>
#include <string>
#include <fstream>
#include <iomanip>
#include <algorithm>
#include <cstdint>

namespace alife::data {

/**
 * @brief Discrete ecosystem event categories.
 */
enum class EventType : uint8_t {
    Birth = 0,
    Death = 1,
    Reproduction = 2,
    Mutation = 3,
    FoodConsumption = 4,
    Predation = 5,
    Extinction = 6,
    Recovery = 7
};

[[nodiscard]] constexpr const char* eventTypeToString(EventType type) noexcept {
    switch (type) {
        case EventType::Birth: return "Birth";
        case EventType::Death: return "Death";
        case EventType::Reproduction: return "Reproduction";
        case EventType::Mutation: return "Mutation";
        case EventType::FoodConsumption: return "FoodConsumption";
        case EventType::Predation: return "Predation";
        case EventType::Extinction: return "Extinction";
        case EventType::Recovery: return "Recovery";
        default: return "Unknown";
    }
}

/**
 * @brief Lightweight record of an individual ecological or lifecycle event.
 */
struct EcosystemEvent {
    double timestamp{0.0};
    uint64_t stepCount{0};
    EventType type{EventType::Birth};
    entities::EntityId primaryId{0};
    entities::EntityId secondaryId{0};
    biology::SpeciesId speciesId{biology::SpeciesId::Herbivore};
    std::string details;
};

/**
 * @brief Bounded in-memory event logger maintaining historical simulation events.
 * Safely caps memory by discarding oldest entries when capacity is reached.
 */
class EventLogger {
public:
    explicit EventLogger(size_t maxEvents = 10000)
        : maxEvents_(maxEvents) {
        events_.reserve(std::min(maxEvents, size_t(2000)));
    }

    void logEvent(const EcosystemEvent& event) {
        if (events_.size() >= maxEvents_) {
            events_.erase(events_.begin());
        }
        events_.push_back(event);
    }

    void logEvent(double timestamp,
                  uint64_t stepCount,
                  EventType type,
                  entities::EntityId primaryId,
                  entities::EntityId secondaryId,
                  biology::SpeciesId speciesId,
                  const std::string& details = "") {
        logEvent(EcosystemEvent{timestamp, stepCount, type, primaryId, secondaryId, speciesId, details});
    }

    void reset() noexcept {
        events_.clear();
    }

    [[nodiscard]] const std::vector<EcosystemEvent>& getEvents() const noexcept {
        return events_;
    }

    [[nodiscard]] size_t getEventCount() const noexcept {
        return events_.size();
    }

    [[nodiscard]] size_t getMaxEvents() const noexcept {
        return maxEvents_;
    }

    void setMaxEvents(size_t maxEvents) noexcept {
        maxEvents_ = maxEvents;
        if (events_.size() > maxEvents_) {
            events_.erase(events_.begin(), events_.begin() + (events_.size() - maxEvents_));
        }
    }

    bool exportToCsv(const std::string& filepath) const {
        std::ofstream file(filepath);
        if (!file.is_open()) {
            return false;
        }

        // CSV Header
        file << "timestamp,step,event_type,primary_id,secondary_id,species,details\n";

        for (const auto& ev : events_) {
            file << std::fixed << std::setprecision(3) << ev.timestamp << ","
                 << ev.stepCount << ","
                 << eventTypeToString(ev.type) << ","
                 << ev.primaryId << ","
                 << ev.secondaryId << ","
                 << biology::speciesName(ev.speciesId) << ",\""
                 << ev.details << "\"\n";
        }

        return true;
    }

private:
    size_t maxEvents_{10000};
    std::vector<EcosystemEvent> events_;
};

} // namespace alife::data
