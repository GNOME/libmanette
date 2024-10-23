/* manette-hid-driver.c
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

#include "manette-hid-driver-private.h"

G_DEFINE_INTERFACE (ManetteHidDriver, manette_hid_driver, G_TYPE_OBJECT)

enum {
  SIGNAL_EVENT,
  SIGNAL_LAST_SIGNAL,
};

static guint signals[SIGNAL_LAST_SIGNAL];

static char *
manette_hid_driver_real_get_name (ManetteHidDriver *self)
{
  return NULL;
}

static gboolean
manette_hid_driver_real_has_rumble (ManetteHidDriver *self)
{
  return FALSE;
}

static gboolean
manette_hid_driver_real_rumble (ManetteHidDriver *self,
                                guint16           strong_magnitude,
                                guint16           weak_magnitude,
                                guint16           milliseconds)
{
  return FALSE;
}

static void
manette_hid_driver_default_init (ManetteHidDriverInterface *iface)
{
  iface->get_name = manette_hid_driver_real_get_name;
  iface->has_rumble = manette_hid_driver_real_has_rumble;
  iface->rumble = manette_hid_driver_real_rumble;

  signals[SIGNAL_EVENT] =
    g_signal_new ("event",
                  G_TYPE_FROM_INTERFACE (iface),
                  G_SIGNAL_RUN_FIRST,
                  0,
                  NULL, NULL, NULL,
                  G_TYPE_NONE,
                  1,
                  G_TYPE_POINTER);
}

gboolean
manette_hid_driver_initialize (ManetteHidDriver *self)
{
  ManetteHidDriverInterface *iface;

  g_assert (MANETTE_IS_HID_DRIVER (self));

  iface = MANETTE_HID_DRIVER_GET_IFACE (self);

  g_assert (iface->initialize);

  return iface->initialize (self);
}

guint
manette_hid_driver_get_poll_rate (ManetteHidDriver *self)
{
  ManetteHidDriverInterface *iface;

  g_assert (MANETTE_IS_HID_DRIVER (self));

  iface = MANETTE_HID_DRIVER_GET_IFACE (self);

  g_assert (iface->get_poll_rate);

  return iface->get_poll_rate (self);
}

char *
manette_hid_driver_get_name (ManetteHidDriver *self)
{
  ManetteHidDriverInterface *iface;

  g_assert (MANETTE_IS_HID_DRIVER (self));

  iface = MANETTE_HID_DRIVER_GET_IFACE (self);

  g_assert (iface->get_name);

  return iface->get_name (self);
}

gboolean
manette_hid_driver_has_input (ManetteHidDriver *self,
                              guint             type,
                              guint             code)
{
  ManetteHidDriverInterface *iface;

  g_assert (MANETTE_IS_HID_DRIVER (self));

  iface = MANETTE_HID_DRIVER_GET_IFACE (self);

  g_assert (iface->has_input);

  return iface->has_input (self, type, code);
}

void
manette_hid_driver_poll (ManetteHidDriver *self,
                         gint64            time)
{
  ManetteHidDriverInterface *iface;

  g_assert (MANETTE_IS_HID_DRIVER (self));

  iface = MANETTE_HID_DRIVER_GET_IFACE (self);

  g_assert (iface->poll);

  iface->poll (self, time);
}

gboolean
manette_hid_driver_has_rumble (ManetteHidDriver *self)
{
  ManetteHidDriverInterface *iface;

  g_assert (MANETTE_IS_HID_DRIVER (self));

  iface = MANETTE_HID_DRIVER_GET_IFACE (self);

  g_assert (iface->has_rumble);

  return iface->has_rumble (self);
}

gboolean
manette_hid_driver_rumble (ManetteHidDriver *self,
                           guint16           strong_magnitude,
                           guint16           weak_magnitude,
                           guint16           milliseconds)
{
  ManetteHidDriverInterface *iface;

  g_assert (MANETTE_IS_HID_DRIVER (self));
  g_assert (milliseconds <= G_MAXINT16);

  iface = MANETTE_HID_DRIVER_GET_IFACE (self);

  g_assert (iface->has_rumble);

  return iface->rumble (self, strong_magnitude, weak_magnitude, milliseconds);
}

void
manette_hid_driver_emit_event (ManetteHidDriver *self,
                               ManetteEvent     *event)
{
  g_assert (MANETTE_IS_HID_DRIVER (self));
  g_assert (event != NULL);

  g_signal_emit (self, signals[SIGNAL_EVENT], 0, event);
}
