/* SPDX-FileCopyrightText: 2025 Elad Dvash */
/* SPDX-License-Identifier: Apache-2.0 */

#include "settings_themes.h"
#include "settings_menu.h"
#include "settings_option_menu.h"
#include "settings_window.h"

#include "applib/ui/dialogs/dialog.h"
#include "applib/ui/dialogs/expandable_dialog.h"
#include "applib/graphics/gtypes.h"
#include "applib/graphics/graphics.h"
#include "applib/ui/menu_layer.h"
#include "kernel/pbl_malloc.h"
#include "services/common/i18n/i18n.h"
#include "shell/prefs.h"
#include "system/passert.h"
#include "util/size.h"

/* Per-window data for this settings module. */
typedef struct SettingsThemesData {
  SettingsCallbacks callbacks;
  Window window;
  StatusBarLayer status_layer;
  MenuLayer menu_layer;

  SettingsMenuItem current_category; //!< SettingsMenuItem_Invalid if not currently in a category.

  const char *title;
} SettingsThemesData;


/* Menu row indices for the Themes menu. */
typedef enum ThemesMenuIndex {
  ThemesMenuIndex_Choice = 0,
#if PBL_COLOR
  ThemesMenuIndex_Light,
  ThemesMenuIndex_Dark,
#endif
  ThemesMenuIndexCount
} ThemesMenuIndex;

static const char *mode_names[] = {"Default", "Light", "Dark"};

/* Free i18n strings and allocated context when the menu is torn down. */
static void prv_deinit_cb(SettingsCallbacks *context) {
  i18n_free_all(context);
  app_free(context);
}

static uint16_t prv_num_rows_cb(SettingsCallbacks *context) {
  return ThemesMenuIndexCount;
}

static void prv_draw_row_cb(SettingsCallbacks *context, GContext *ctx,
                            const Layer *cell_layer, uint16_t row, bool selected) {
  SettingsCallbacks *data = (SettingsCallbacks *)context;
  const char *title = NULL;
  const char *subtitle = NULL;

  switch ((ThemesMenuIndex)row) {
    case ThemesMenuIndex_Choice:
      /* Title for the Theme Mode item. */
      title = i18n_noop("Theme Mode");
      subtitle = i18n_noop(mode_names[shell_prefs_get_theme_mode()]);
      break;
#if PBL_COLOR
    case ThemesMenuIndex_Light:
      /* Title for the Apps accent color item. */
      title = i18n_noop("Light Mode Accent");
      break;
    case ThemesMenuIndex_Dark:
      /* Title for the Settings accent color item. */
      title = i18n_noop("Dark Mode Accent");
      break;
#endif
      case ThemesMenuIndexCount:
      break;
  }

  PBL_ASSERTN(title);
  menu_cell_basic_draw(ctx, cell_layer, i18n_get(title, data), i18n_get(subtitle, data), NULL);
}

#if PBL_COLOR
static const char* color_names[ARRAY_LENGTH(s_color_definitions)];
static bool color_names_initialized = false;

static const char** prv_get_color_names() {
  if (!color_names_initialized) {
    for (size_t i = 0; i < ARRAY_LENGTH(s_color_definitions); i++) {
      color_names[i] = (char*)s_color_definitions[i].name;
    }
    color_names_initialized = true;
  }
  return color_names;
}




static int prv_color_to_index(GColor color, bool is_light, ColorDefinition color_definition) {
  GColor default_color;
  default_color = is_light ? color_definition.light : color_definition.dark;
  if (color.argb == GColorClear.argb || color.argb == default_color.argb) {
    return 0;
  }
  for (size_t i = 0; i < ARRAY_LENGTH(s_color_definitions); i++) {
    GColor selected_color = is_light ?  s_color_definitions[i].light : s_color_definitions[i].dark;
    if ((uint8_t)(color.argb) == (uint8_t)(selected_color.argb)) {
      return i;
    }
  }
  return -1;
}


/////////////////////////////
// Light Accent Color Settings
/////////////////////////////

