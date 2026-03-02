/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include <cmath>

#include <yoga/node/LayoutResults.h>
#include <yoga/numeric/Comparison.h>

namespace iris::ui {

bool LayoutResults::operator==(LayoutResults layout) const {
  bool isEqual = iris::ui::inexactEquals(position_, layout.position_) &&
      iris::ui::inexactEquals(dimensions_, layout.dimensions_) &&
      iris::ui::inexactEquals(margin_, layout.margin_) &&
      iris::ui::inexactEquals(border_, layout.border_) &&
      iris::ui::inexactEquals(padding_, layout.padding_) &&
      direction() == layout.direction() &&
      hadOverflow() == layout.hadOverflow() &&
      lastOwnerDirection == layout.lastOwnerDirection &&
      configVersion == layout.configVersion &&
      nextCachedMeasurementsIndex == layout.nextCachedMeasurementsIndex &&
      cachedLayout == layout.cachedLayout &&
      computedFlexBasis == layout.computedFlexBasis;

  for (uint32_t i = 0; i < LayoutResults::MaxCachedMeasurements && isEqual;
       ++i) {
    isEqual = isEqual && cachedMeasurements[i] == layout.cachedMeasurements[i];
  }

  if (!iris::ui::isUndefined(measuredDimensions_[0]) ||
      !iris::ui::isUndefined(layout.measuredDimensions_[0])) {
    isEqual =
        isEqual && (measuredDimensions_[0] == layout.measuredDimensions_[0]);
  }
  if (!iris::ui::isUndefined(measuredDimensions_[1]) ||
      !iris::ui::isUndefined(layout.measuredDimensions_[1])) {
    isEqual =
        isEqual && (measuredDimensions_[1] == layout.measuredDimensions_[1]);
  }

  return isEqual;
}

} // namespace iris::ui
