#pragma once

#include "alife/core/Vector2.hpp"
#include <vector>
#include <cmath>
#include <algorithm>
#include <cstdint>

namespace alife::sim {

/**
 * @brief High-performance uniform 2D spatial grid for proximity queries.
 * Replaces O(N^2) all-pairs distance searches with O(1) cell-based lookups.
 * Pre-allocates cell capacity to achieve zero dynamic allocations per frame.
 */
class SpatialGrid {
public:
    SpatialGrid(float worldWidth = 1200.0f, float worldHeight = 800.0f, float cellSize = 100.0f) {
        init(worldWidth, worldHeight, cellSize);
    }

    void init(float worldWidth, float worldHeight, float cellSize) {
        worldWidth_ = (worldWidth > 0.0f) ? worldWidth : 1200.0f;
        worldHeight_ = (worldHeight > 0.0f) ? worldHeight : 800.0f;
        cellSize_ = (cellSize > 10.0f) ? cellSize : 100.0f;
        invCellSize_ = 1.0f / cellSize_;

        cols_ = static_cast<int>(std::ceil(worldWidth_ * invCellSize_));
        rows_ = static_cast<int>(std::ceil(worldHeight_ * invCellSize_));
        if (cols_ < 1) cols_ = 1;
        if (rows_ < 1) rows_ = 1;

        cells_.resize(static_cast<size_t>(cols_ * rows_));
        for (auto& cell : cells_) {
            cell.reserve(32);
        }
    }

    void clear() noexcept {
        totalEntries_ = 0;
        for (auto& cell : cells_) {
            cell.clear();
        }
    }

    void insert(uint32_t index, float x, float y) {
        int cx = static_cast<int>(x * invCellSize_);
        int cy = static_cast<int>(y * invCellSize_);
        cx = std::clamp(cx, 0, cols_ - 1);
        cy = std::clamp(cy, 0, rows_ - 1);

        size_t cellIdx = static_cast<size_t>(cy * cols_ + cx);
        cells_[cellIdx].push_back(index);
        ++totalEntries_;
    }

    /**
     * @brief Query entity indices within the cells overlapping the circle centered at (x, y) with given radius.
     * Guarantees that every candidate within radius is visited without duplicates.
     */
    template<typename Callback>
    void queryNearby(float x, float y, float radius, Callback&& callback) const {
        int minCx = std::clamp(static_cast<int>((x - radius) * invCellSize_), 0, cols_ - 1);
        int maxCx = std::clamp(static_cast<int>((x + radius) * invCellSize_), 0, cols_ - 1);
        int minCy = std::clamp(static_cast<int>((y - radius) * invCellSize_), 0, rows_ - 1);
        int maxCy = std::clamp(static_cast<int>((y + radius) * invCellSize_), 0, rows_ - 1);

        for (int cy = minCy; cy <= maxCy; ++cy) {
            int rowOffset = cy * cols_;
            for (int cx = minCx; cx <= maxCx; ++cx) {
                const auto& cell = cells_[static_cast<size_t>(rowOffset + cx)];
                for (uint32_t idx : cell) {
                    callback(idx);
                }
            }
        }
    }

    [[nodiscard]] size_t size() const noexcept { return totalEntries_; }
    [[nodiscard]] bool empty() const noexcept { return totalEntries_ == 0; }
    [[nodiscard]] int getCols() const noexcept { return cols_; }
    [[nodiscard]] int getRows() const noexcept { return rows_; }
    [[nodiscard]] float getCellSize() const noexcept { return cellSize_; }
    [[nodiscard]] float getInvCellSize() const noexcept { return invCellSize_; }

private:
    float worldWidth_{1200.0f};
    float worldHeight_{800.0f};
    float cellSize_{100.0f};
    float invCellSize_{0.01f};
    int cols_{12};
    int rows_{8};
    size_t totalEntries_{0};

    std::vector<std::vector<uint32_t>> cells_;
};

} // namespace alife::sim