static void prv_light_color_menu_select(OptionMenu *option_menu, int selection, void *context) {
  const ColorDefinition default_color_definition = DEFAULT_COLOR_DEFINITION;
  const ColorDefinition color_definition = selection != 0 ? s_color_definitions[selection] : default_color_definition;

  GColor highlight_color = color_definition.light;
  shell_prefs_set_highlight_color(highlight_color, true);

  app_window_stack_remove(&option_menu->window, true /* animated */);
}

static void prv_option_light_menu_selection_will_change(OptionMenu *option_menu,
                                             uint16_t new_row,
                                             uint16_t old_row,
                                             void *context) {
  if (new_row == old_row) {
    return;
  }
  const ColorDefinition default_color_definition = DEFAULT_COLOR_DEFINITION;
  const ColorDefinition color_definition = new_row != 0 ? s_color_definitions[new_row] : default_color_definition;
  GColor highlight_color = color_definition.light;
  GColor highlight_foreground_color = shell_prefs_get_theme_mode_colors().light_highlight_foreground_color;

  if (highlight_color.argb != GColorClear.argb) {
    option_menu_set_highlight_colors(option_menu, highlight_color, highlight_foreground_color);
  }
  else {
    option_menu_set_highlight_colors(option_menu, default_color_definition.light, highlight_foreground_color);
  }
}

static void prv_push_light_color_menu(SettingsCallbacks *data) {
  const char *title = i18n_noop("Light Mode Accent");
  const ColorDefinition default_color_definition = DEFAULT_COLOR_DEFINITION;
  int selected = prv_color_to_index(shell_prefs_get_theme_mode_colors().light_highlight_color, true, default_color_definition);
  const char** color_names = prv_get_color_names();
  const OptionMenuCallbacks callbacks = {
    .select = prv_light_color_menu_select,
    .selection_will_change = prv_option_light_menu_selection_will_change,
  };
  if (selected < 0) {
    WTF;
  }
  OptionMenu * const option_menu = settings_option_menu_create(
      title, OptionMenuContentType_SingleLine, selected, &callbacks,
      ARRAY_LENGTH(s_color_definitions), true /* icons_enabled */, color_names, data);

  if (option_menu) {
    const bool animated = true;

    GColor normal_background_color = shell_prefs_get_theme_mode_colors().light_screen_background_color;
    GColor normal_foreground_color = shell_prefs_get_theme_mode_colors().light_screen_foreground_color;
    option_menu_set_normal_colors(option_menu, normal_background_color, normal_foreground_color);
    option_menu_set_status_colors(option_menu, normal_background_color, normal_foreground_color);

    const ColorDefinition color_definition = selected != 0 ? s_color_definitions[selected] : default_color_definition;
    GColor highlight_color = color_definition.light;
    GColor highlight_foreground_color = shell_prefs_get_theme_mode_colors().light_highlight_foreground_color;
    option_menu_set_highlight_colors(option_menu, highlight_color, highlight_foreground_color);
    window_set_background_color(&option_menu->window, normal_background_color);
    status_bar_layer_set_colors(&option_menu->status_layer, normal_background_color, normal_foreground_color);

    app_window_stack_push(&option_menu->window, animated);
  }
}

/////////////////////////////
// Dark Accent Color Settings
/////////////////////////////

static void prv_dark_color_menu_select(OptionMenu *option_menu, int selection, void *context) {
  const ColorDefinition default_color_definition = DEFAULT_COLOR_DEFINITION;
  const ColorDefinition color_definition = selection != 0 ? s_color_definitions[selection] : default_color_definition;

  GColor highlight_color = color_definition.dark;
  shell_prefs_set_highlight_color(highlight_color, false);

  app_window_stack_remove(&option_menu->window, true /* animated */);
}

static void prv_option_dark_menu_selection_will_change(OptionMenu *option_menu,
                                             uint16_t new_row,
                                             uint16_t old_row,
                                             void *context) {
  if (new_row == old_row) {
    return;
  }
  const ColorDefinition default_color_definition = DEFAULT_COLOR_DEFINITION;
  const ColorDefinition color_definition = new_row != 0 ? s_color_definitions[new_row] : default_color_definition;

  GColor highlight_color = color_definition.dark;
  GColor highlight_foreground_color = shell_prefs_get_theme_mode_colors().dark_highlight_foreground_color;
  if (highlight_color.argb != GColorClear.argb) {
    option_menu_set_highlight_colors(option_menu, highlight_color, highlight_foreground_color);
  }
  else {
    option_menu_set_highlight_colors(option_menu, default_color_definition.light, highlight_foreground_color);
  }
}

