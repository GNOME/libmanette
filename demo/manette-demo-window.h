/* manette-demo-window.h
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

#pragma once

#include <adwaita.h>

G_BEGIN_DECLS

#define MANETTE_DEMO_TYPE_WINDOW (manette_demo_window_get_type())

G_DECLARE_FINAL_TYPE (ManetteDemoWindow, manette_demo_window, MANETTE_DEMO, WINDOW, AdwApplicationWindow)

GtkWindow *manette_demo_window_new (GtkApplication *app);

G_END_DECLS
