/* manette-hid-backend.c
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

#include "manette-hid-backend-private.h"

#include <hidapi.h>
#include <linux/input.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include "drivers/manette-steam-deck-driver-private.h"

#include "manette-device-type-private.h"
#include "manette-hid-driver-private.h"

struct _ManetteHidBackend
{
  GObject parent_instance;

  char *filename;
  hid_device *hid;
  ManetteDeviceType device_type;
  ManetteHidDriver *driver;
  char *name;
  guint event_source_id;
};

static void manette_hid_backend_backend_init (ManetteBackendInterface *iface);

G_DEFINE_FINAL_TYPE_WITH_CODE (ManetteHidBackend, manette_hid_backend, G_TYPE_OBJECT,
                               G_IMPLEMENT_INTERFACE (MANETTE_TYPE_BACKEND, manette_hid_backend_backend_init))

static gboolean
poll_events (ManetteHidBackend *self)
{
  guint64 time;

  g_assert (MANETTE_IS_HID_BACKEND (self));

  time = g_get_monotonic_time ();
  manette_hid_driver_poll (self->driver, time);

  return G_SOURCE_CONTINUE;
}

static void
manette_hid_backend_finalize (GObject *object)
{
  ManetteHidBackend *self = MANETTE_HID_BACKEND (object);

  g_clear_handle_id (&self->event_source_id, g_source_remove);
  g_free (self->driver);
  hid_close (self->hid);
  g_free (self->filename);
  g_free (self->name);

  G_OBJECT_CLASS (manette_hid_backend_parent_class)->finalize (object);
}

static void
manette_hid_backend_class_init (ManetteHidBackendClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = manette_hid_backend_finalize;
}

static void
manette_hid_backend_init (ManetteHidBackend *self)
{
}

static gboolean
manette_hid_backend_initialize (ManetteBackend *backend)
{
  ManetteHidBackend *self = MANETTE_HID_BACKEND (backend);
  const struct hid_device_info *info;
  g_autoptr (GError) error = NULL;
  guint poll_rate;

  self->hid = hid_open_path (self->filename);
  if (!self->hid) {
    g_debug ("Failed to open hid device: %ls", hid_error (NULL));
    return FALSE;
  }

  hid_set_nonblocking (self->hid, 1);

  info = hid_get_device_info (self->hid);
  if (!info) {
    g_debug ("Failed to get device info: %ls", hid_error (self->hid));
    return FALSE;
  }

  self->device_type = manette_device_type_guess (info->vendor_id, info->product_id);

  /* Generic is handled through evdev backend, unsupported is skipped */
  if (self->device_type == MANETTE_DEVICE_GENERIC ||
      self->device_type == MANETTE_DEVICE_UNSUPPORTED) {
    return FALSE;
  }

  switch (self->device_type) {
  case MANETTE_DEVICE_STEAM_DECK:
    self->driver = manette_steam_deck_driver_new (self->hid);
    break;
  default:
    g_assert_not_reached ();
  }

  g_signal_connect_swapped (self->driver, "button-event",
                            G_CALLBACK (manette_backend_emit_button_event), self);
  g_signal_connect_swapped (self->driver, "axis-event",
                            G_CALLBACK (manette_backend_emit_axis_event), self);

  if (!manette_hid_driver_initialize (self->driver))
    return FALSE;

  // Poll the events in the main loop.
  poll_rate = manette_hid_driver_get_poll_rate (self->driver);
  self->event_source_id = g_timeout_add (poll_rate, G_SOURCE_FUNC (poll_events), self);

  return TRUE;
}

static const char *
manette_hid_backend_get_name (ManetteBackend *backend)
{
  ManetteHidBackend *self = MANETTE_HID_BACKEND (backend);

  if (!self->name) {
    char *driver_name = manette_hid_driver_get_name (self->driver);

    if (driver_name) {
      self->name = driver_name;
    } else {
      const struct hid_device_info *info = hid_get_device_info (self->hid);

      self->name = g_strdup_printf ("%ls", info->product_string);
    }
  }

  return self->name;
}

