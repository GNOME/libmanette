/* manette-device.c
 *
 * Copyright (C) 2017 Adrien Plazas <kekun.plazas@laposte.net>
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

#include "manette-device-private.h"

#include <errno.h>
#include <fcntl.h>
#include <linux/input-event-codes.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "manette-backend-private.h"
#include "manette-device-type-private.h"
#include "manette-event-mapping-private.h"
#include "manette-event-private.h"
#include "manette-mapping-manager-private.h"

/**
 * ManetteDevice:
 *
 * An object representing a physical gamepad.
 *
 * See also: [class@Monitor].
 */

struct _ManetteDevice
{
  GObject parent_instance;

  char *guid;
  ManetteMapping *mapping;
  ManetteBackend *backend;

  ManetteDeviceType device_type;

  guint32 current_event_time;
};

G_DEFINE_FINAL_TYPE (ManetteDevice, manette_device, G_TYPE_OBJECT)

enum {
  SIG_EVENT,
  SIG_DISCONNECTED,
  SIG_BUTTON_PRESS_EVENT,
  SIG_BUTTON_RELEASE_EVENT,
  SIG_ABSOLUTE_AXIS_EVENT,
  N_SIGNALS,
};

static guint signals[N_SIGNALS];

/* Private */

static void
forward_event (ManetteDevice *self,
               ManetteEvent  *event)
{
  switch (manette_event_get_event_type (event)) {
  case MANETTE_EVENT_BUTTON_PRESS:
    g_signal_emit (self, signals[SIG_BUTTON_PRESS_EVENT], 0,
                   (guint) event->button.button);
    break;
  case MANETTE_EVENT_BUTTON_RELEASE:
    g_signal_emit (self, signals[SIG_BUTTON_RELEASE_EVENT], 0,
                   (guint) event->button.button);
    break;
  case MANETTE_EVENT_ABSOLUTE:
    g_signal_emit (self, signals[SIG_ABSOLUTE_AXIS_EVENT], 0,
                   (guint) event->absolute.axis,
                   event->absolute.value);
    break;
  default:
    break;
  }
}

static void
map_event (ManetteDevice *self,
           ManetteEvent  *event)
{
  GSList *mapped_events = manette_map_event (self->mapping, event);
  GSList *l = NULL;

  for (l = mapped_events; l != NULL; l = l ->next)
    forward_event (self, l->data);

  g_slist_free_full (mapped_events, (GDestroyNotify) manette_event_free);
}

static void
manette_device_finalize (GObject *object)
{
  ManetteDevice *self = (ManetteDevice *)object;

  g_clear_pointer (&self->guid, g_free);
  g_clear_object (&self->mapping);
  g_clear_object (&self->backend);

  G_OBJECT_CLASS (manette_device_parent_class)->finalize (object);
}

static void
manette_device_class_init (ManetteDeviceClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = manette_device_finalize;

  /**
   * ManetteDevice::event:
   * @self: a device
   * @event: the event emitted by the device
   *
   * Emitted for any kind of event before mapping it.
   */
  signals[SIG_EVENT] =
    g_signal_new ("event",
                  MANETTE_TYPE_DEVICE,
                  G_SIGNAL_RUN_LAST,
                  0, NULL, NULL,
                  g_cclosure_marshal_VOID__BOXED,
                  G_TYPE_NONE, 1,
                  MANETTE_TYPE_EVENT | G_SIGNAL_TYPE_STATIC_SCOPE);

  /**
   * ManetteDevice::button-press-event:
   * @self: a device
   * @button: the button hardware code
   *
   * Emitted when a button is pressed.
   */
  signals[SIG_BUTTON_PRESS_EVENT] =
    g_signal_new ("button-press-event",
                  MANETTE_TYPE_DEVICE,
                  G_SIGNAL_RUN_LAST,
                  0, NULL, NULL,
                  g_cclosure_marshal_VOID__UINT,
                  G_TYPE_NONE, 1,
                  G_TYPE_UINT);

  /**
   * ManetteDevice::button-release-event:
   * @self: a device
   * @button: the button hardware code
   *
   * Emitted when a button is released.
   */
  signals[SIG_BUTTON_RELEASE_EVENT] =
    g_signal_new ("button-release-event",
                  MANETTE_TYPE_DEVICE,
                  G_SIGNAL_RUN_LAST,
                  0, NULL, NULL,
                  g_cclosure_marshal_VOID__UINT,
                  G_TYPE_NONE, 1,
                  G_TYPE_UINT);

  /**
   * ManetteDevice::absolute-axis-event:
   * @self: a device
   * @axis: the axis hardware code
   * @value: the axis value
   *
   * Emitted when an absolute axis' value changes.
   */
  signals[SIG_ABSOLUTE_AXIS_EVENT] =
    g_signal_new ("absolute-axis-event",
                  MANETTE_TYPE_DEVICE,
                  G_SIGNAL_RUN_LAST,
                  0, NULL, NULL,
                  NULL,
                  G_TYPE_NONE, 2,
                  G_TYPE_UINT, G_TYPE_DOUBLE);

  /**
   * ManetteDevice::disconnected:
   * @self: a device
   *
   * Emitted when the device is disconnected.
   */
  signals[SIG_DISCONNECTED] =
    g_signal_new ("disconnected",
                  MANETTE_TYPE_DEVICE,
                  G_SIGNAL_RUN_LAST,
                  0, NULL, NULL,
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);
}

