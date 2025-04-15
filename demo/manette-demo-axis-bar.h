/* manette-demo-axis-bar.h
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

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define MANETTE_DEMO_TYPE_AXIS_BAR (manette_demo_axis_bar_get_type())

G_DECLARE_FINAL_TYPE (ManetteDemoAxisBar, manette_demo_axis_bar, MANETTE_DEMO, AXIS_BAR, GtkWidget)

GtkWidget *manette_demo_axis_bar_new (gboolean is_trigger);

void manette_demo_axis_bar_set_value (ManetteDemoAxisBar *self,
                                      double              value);

G_END_DECLS