static void prv_push_dark_color_menu(SettingsCallbacks *data) {
  const char *title = i18n_noop("Dark Mode Accent");
  const ColorDefinition default_color_definition = DEFAULT_COLOR_DEFINITION;
  int selected = prv_color_to_index(shell_prefs_get_theme_mode_colors().dark_highlight_color, false, default_color_definition);
  const char** color_names = prv_get_color_names();
  const OptionMenuCallbacks callbacks = {
    .select = prv_dark_color_menu_select,
    .selection_will_change = prv_option_dark_menu_selection_will_change,
  };
  if (selected < 0) {
    WTF;
  }
  OptionMenu * const option_menu = settings_option_menu_create(
      title, OptionMenuContentType_SingleLine, selected, &callbacks,
      ARRAY_LENGTH(s_color_definitions), true /* icons_enabled */, color_names, data);

  if (option_menu) {
    const bool animated = true;
    GColor normal_background_color = shell_prefs_get_theme_mode_colors().dark_screen_background_color;
    GColor normal_foreground_color = shell_prefs_get_theme_mode_colors().dark_screen_foreground_color;
    option_menu_set_normal_colors(option_menu, normal_background_color, normal_foreground_color);
    option_menu_set_status_colors(option_menu, normal_background_color, normal_foreground_color);
    const ColorDefinition color_definition = selected != 0 ? s_color_definitions[selected] : default_color_definition;
    GColor highlight_color = color_definition.dark;
    GColor highlight_foreground_color = shell_prefs_get_theme_mode_colors().dark_highlight_foreground_color;
    option_menu_set_highlight_colors(option_menu, highlight_color, highlight_foreground_color);
    window_set_background_color(&option_menu->window, normal_background_color);
    status_bar_layer_set_colors(&option_menu->status_layer, normal_background_color, normal_foreground_color);
    

    app_window_stack_push(&option_menu->window, animated);
  }
}
#endif

static void prv_select_mode_switch(SettingsCallbacks *data, ThemeMode new_mode) {
  shell_prefs_set_theme_mode(new_mode);
  if (data->appear) {
    data->appear(data);
  }
}

static void prv_select_click_cb(SettingsCallbacks *context, uint16_t row) {
  SettingsCallbacks *data = (SettingsCallbacks *)context;
  switch ((ThemesMenuIndex)row) {
    case ThemesMenuIndex_Choice: {
      ThemeMode current_mode = shell_prefs_get_theme_mode();
      ThemeMode new_mode = (ThemeMode)((current_mode + 1) % ThemeModeCount);
      prv_select_mode_switch(data, new_mode);
      break;
    }
#if PBL_COLOR
    case ThemesMenuIndex_Light: {
      prv_push_light_color_menu(data);
      break;
    }
    case ThemesMenuIndex_Dark: {
      prv_push_dark_color_menu(data);
      break;
    }
#endif
    case ThemesMenuIndexCount:
      break;
  }
  settings_menu_reload_data(SettingsMenuItemThemes);
  settings_menu_mark_dirty(SettingsMenuItemThemes);
}

static Window *prv_create_settings_window(void) {
  SettingsThemesData *data = app_malloc_check(sizeof(*data));

  *data = (SettingsThemesData) {
    .callbacks = {
      .deinit = prv_deinit_cb,
      .draw_row = prv_draw_row_cb,
      .select_click = prv_select_click_cb,
      .num_rows = prv_num_rows_cb,
    }
  };

  return settings_window_create(SettingsMenuItemThemes, &data->callbacks);
}

static Window *prv_init(void) {
  return prv_create_settings_window();
}


const SettingsModuleMetadata *settings_themes_get_info(void) {
  static const SettingsModuleMetadata s_module_info = {
    /// Title of the Themes Settings submenu in Settings
    .name = i18n_noop("Themes"),
    .init = prv_init,
  };

  return &s_module_info;
}