static int
manette_hid_backend_get_vendor_id (ManetteBackend *backend)
{
  ManetteHidBackend *self = MANETTE_HID_BACKEND (backend);
  const struct hid_device_info *info = hid_get_device_info (self->hid);

  return info->vendor_id;
}

static int
manette_hid_backend_get_product_id (ManetteBackend *backend)
{
  ManetteHidBackend *self = MANETTE_HID_BACKEND (backend);
  const struct hid_device_info *info = hid_get_device_info (self->hid);

  return info->product_id;
}

static int
manette_hid_backend_get_bustype_id (ManetteBackend *backend)
{
  ManetteHidBackend *self = MANETTE_HID_BACKEND (backend);
  const struct hid_device_info *info = hid_get_device_info (self->hid);

  switch (info->bus_type) {
  case HID_API_BUS_UNKNOWN:
    return 0;
  case HID_API_BUS_USB:
    return BUS_USB;
  case HID_API_BUS_BLUETOOTH:
    return BUS_BLUETOOTH;
  case HID_API_BUS_I2C:
    return BUS_I2C;
  case HID_API_BUS_SPI:
    return BUS_SPI;
  default:
    g_assert_not_reached ();
  }
}

static int
manette_hid_backend_get_version_id (ManetteBackend *backend)
{
  ManetteHidBackend *self = MANETTE_HID_BACKEND (backend);
  const struct hid_device_info *info = hid_get_device_info (self->hid);

  return info->release_number;
}

void
manette_hid_backend_set_mapping (ManetteBackend *backend,
                                 ManetteMapping *mapping)
{
  g_assert_not_reached ();
}

gboolean
manette_hid_backend_has_button (ManetteBackend *backend,
                                ManetteButton   button)
{
  ManetteHidBackend *self = MANETTE_HID_BACKEND (backend);

  return manette_hid_driver_has_button (self->driver, button);
}

gboolean
manette_hid_backend_has_axis (ManetteBackend *backend,
                              ManetteAxis     axis)
{
  ManetteHidBackend *self = MANETTE_HID_BACKEND (backend);

  return manette_hid_driver_has_axis (self->driver, axis);
}

gboolean
manette_hid_backend_has_input (ManetteBackend *backend,
                               guint           type,
                               guint           code)
{
  return FALSE;
}

static gboolean
manette_hid_backend_has_rumble (ManetteBackend *backend)
{
  ManetteHidBackend *self = MANETTE_HID_BACKEND (backend);

  return manette_hid_driver_has_rumble (self->driver);
}

static gboolean
manette_hid_backend_rumble (ManetteBackend *backend,
                            guint16         strong_magnitude,
                            guint16         weak_magnitude,
                            guint16         milliseconds)
{
  ManetteHidBackend *self = MANETTE_HID_BACKEND (backend);

  return manette_hid_driver_rumble (self->driver,
                                    strong_magnitude,
                                    weak_magnitude,
                                    milliseconds);
}

static void
manette_hid_backend_backend_init (ManetteBackendInterface *iface)
{
  iface->initialize = manette_hid_backend_initialize;
  iface->get_name = manette_hid_backend_get_name;
  iface->get_vendor_id = manette_hid_backend_get_vendor_id;
  iface->get_product_id = manette_hid_backend_get_product_id;
  iface->get_bustype_id = manette_hid_backend_get_bustype_id;
  iface->get_version_id = manette_hid_backend_get_version_id;
  iface->set_mapping = manette_hid_backend_set_mapping;
  iface->has_button = manette_hid_backend_has_button;
  iface->has_axis = manette_hid_backend_has_axis;
  iface->has_input = manette_hid_backend_has_input;
  iface->has_rumble = manette_hid_backend_has_rumble;
  iface->rumble = manette_hid_backend_rumble;
}

ManetteBackend *
manette_hid_backend_new (const char *filename)
{
  ManetteHidBackend *self = g_object_new (MANETTE_TYPE_HID_BACKEND, NULL);

  self->filename = g_strdup (filename);

  return MANETTE_BACKEND (self);
}
