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
  SIGNAL_EVENT,
  SIGNAL_LAST_SIGNAL,
};

static guint signals[SIGNAL_LAST_SIGNAL];

static void
manette_backend_default_init (ManetteBackendInterface *iface)
{
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
manette_backend_emit_event (ManetteBackend *self,
                            ManetteEvent   *event)
{
  g_assert (MANETTE_IS_BACKEND (self));
  g_assert (event != NULL);

  g_signal_emit (self, signals[SIGNAL_EVENT], 0, event);
}
