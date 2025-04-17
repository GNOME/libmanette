/* manette-backend.c
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

#include "manette-backend-private.h"

G_DEFINE_INTERFACE (ManetteBackend, manette_backend, G_TYPE_OBJECT)

enum {
  SIGNAL_BUTTON_EVENT,
  SIGNAL_AXIS_EVENT,
  SIGNAL_UNMAPPED_BUTTON_EVENT,
  SIGNAL_UNMAPPED_ABSOLUTE_EVENT,
  SIGNAL_UNMAPPED_HAT_EVENT,
  SIGNAL_LAST_SIGNAL,
};

static guint signals[SIGNAL_LAST_SIGNAL];

static void
manette_backend_default_init (ManetteBackendInterface *iface)
{
  signals[SIGNAL_BUTTON_EVENT] =
    g_signal_new ("button-event",
                  G_TYPE_FROM_INTERFACE (iface),
                  G_SIGNAL_RUN_FIRST,
                  0,
                  NULL, NULL, NULL,
                  G_TYPE_NONE,
                  3,
                  G_TYPE_UINT64, MANETTE_TYPE_BUTTON, G_TYPE_BOOLEAN);

  signals[SIGNAL_AXIS_EVENT] =
    g_signal_new ("axis-event",
                  G_TYPE_FROM_INTERFACE (iface),
                  G_SIGNAL_RUN_FIRST,
                  0,
                  NULL, NULL, NULL,
                  G_TYPE_NONE,
                  3,
                  G_TYPE_UINT64, MANETTE_TYPE_AXIS, G_TYPE_DOUBLE);

  signals[SIGNAL_UNMAPPED_BUTTON_EVENT] =
    g_signal_new ("unmapped-button-event",
                  G_TYPE_FROM_INTERFACE (iface),
                  G_SIGNAL_RUN_FIRST,
                  0,
                  NULL, NULL, NULL,
                  G_TYPE_NONE,
                  3,
                  G_TYPE_UINT64, G_TYPE_UINT, G_TYPE_BOOLEAN);

  signals[SIGNAL_UNMAPPED_ABSOLUTE_EVENT] =
    g_signal_new ("unmapped-absolute-event",
                  G_TYPE_FROM_INTERFACE (iface),
                  G_SIGNAL_RUN_FIRST,
                  0,
                  NULL, NULL, NULL,
                  G_TYPE_NONE,
                  3,
                  G_TYPE_UINT64, G_TYPE_UINT, G_TYPE_DOUBLE);

  signals[SIGNAL_UNMAPPED_HAT_EVENT] =
    g_signal_new ("unmapped-hat-event",
                  G_TYPE_FROM_INTERFACE (iface),
                  G_SIGNAL_RUN_FIRST,
                  0,
                  NULL, NULL, NULL,
                  G_TYPE_NONE,
                  3,
                  G_TYPE_UINT64, G_TYPE_UINT, G_TYPE_CHAR);
}

gboolean
manette_backend_initialize (ManetteBackend *self)
{
  ManetteBackendInterface *iface;

  g_assert (MANETTE_IS_BACKEND (self));

  iface = MANETTE_BACKEND_GET_IFACE (self);

  g_assert (iface->initialize);

  return iface->initialize (self);
}

const char *
manette_backend_get_name (ManetteBackend *self)
{
  ManetteBackendInterface *iface;

  g_assert (MANETTE_IS_BACKEND (self));

  iface = MANETTE_BACKEND_GET_IFACE (self);

  g_assert (iface->get_name);

  return iface->get_name (self);
}

int
manette_backend_get_vendor_id (ManetteBackend *self)
{
  ManetteBackendInterface *iface;

  g_assert (MANETTE_IS_BACKEND (self));

  iface = MANETTE_BACKEND_GET_IFACE (self);

  g_assert (iface->get_vendor_id);

  return iface->get_vendor_id (self);
}

int
manette_backend_get_product_id (ManetteBackend *self)
{
  ManetteBackendInterface *iface;

  g_assert (MANETTE_IS_BACKEND (self));

  iface = MANETTE_BACKEND_GET_IFACE (self);

  g_assert (iface->get_product_id);

  return iface->get_product_id (self);
}

int
manette_backend_get_bustype_id (ManetteBackend *self)
{
  ManetteBackendInterface *iface;

  g_assert (MANETTE_IS_BACKEND (self));

  iface = MANETTE_BACKEND_GET_IFACE (self);

  g_assert (iface->get_bustype_id);

  return iface->get_bustype_id (self);
}

int
manette_backend_get_version_id (ManetteBackend *self)
{
  ManetteBackendInterface *iface;

  g_assert (MANETTE_IS_BACKEND (self));

  iface = MANETTE_BACKEND_GET_IFACE (self);

  g_assert (iface->get_version_id);

  return iface->get_version_id (self);
}

void
manette_backend_set_mapping (ManetteBackend *self,
                             ManetteMapping *mapping)
{
  ManetteBackendInterface *iface;

  g_assert (MANETTE_IS_BACKEND (self));
  g_assert (mapping == NULL || MANETTE_IS_MAPPING (mapping));

  iface = MANETTE_BACKEND_GET_IFACE (self);

  g_assert (iface->set_mapping);

  iface->set_mapping (self, mapping);
}

gboolean
manette_backend_has_button (ManetteBackend *self,
                            ManetteButton   button)
{
  ManetteBackendInterface *iface;

  g_assert (MANETTE_IS_BACKEND (self));

  iface = MANETTE_BACKEND_GET_IFACE (self);

  g_assert (iface->has_button);

  return iface->has_button (self, button);
}

gboolean
manette_backend_has_axis (ManetteBackend *self,
                          ManetteAxis     axis)
{
  ManetteBackendInterface *iface;

  g_assert (MANETTE_IS_BACKEND (self));

  iface = MANETTE_BACKEND_GET_IFACE (self);

  g_assert (iface->has_axis);

  return iface->has_axis (self, axis);
}

gboolean
manette_backend_has_input (ManetteBackend *self,
                           guint           type,
                           guint           code)
{
  ManetteBackendInterface *iface;

  g_assert (MANETTE_IS_BACKEND (self));

  iface = MANETTE_BACKEND_GET_IFACE (self);

  g_assert (iface->has_input);

  return iface->has_input (self, type, code);
}

gboolean
manette_backend_has_rumble (ManetteBackend *self)
{
  ManetteBackendInterface *iface;

  g_assert (MANETTE_IS_BACKEND (self));

  iface = MANETTE_BACKEND_GET_IFACE (self);

  g_assert (iface->has_rumble);

  return iface->has_rumble (self);
}

gboolean
manette_backend_rumble (ManetteBackend *self,
                        guint16         strong_magnitude,
                        guint16         weak_magnitude,
                        guint16         milliseconds)
{
  ManetteBackendInterface *iface;

  g_assert (MANETTE_IS_BACKEND (self));
  g_assert (milliseconds <= G_MAXINT16);

  iface = MANETTE_BACKEND_GET_IFACE (self);

  g_assert (iface->has_rumble);

  return iface->rumble (self, strong_magnitude, weak_magnitude, milliseconds);
}

void
manette_backend_emit_button_event (ManetteBackend *self,
                                   guint64         time,
                                   ManetteButton   button,
                                   gboolean        pressed)
{
  g_assert (MANETTE_IS_BACKEND (self));

  pressed = !!pressed;

  g_signal_emit (self, signals[SIGNAL_BUTTON_EVENT], 0, time, button, pressed);
}

void
manette_backend_emit_axis_event (ManetteBackend *self,
                                 guint64         time,
                                 ManetteAxis     axis,
                                 double          value)
{
  g_assert (MANETTE_IS_BACKEND (self));

  g_signal_emit (self, signals[SIGNAL_AXIS_EVENT], 0, time, axis, value);
}

void
manette_backend_emit_unmapped_button_event (ManetteBackend *self,
                                            guint64         time,
                                            guint           index,
                                            gboolean        pressed)
{
  g_assert (MANETTE_IS_BACKEND (self));

  pressed = !!pressed;

  g_signal_emit (self, signals[SIGNAL_UNMAPPED_BUTTON_EVENT], 0, time, index, pressed);
}

void
manette_backend_emit_unmapped_absolute_event (ManetteBackend *self,
                                              guint64         time,
                                              guint           index,
                                              double          value)
{
  g_assert (MANETTE_IS_BACKEND (self));

  g_signal_emit (self, signals[SIGNAL_UNMAPPED_ABSOLUTE_EVENT], 0, time, index, value);
}

void
manette_backend_emit_unmapped_hat_event (ManetteBackend *self,
                                         guint64         time,
                                         guint           index,
                                         gint8           value)
{
  g_assert (MANETTE_IS_BACKEND (self));

  g_signal_emit (self, signals[SIGNAL_UNMAPPED_HAT_EVENT], 0, time, index, value);
}
