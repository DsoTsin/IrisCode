/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#pragma once

#include <yoga/Yoga.h>

namespace iris::ui {

class Node;
class Config;

[[noreturn]] void fatalWithMessage(const char* message);

void assertFatal(bool condition, const char* message);
void assertFatalWithNode(
    const iris::ui::Node* node,
    bool condition,
    const char* message);
void assertFatalWithConfig(
    const iris::ui::Config* config,
    bool condition,
    const char* message);

} // namespace iris::ui
