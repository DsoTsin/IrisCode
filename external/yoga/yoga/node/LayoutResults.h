/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#pragma once

#include <array>

#include <yoga/debug/AssertFatal.h>
#include <yoga/enums/Dimension.h>
#include <yoga/enums/Direction.h>
#include <yoga/enums/Edge.h>
#include <yoga/enums/PhysicalEdge.h>
#include <yoga/node/CachedMeasurement.h>
#include <yoga/numeric/FloatOptional.h>

namespace iris::ui {

struct LayoutResults {
  // This value was chosen based on empirical data:
  // 98% of analyzed layouts require less than 8 entries.
  static constexpr int32_t MaxCachedMeasurements = 8;

  uint32_t computedFlexBasisGeneration = 0;
  FloatOptional computedFlexBasis = {};

  // Instead of recomputing the entire layout every single time, we cache some
  // information to break early when nothing changed
  uint32_t generationCount = 0;
  uint32_t configVersion = 0;
  Direction lastOwnerDirection = Direction::Inherit;

  uint32_t nextCachedMeasurementsIndex = 0;
  std::array<CachedMeasurement, MaxCachedMeasurements> cachedMeasurements = {};

  CachedMeasurement cachedLayout{};

  Direction direction() const {
    return direction_;
  }

  void setDirection(Direction direction) {
    direction_ = direction;
  }

  bool hadOverflow() const {
    return hadOverflow_;
  }

  void setHadOverflow(bool hadOverflow) {
    hadOverflow_ = hadOverflow;
  }

  float dimension(Dimension axis) const {
    return dimensions_[iris::ui::to_underlying(axis)];
  }

  void setDimension(Dimension axis, float dimension) {
    dimensions_[iris::ui::to_underlying(axis)] = dimension;
  }

  float measuredDimension(Dimension axis) const {
    return measuredDimensions_[iris::ui::to_underlying(axis)];
  }

  float rawDimension(Dimension axis) const {
    return rawDimensions_[iris::ui::to_underlying(axis)];
  }

  void setMeasuredDimension(Dimension axis, float dimension) {
    measuredDimensions_[iris::ui::to_underlying(axis)] = dimension;
  }

  void setRawDimension(Dimension axis, float dimension) {
    rawDimensions_[iris::ui::to_underlying(axis)] = dimension;
  }

  float position(PhysicalEdge physicalEdge) const {
    return position_[iris::ui::to_underlying(physicalEdge)];
  }

  void setPosition(PhysicalEdge physicalEdge, float dimension) {
    position_[iris::ui::to_underlying(physicalEdge)] = dimension;
  }

  float margin(PhysicalEdge physicalEdge) const {
    return margin_[iris::ui::to_underlying(physicalEdge)];
  }

  void setMargin(PhysicalEdge physicalEdge, float dimension) {
    margin_[iris::ui::to_underlying(physicalEdge)] = dimension;
  }

  float border(PhysicalEdge physicalEdge) const {
    return border_[iris::ui::to_underlying(physicalEdge)];
  }

  void setBorder(PhysicalEdge physicalEdge, float dimension) {
    border_[iris::ui::to_underlying(physicalEdge)] = dimension;
  }

  float padding(PhysicalEdge physicalEdge) const {
    return padding_[iris::ui::to_underlying(physicalEdge)];
  }

  void setPadding(PhysicalEdge physicalEdge, float dimension) {
    padding_[iris::ui::to_underlying(physicalEdge)] = dimension;
  }

  bool operator==(LayoutResults layout) const;
  bool operator!=(LayoutResults layout) const {
    return !(*this == layout);
  }

 private:
  Direction direction_ : bitCount<Direction>() = Direction::Inherit;
  bool hadOverflow_ : 1 = false;

  std::array<float, 2> dimensions_ = {{YGUndefined, YGUndefined}};
  std::array<float, 2> measuredDimensions_ = {{YGUndefined, YGUndefined}};
  std::array<float, 2> rawDimensions_ = {{YGUndefined, YGUndefined}};
  std::array<float, 4> position_ = {};
  std::array<float, 4> margin_ = {};
  std::array<float, 4> border_ = {};
  std::array<float, 4> padding_ = {};
};

} // namespace iris::ui
