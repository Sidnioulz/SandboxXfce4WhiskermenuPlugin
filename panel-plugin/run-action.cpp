/*
 * Copyright (C) 2013, 2015 Graeme Gott <graeme@gottcode.org>
 *
 * This library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this library.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "run-action.h"

#include "query.h"
#include "launcher.h"
#include "window.h"
#include "applications-page.h"
#include "settings.h"

#include <garcon/garcon.h>
#include <libxfce4ui/libxfce4ui.h>

using namespace WhiskerMenu;

//-----------------------------------------------------------------------------

RunAction::RunAction(Window* window) :
  m_window(window)
{
	set_icon("system-run");
}

//-----------------------------------------------------------------------------

void RunAction::run(GdkScreen* screen) const
{
  gboolean          result   = FALSE;
	GError*           error    = NULL;
  std::string       app_name;
  size_t            pos;
  GarconMenuItem   *item     = NULL;
  ApplicationsPage *apps     = NULL;
  Launcher         *launcher = NULL;
	
  // get the app's name
	app_name = m_command_line;
	pos = app_name.find_first_of(' ');
	if (pos != std::string::npos && pos > 0)
	  app_name.erase(pos);
  app_name += ".desktop";

  // find out if it ought to be sandboxed
  if (!m_window)
	{
		xfce_dialog_show_error_manual(NULL, _("Missing window"), _("Failed to execute command \"%s\"."), m_command_line.c_str());
		return;
	}

  apps = m_window->get_applications();
  if (!apps)
	{
		xfce_dialog_show_error_manual(NULL, _("Missing applications page"), _("Failed to execute command \"%s\"."), m_command_line.c_str());
		return;
	}

  launcher = apps->get_application(app_name);

  item = launcher? launcher->get_garcon_menu_item():NULL;
  if (item && garcon_menu_item_get_sandboxed(item))
  {
    gchar *garcon_command = garcon_menu_item_expand_command(item,
                                                            m_command_line.c_str(),
                                                            xfce_workspace_is_active_secure(screen));
	  result = xfce_spawn_command_line_on_screen(screen,
	                                             garcon_command,
                                               FALSE,
                                               garcon_menu_item_supports_startup_notification (item),
                                               &error);
    g_free(garcon_command);
  }
  else
  {
	  result = xfce_spawn_command_line_on_screen(screen, m_command_line.c_str(), FALSE, FALSE, &error);
  }

  if (item)
    g_object_unref(item);

	if (G_UNLIKELY(!result))
	{
		xfce_dialog_show_error(NULL, error, _("Failed to execute command \"%s\"."), m_command_line.c_str());
		g_error_free(error);
	}	
}

//-----------------------------------------------------------------------------

guint RunAction::search(const Query& query)
{
	// Check if in PATH
	bool valid = false;

	gchar** argv;
	if (g_shell_parse_argv(query.raw_query().c_str(), NULL, &argv, NULL))
	{
		gchar* path = g_find_program_in_path(argv[0]);
		valid = path != NULL;
		g_free(path);
		g_strfreev(argv);
	}

	if (!valid)
	{
		return G_MAXUINT;
	}

	m_command_line = query.raw_query();

	// Set item text
	const gchar* direction = (gtk_widget_get_default_direction() != GTK_TEXT_DIR_RTL) ? "\342\200\216" : "\342\200\217";
	gchar* display_name = g_strdup_printf(_("Run %s"), m_command_line.c_str());
	if (wm_settings->launcher_show_description)
	{
		set_text(g_markup_printf_escaped("%s<b>%s</b>\n", direction, display_name));
	}
	else
	{
		set_text(g_markup_printf_escaped("%s%s", direction, display_name));
	}
	g_free(display_name);

	// Sort after matches in names and before matches in executables
	return 0xFFF;
}

//-----------------------------------------------------------------------------
