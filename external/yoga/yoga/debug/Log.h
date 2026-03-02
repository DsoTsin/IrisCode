/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#pragma once

#include <yoga/Yoga.h>

#include <yoga/config/Config.h>
#include <yoga/enums/LogLevel.h>
#include <yoga/node/Node.h>

namespace iris::ui {

void log(LogLevel level, const char* format, ...) noexcept;

void log(
    const iris::ui::Node* node,
    LogLevel level,
    const char* format,
    ...) noexcept;

void log(
    const iris::ui::Config* config,
    LogLevel level,
    const char* format,
    ...) noexcept;

YGLogger getDefaultLogger();

} // namespace iris::ui