static void
manette_device_init (ManetteDevice *self)
{
}

static char *
compute_guid_string (ManetteDevice *self)
{
  return g_strdup_printf ("%08x%08x%08x%08x",
                          GINT_TO_BE (manette_device_get_bustype_id (self)),
                          GINT_TO_BE (manette_device_get_vendor_id (self)),
                          GINT_TO_BE (manette_device_get_product_id (self)),
                          GINT_TO_BE (manette_device_get_version_id (self)));
}

static void
event_cb (ManetteDevice *self,
          ManetteEvent  *event)
{
  self->current_event_time = event->any.time;

  // Send the unmapped event first.
  g_signal_emit (self, signals[SIG_EVENT], 0, event);

  // Then map or forward the event using dedicated signals.
  if (self->mapping == NULL)
    forward_event (self, event);
  else
    map_event (self, event);
}

/**
 * manette_device_new:
 * @filename: the filename of the device
 * @error: return location for an error
 *
 * Creates a new `ManetteDevice`.
 *
 * Returns: (transfer full): the new device
 */
ManetteDevice *
manette_device_new (ManetteBackend  *backend,
                    GError         **error)
{
  g_autoptr (ManetteDevice) self = NULL;
  int vendor, product;

  g_return_val_if_fail (MANETTE_IS_BACKEND (backend), NULL);

  self = g_object_new (MANETTE_TYPE_DEVICE, NULL);

  self->backend = backend;

  vendor = manette_device_get_vendor_id (self);
  product = manette_device_get_product_id (self);

  self->device_type = manette_device_type_guess (vendor, product);

  g_signal_connect_swapped (self->backend, "event", G_CALLBACK (event_cb), self);

  return g_steal_pointer (&self);
}

/**
 * manette_device_get_guid:
 * @self: a device
 *
 * Gets the identifier used by SDL mappings to discriminate game controller
 * devices.
 *
 * Returns: (transfer none): the identifier used by SDL mappings
 */
const char *
manette_device_get_guid (ManetteDevice *self)
{
  g_return_val_if_fail (MANETTE_IS_DEVICE (self), NULL);

  if (self->guid == NULL)
    self->guid = compute_guid_string (self);

  return self->guid;
}

/**
 * manette_device_has_input:
 * @self: a device
 * @type: the input type
 * @code: the input code
 *
 * Gets whether the device has the given input.
 *
 * If the input is present, it means that the device can send events for it
 * regardless of whether the device is mapped or not.
 *
 * Returns: whether the device has the given input
 */
gboolean
manette_device_has_input (ManetteDevice *self,
                          guint          type,
                          guint          code)
{
  g_return_val_if_fail (MANETTE_IS_DEVICE (self), FALSE);

  return MANETTE_IS_MAPPING (self->mapping) ?
    manette_mapping_has_destination_input (self->mapping, type, code) :
    manette_backend_has_input (self->backend, type, code);
}

