/* SPDX-FileCopyrightText: 2025 Elad Dvash */
/* SPDX-License-Identifier: Apache-2.0 */

#pragma once
#include "settings_menu.h"
#include "shell/system_theme.h"

#ifndef DEFAULT_COLOR_DEFINITION
#define DEFAULT_COLOR_DEFINITION {\
  .name = "Default",\
  .light = PBL_IF_COLOR_ELSE(GColorVividCerulean, GColorWhite),\
  .dark = PBL_IF_COLOR_ELSE(GColorCobaltBlue, GColorWhite)};
#endif

static const ColorDefinition s_color_definitions[11] = {
  {"Default", GColorClear},
  {"Red", GColorSunsetOrange, GColorDarkCandyAppleRed},
  {"Orange", GColorChromeYellow, GColorWindsorTan},
  {"Yellow", GColorYellow, GColorArmyGreen},
  {"Green", GColorGreen, GColorDarkGreen},
  {"Cyan", GColorCyan, GColorMidnightGreen},
  {"Light Blue", GColorVividCerulean, GColorCobaltBlue},
  {"Royal Blue", GColorVeryLightBlue, GColorDukeBlue},
  {"Purple", GColorLavenderIndigo, GColorIndigo},
  {"Magenta", GColorMagenta, GColorPurple},
  {"Pink", GColorBrilliantRose, GColorJazzberryJam},
};
const SettingsModuleMetadata *settings_themes_get_info(void);
