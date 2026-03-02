/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#pragma once

#include <cmath>

#include <yoga/Yoga.h>

#include <yoga/algorithm/SizingMode.h>
#include <yoga/numeric/Comparison.h>

namespace iris::ui {

struct CachedMeasurement {
  float availableWidth{-1};
  float availableHeight{-1};
  SizingMode widthSizingMode{SizingMode::MaxContent};
  SizingMode heightSizingMode{SizingMode::MaxContent};

  float computedWidth{-1};
  float computedHeight{-1};

  bool operator==(CachedMeasurement measurement) const {
    bool isEqual = widthSizingMode == measurement.widthSizingMode &&
        heightSizingMode == measurement.heightSizingMode;

    if (!iris::ui::isUndefined(availableWidth) ||
        !iris::ui::isUndefined(measurement.availableWidth)) {
      isEqual = isEqual && availableWidth == measurement.availableWidth;
    }
    if (!iris::ui::isUndefined(availableHeight) ||
        !iris::ui::isUndefined(measurement.availableHeight)) {
      isEqual = isEqual && availableHeight == measurement.availableHeight;
    }
    if (!iris::ui::isUndefined(computedWidth) ||
        !iris::ui::isUndefined(measurement.computedWidth)) {
      isEqual = isEqual && computedWidth == measurement.computedWidth;
    }
    if (!iris::ui::isUndefined(computedHeight) ||
        !iris::ui::isUndefined(measurement.computedHeight)) {
      isEqual = isEqual && computedHeight == measurement.computedHeight;
    }

    return isEqual;
  }
};

} // namespace iris::ui
