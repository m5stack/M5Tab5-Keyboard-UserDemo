/*
 * SPDX-FileCopyrightText: 2025-2026 M5Stack
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef APP_BUILD_INFO_H
#define APP_BUILD_INFO_H

#ifndef FIRMWARE_VERSION
#define FIRMWARE_VERSION "0.0.0-dev"
#endif

#ifndef FIRMWARE_BUILD_TIME
#define FIRMWARE_BUILD_TIME __DATE__ "_" __TIME__
#endif

#endif  // APP_BUILD_INFO_H