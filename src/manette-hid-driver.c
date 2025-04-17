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

#include "config.h"

#include "manette-hid-driver-private.h"

G_DEFINE_INTERFACE (ManetteHidDriver, manette_hid_driver, G_TYPE_OBJECT)

enum {
  SIGNAL_BUTTON_EVENT,
  SIGNAL_AXIS_EVENT,
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

  signals[SIGNAL_BUTTON_EVENT] =
    g_signal_new ("button-event",
                  G_TYPE_FROM_INTERFACE (iface),
                  G_SIGNAL_RUN_FIRST,
                  0,
                  NULL, NULL, NULL,
                  G_TYPE_NONE,
                  3,
                  G_TYPE_UINT64, G_TYPE_UINT, G_TYPE_BOOLEAN);

  signals[SIGNAL_AXIS_EVENT] =
    g_signal_new ("axis-event",
                  G_TYPE_FROM_INTERFACE (iface),
                  G_SIGNAL_RUN_FIRST,
                  0,
                  NULL, NULL, NULL,
                  G_TYPE_NONE,
                  3,
                  G_TYPE_UINT64, MANETTE_TYPE_AXIS, G_TYPE_DOUBLE);
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
manette_hid_driver_has_button (ManetteHidDriver *self,
                               ManetteButton     button)
{
  ManetteHidDriverInterface *iface;

  g_assert (MANETTE_IS_HID_DRIVER (self));

  iface = MANETTE_HID_DRIVER_GET_IFACE (self);

  g_assert (iface->has_button);

  return iface->has_button (self, button);
}

gboolean
manette_hid_driver_has_axis (ManetteHidDriver *self,
                             ManetteAxis       axis)
{
  ManetteHidDriverInterface *iface;

  g_assert (MANETTE_IS_HID_DRIVER (self));

  iface = MANETTE_HID_DRIVER_GET_IFACE (self);

  g_assert (iface->has_axis);

  return iface->has_axis (self, axis);
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
manette_hid_driver_emit_button_event (ManetteHidDriver *self,
                                      guint64           time,
                                      ManetteButton     button,
                                      gboolean          pressed)
{
  g_assert (MANETTE_IS_HID_DRIVER (self));

  pressed = !!pressed;

  g_signal_emit (self, signals[SIGNAL_BUTTON_EVENT], 0, time, button, pressed);
}

void
manette_hid_driver_emit_axis_event (ManetteHidDriver *self,
                                    guint64           time,
                                    ManetteAxis       axis,
                                    double            value)
{
  g_assert (MANETTE_IS_HID_DRIVER (self));

  g_signal_emit (self, signals[SIGNAL_AXIS_EVENT], 0, time, axis, value);
}