/**
 * manette_device_get_name:
 * @self: a device
 *
 * Gets the device's name.
 *
 * Returns: (transfer none): the name of @self
 */
const char *
manette_device_get_name (ManetteDevice *self)
{
  g_return_val_if_fail (MANETTE_IS_DEVICE (self), NULL);

  return manette_backend_get_name (self->backend);
}

/**
 * manette_device_get_product_id:
 * @self: a device
 *
 * Gets the device's product ID.
 *
 * You can find some product IDs defined in the kernel's code, in
 * `drivers/hid/hid-ids.h`. Just note that there isn't all devices there, but
 * sufficiently enough most of the time.
 *
 * Returns: the product ID of @self
 */
int
manette_device_get_product_id (ManetteDevice *self)
{
  g_return_val_if_fail (MANETTE_IS_DEVICE (self), 0);

  return manette_backend_get_product_id (self->backend);
}

/**
 * manette_device_get_vendor_id:
 * @self: a device
 *
 * Gets the device's vendor ID.
 *
 * You can find some vendor IDs defined in the kernel's code, in
 * `drivers/hid/hid-ids.h`. Just note that there isn't all devices there, but
 * sufficiently enough most of the time.
 *
 * Returns: the vendor ID of @self
 */
int
manette_device_get_vendor_id (ManetteDevice *self)
{
  g_return_val_if_fail (MANETTE_IS_DEVICE (self), 0);

  return manette_backend_get_vendor_id (self->backend);
}

/**
 * manette_device_get_bustype_id:
 * @self: a device
 *
 * Gets the device's bustype ID.
 *
 * This corresponds to BUS_* as defined in `linux/input.h`
 *
 * Returns: the bustype ID of @self
 */
int
manette_device_get_bustype_id (ManetteDevice *self)
{
  g_return_val_if_fail (MANETTE_IS_DEVICE (self), 0);

  return manette_backend_get_bustype_id (self->backend);
}

/**
 * manette_device_get_version_id:
 * @self: a device
 *
 * Gets the device's version ID.
 *
 * Returns: the version ID of @self
 */
int
manette_device_get_version_id (ManetteDevice *self)
{
  g_return_val_if_fail (MANETTE_IS_DEVICE (self), 0);

  return manette_backend_get_version_id (self->backend);
}

/**
 * manette_device_get_device_type:
 * @self: a device
 *
 * Gets the device type of @self.
 *
 * Returns: the device type
 */
ManetteDeviceType
manette_device_get_device_type (ManetteDevice *self)
{
  g_return_val_if_fail (MANETTE_IS_DEVICE (self), MANETTE_DEVICE_GENERIC);

  return self->device_type;
}

/**
 * manette_device_get_current_event_time:
 * @self: a device
 *
 * Gets the timestamp of when the current event was emitted on @self.
 *
 * Use this timestamp to ensure external factors such as synchronous disk writes
 * don't influence your timing computations.
 *
 * Returns: the timestamp of when the current event was emitted
 */
guint32
manette_device_get_current_event_time (ManetteDevice *self)
{
  g_return_val_if_fail (MANETTE_IS_DEVICE (self), 0);

  return self->current_event_time;
}

/**
 * manette_device_supports_mapping:
 * @self: a #ManetteDevice
 *
 * Gets whether @self supports mapping.
 *
 * Returns: whether @self supports mapping
 */
gboolean
manette_device_supports_mapping (ManetteDevice *self)
{
  g_return_val_if_fail (MANETTE_IS_DEVICE (self), FALSE);

  return self->device_type == MANETTE_DEVICE_GENERIC;
}

/**
 * manette_device_set_mapping:
 * @self: a device
 * @mapping: a mapping
 *
 * Associate @mapping to @self to map its hardware events into standard gamepad
 * ones.
 */
void
manette_device_set_mapping (ManetteDevice  *self,
                            ManetteMapping *mapping)
{
  g_return_if_fail (MANETTE_IS_DEVICE (self));
  g_return_if_fail (manette_device_supports_mapping (self));

  g_set_object (&self->mapping, mapping);
}

