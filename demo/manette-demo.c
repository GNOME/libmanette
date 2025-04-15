/* manette-demo.c
 *
 * Copyright (C) 2025 Alice Mikhaylenko <alicem@gnome.org>
 *
 * This file is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2.1 of the
 * License, or (at your option) any later version.
 *
 * This file is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <adwaita.h>

#include "manette-demo-window.h"

static void
show_about (GSimpleAction *action,
            GVariant      *state,
            gpointer       user_data)
{
}

static void
quit_app (GSimpleAction *action,
          GVariant      *state,
          gpointer       user_data)
{
  GApplication *app = G_APPLICATION (user_data);

  g_application_quit (app);
}

static void
activate_cb (GApplication *app)
{
  GtkWindow *window = gtk_application_get_active_window (GTK_APPLICATION (app));

  if (!window)
    window = manette_demo_window_new (GTK_APPLICATION (app));

  gtk_window_present (window);
}

int
main (int    argc,
      char **argv)
{
  g_autoptr (AdwApplication) app = NULL;
  static GActionEntry app_entries[] = {
    { "about", show_about, NULL, NULL, NULL },
    { "quit", quit_app, NULL, NULL, NULL },
  };

  app = adw_application_new ("org.gnome.Manette1.Demo", G_APPLICATION_NON_UNIQUE);

  g_action_map_add_action_entries (G_ACTION_MAP (app),
                                   app_entries, G_N_ELEMENTS (app_entries),
                                   app);

  g_signal_connect (app, "activate", G_CALLBACK (activate_cb), NULL);

  return g_application_run (G_APPLICATION (app), argc, argv);
}
