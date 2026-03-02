/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#pragma once

#include <yoga/enums/Edge.h>

namespace iris::ui {

enum class PhysicalEdge : uint32_t {
  Left = iris::ui::to_underlying(Edge::Left),
  Top = iris::ui::to_underlying(Edge::Top),
  Right = iris::ui::to_underlying(Edge::Right),
  Bottom = iris::ui::to_underlying(Edge::Bottom),
};

} // namespace iris::ui