/**
 * manette_device_get_mapping:
 * @self: a device
 *
 * Gets the user mapping for @self, or default mapping if there isn't any.
 *
 * Can return `NULL` if there's no mapping or @self doesn't support mappings.
 *
 * Returns: (transfer full) (nullable): the mapping for @self
 */
char *
manette_device_get_mapping (ManetteDevice *self)
{
  const char *guid;
  g_autoptr (ManetteMappingManager) mapping_manager = NULL;

  g_return_val_if_fail (MANETTE_IS_DEVICE (self), FALSE);

  if (!manette_device_supports_mapping (self))
    return NULL;

  guid = manette_device_get_guid (self);
  mapping_manager = manette_mapping_manager_new ();

  return manette_mapping_manager_get_mapping (mapping_manager, guid);
}

/**
 * manette_device_has_user_mapping:
 * @self: a device
 *
 * Gets whether @self has a user mapping.
 *
 * Returns: whether @self has a user mapping
 */
gboolean
manette_device_has_user_mapping (ManetteDevice *self)
{
  const char *guid;
  g_autoptr (ManetteMappingManager) mapping_manager = NULL;

  g_return_val_if_fail (MANETTE_IS_DEVICE (self), FALSE);

  if (!manette_device_supports_mapping (self))
    return FALSE;

  guid = manette_device_get_guid (self);
  mapping_manager = manette_mapping_manager_new ();

  return manette_mapping_manager_has_user_mapping (mapping_manager, guid);
}

/**
 * manette_device_save_user_mapping:
 * @self: a device
 * @mapping_string: the mapping string
 *
 * Saves @mapping_string as the user mapping for @self.
 */
void
manette_device_save_user_mapping (ManetteDevice *self,
                                  const char    *mapping_string)
{
  const char *guid;
  const char *name;
  g_autoptr (ManetteMappingManager) mapping_manager = NULL;

  g_return_if_fail (MANETTE_IS_DEVICE (self));
  g_return_if_fail (mapping_string != NULL);
  g_return_if_fail (manette_device_supports_mapping (self));

  guid = manette_device_get_guid (self);
  name = manette_device_get_name (self);
  mapping_manager = manette_mapping_manager_new ();
  manette_mapping_manager_save_mapping (mapping_manager,
                                        guid,
                                        name,
                                        mapping_string);
}

/**
 * manette_device_remove_user_mapping:
 * @self: a device
 *
 * Removes the user mapping for @self.
 */
void
manette_device_remove_user_mapping (ManetteDevice *self)
{
  const char *guid;
  g_autoptr (ManetteMappingManager) mapping_manager = NULL;

  g_return_if_fail (MANETTE_IS_DEVICE (self));
  g_return_if_fail (manette_device_supports_mapping (self));

  guid = manette_device_get_guid (self);
  mapping_manager = manette_mapping_manager_new ();
  manette_mapping_manager_delete_mapping (mapping_manager, guid);
}

/**
 * manette_device_has_rumble:
 * @self: a device
 *
 * Gets whether @self supports rumble.
 *
 * Returns: whether @self supports rumble
 */
gboolean
manette_device_has_rumble (ManetteDevice *self)
{
  g_return_val_if_fail (MANETTE_IS_DEVICE (self), FALSE);

  return manette_backend_has_rumble (self->backend);
}

/**
 * manette_device_rumble:
 * @self: a device
 * @strong_magnitude: the magnitude for the heavy motor
 * @weak_magnitude: the magnitude for the light motor
 * @milliseconds: the rumble effect play time in milliseconds
 *
 * Make @self rumble during @milliseconds milliseconds.
 *
 * The heavy and light motors will rumble at their respectively defined
 * magnitudes.
 *
 * The duration cannot exceed 32767 milliseconds.
 *
 * Returns: whether the rumble effect was played
 */
gboolean
manette_device_rumble (ManetteDevice *self,
                       guint16        strong_magnitude,
                       guint16        weak_magnitude,
                       guint16        milliseconds)
{
  g_return_val_if_fail (MANETTE_IS_DEVICE (self), FALSE);
  g_return_val_if_fail (milliseconds <= G_MAXINT16, FALSE);

  return manette_backend_rumble (self->backend,
                                 strong_magnitude,
                                 weak_magnitude,
                                 milliseconds);
}
