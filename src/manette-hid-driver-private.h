/* manette-hid-driver-private.h
 *
 * Copyright (C) 2024 Alice Mikhaylenko <alicem@gnome.org>
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

#if !defined(MANETTE_COMPILATION)
# error "This file is private, only <libmanette.h> can be included directly."
#endif

#include <glib-object.h>

#include "manette-event-private.h"

G_BEGIN_DECLS

#define MANETTE_TYPE_HID_DRIVER (manette_hid_driver_get_type ())

G_DECLARE_INTERFACE (ManetteHidDriver, manette_hid_driver, MANETTE, HID_DRIVER, GObject)

struct _ManetteHidDriverInterface
{
  GTypeInterface parent;

  gboolean (* initialize) (ManetteHidDriver *self);

  guint (* get_poll_rate) (ManetteHidDriver *self);

  char * (* get_name) (ManetteHidDriver *self);

  gboolean (* has_input) (ManetteHidDriver *self,
                          guint             type,
                          guint             code);

  void (* poll) (ManetteHidDriver *self,
                 gint64            time);

  gboolean (* has_rumble) (ManetteHidDriver *self);
  gboolean (* rumble)     (ManetteHidDriver *self,
                           guint16           strong_magnitude,
                           guint16           weak_magnitude,
                           guint16           milliseconds);
};

gboolean manette_hid_driver_initialize (ManetteHidDriver *self);

guint manette_hid_driver_get_poll_rate (ManetteHidDriver *self);

char *manette_hid_driver_get_name (ManetteHidDriver *self);

gboolean manette_hid_driver_has_input (ManetteHidDriver *self,
                                       guint             type,
                                       guint             code);

void manette_hid_driver_poll (ManetteHidDriver *self,
                              gint64            time);

gboolean manette_hid_driver_has_rumble (ManetteHidDriver *self);

gboolean manette_hid_driver_rumble (ManetteHidDriver *self,
                                    guint16           strong_magnitude,
                                    guint16           weak_magnitude,
                                    guint16           milliseconds);

void manette_hid_driver_emit_event (ManetteHidDriver *self,
                                    ManetteEvent     *event);

G_END_DECLS
