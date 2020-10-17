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

/**
 * SECTION:manette-device
 * @short_description: An object representing a physical gamepad
 * @title: ManetteDevice
 * @See_also: #ManetteMonitor
 */

#include "manette-device-private.h"

#include <errno.h>
#include <fcntl.h>
#include <libevdev/libevdev.h>
#include <linux/input-event-codes.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "manette-event-mapping-private.h"
#include "manette-event-private.h"
#include "manette-mapping-manager-private.h"

struct _ManetteDevice
{
  GObject parent_instance;

  gint fd;
  glong event_source_id;
  struct libevdev *evdev_device;
  guint8 key_map[KEY_MAX];
  guint8 abs_map[ABS_MAX];
  struct input_absinfo abs_info[ABS_MAX];
  gchar *guid;

  ManetteMapping *mapping;

  struct ff_effect rumble_effect;
};

G_DEFINE_TYPE (ManetteDevice, manette_device, G_TYPE_OBJECT)

enum {
  SIG_EVENT,
  SIG_DISCONNECTED,
  SIG_BUTTON_PRESS_EVENT,
  SIG_BUTTON_RELEASE_EVENT,
  SIG_ABSOLUTE_AXIS_EVENT,
  SIG_HAT_AXIS_EVENT,
  N_SIGNALS,
};

static guint signals[N_SIGNALS];

typedef struct {
  ManetteDevice *self;
  guint          signal_id;
  ManetteEvent  *event;
} ManetteDeviceEventSignalData;

/* Private */

static guint
event_type_to_signal (ManetteEventType event_type)
{
  switch (event_type) {
  case MANETTE_EVENT_BUTTON_PRESS:
    return SIG_BUTTON_PRESS_EVENT;
  case MANETTE_EVENT_BUTTON_RELEASE:
    return SIG_BUTTON_RELEASE_EVENT;
  case MANETTE_EVENT_ABSOLUTE:
    return SIG_ABSOLUTE_AXIS_EVENT;
  case MANETTE_EVENT_HAT:
    return SIG_HAT_AXIS_EVENT;
  default:
    return N_SIGNALS;
  }
}

static ManetteDeviceEventSignalData *
manette_device_event_signal_data_new (ManetteDevice *self,
                                      guint          signal_id,
                                      ManetteEvent  *event)
{
  ManetteDeviceEventSignalData *signal_data = g_new (ManetteDeviceEventSignalData, 1);

  signal_data->self = g_object_ref (self);
  signal_data->signal_id = signal_id;
  signal_data->event = manette_event_copy (event);

  return signal_data;
}

static void
manette_device_event_signal_data_free (ManetteDeviceEventSignalData *signal_data)
{
  g_clear_object (&signal_data->self);
  g_clear_pointer (&signal_data->event, manette_event_free);
  g_clear_pointer (&signal_data, g_free);
}

static gboolean
manette_device_event_signal_data_emit (ManetteDeviceEventSignalData *signal_data)
{
  g_signal_emit (signal_data->self, signal_data->signal_id, 0, signal_data->event);

  return FALSE;
}

static void
emit_event_signal_deferred (ManetteDevice *self,
                            guint          signal_id,
                            ManetteEvent  *event)
{
  g_idle_add_full (G_PRIORITY_DEFAULT_IDLE,
                   (GSourceFunc) manette_device_event_signal_data_emit,
                   manette_device_event_signal_data_new (self, signal_id, event),
                   (GDestroyNotify) manette_device_event_signal_data_free);
}

static gboolean
has_key (struct libevdev *device,
         guint            code)
{
  return libevdev_has_event_code (device, (guint) EV_KEY, code);
}

static gboolean
has_abs (struct libevdev *device,
         guint            code)
{
  return libevdev_has_event_code (device, (guint) EV_ABS, code);
}

static gboolean
is_game_controller (struct libevdev *device)
{
  gboolean has_joystick_axes_or_buttons;

  g_assert (device != NULL);

  /* Same detection code as udev-builtin-input_id.c in systemd
   * joysticks don’t necessarily have buttons; e. g.
   * rudders/pedals are joystick-like, but buttonless; they have
   * other fancy axes. */
  has_joystick_axes_or_buttons =
    has_key (device, BTN_TRIGGER) ||
    has_key (device, BTN_A) ||
    has_key (device, BTN_1) ||
    has_abs (device, ABS_RX) ||
    has_abs (device, ABS_RY) ||
    has_abs (device, ABS_RZ) ||
    has_abs (device, ABS_THROTTLE) ||
    has_abs (device, ABS_RUDDER) ||
    has_abs (device, ABS_WHEEL) ||
    has_abs (device, ABS_GAS) ||
    has_abs (device, ABS_BRAKE);

  return has_joystick_axes_or_buttons;
}

static void
forward_event (ManetteDevice *self,
               ManetteEvent  *event)
{
  guint signal = event_type_to_signal (manette_event_get_event_type (event));

  if (signal != N_SIGNALS)
    emit_event_signal_deferred (self, signals[signal], event);
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
remove_event_source (ManetteDevice *self)
{
  g_assert (self != NULL);

  if (self->event_source_id < 0)
    return;

  g_source_remove ((guint) self->event_source_id);
  self->event_source_id = -1;
}

static void
manette_device_finalize (GObject *object)
{
  ManetteDevice *self = (ManetteDevice *)object;

  close (self->fd);
  remove_event_source (self);
  g_clear_pointer (&self->evdev_device, libevdev_free);
  g_clear_pointer (&self->guid, g_free);
  g_clear_object (&self->mapping);

  G_OBJECT_CLASS (manette_device_parent_class)->finalize (object);
}

static void
manette_device_class_init (ManetteDeviceClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = manette_device_finalize;

  /**
   * ManetteDevice::event:
   * @self: a #ManetteDevice
   * @event: the event emitted by the manette device
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
   * @self: a #ManetteDevice
   * @event: the event emitted by the manette device
   *
   * Emitted when a button is pressed.
   */
  signals[SIG_BUTTON_PRESS_EVENT] =
    g_signal_new ("button-press-event",
                  MANETTE_TYPE_DEVICE,
                  G_SIGNAL_RUN_LAST,
                  0, NULL, NULL,
                  g_cclosure_marshal_VOID__BOXED,
                  G_TYPE_NONE, 1,
                  MANETTE_TYPE_EVENT | G_SIGNAL_TYPE_STATIC_SCOPE);

  /**
   * ManetteDevice::button-release-event:
   * @self: a #ManetteDevice
   * @event: the event emitted by the manette device
   *
   * Emitted when a button is released.
   */
  signals[SIG_BUTTON_RELEASE_EVENT] =
    g_signal_new ("button-release-event",
                  MANETTE_TYPE_DEVICE,
                  G_SIGNAL_RUN_LAST,
                  0, NULL, NULL,
                  g_cclosure_marshal_VOID__BOXED,
                  G_TYPE_NONE, 1,
                  MANETTE_TYPE_EVENT | G_SIGNAL_TYPE_STATIC_SCOPE);

  /**
   * ManetteDevice::absolute-axis-event:
   * @self: a #ManetteDevice
   * @event: the event emitted by the manette device
   *
   * Emitted when an absolute axis' value changes.
   */
  signals[SIG_ABSOLUTE_AXIS_EVENT] =
    g_signal_new ("absolute-axis-event",
                  MANETTE_TYPE_DEVICE,
                  G_SIGNAL_RUN_LAST,
                  0, NULL, NULL,
                  g_cclosure_marshal_VOID__BOXED,
                  G_TYPE_NONE, 1,
                  MANETTE_TYPE_EVENT | G_SIGNAL_TYPE_STATIC_SCOPE);

  /**
   * ManetteDevice::hat-axis-event:
   * @self: a #ManetteDevice
   * @event: the event emitted by the manette device
   *
   * Emitted when a hat axis' value changes.
   */
  signals[SIG_HAT_AXIS_EVENT] =
    g_signal_new ("hat-axis-event",
                  MANETTE_TYPE_DEVICE,
                  G_SIGNAL_RUN_LAST,
                  0, NULL, NULL,
                  g_cclosure_marshal_VOID__BOXED,
                  G_TYPE_NONE, 1,
                  MANETTE_TYPE_EVENT | G_SIGNAL_TYPE_STATIC_SCOPE);

  /**
   * ManetteDevice::disconnected:
   * @self: a #ManetteDevice
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
  self->event_source_id = -1;
  self->rumble_effect.type = FF_RUMBLE;
  self->rumble_effect.id = -1;
}

static gchar *
compute_guid_string (struct libevdev *device)
{
  return g_strdup_printf ("%08x%08x%08x%08x",
                          GINT_TO_BE (libevdev_get_id_bustype (device)),
                          GINT_TO_BE (libevdev_get_id_vendor (device)),
                          GINT_TO_BE (libevdev_get_id_product (device)),
                          GINT_TO_BE (libevdev_get_id_version (device)));
}

static gdouble
centered_absolute_value (struct input_absinfo *abs_info,
                         gint32                value)
{
  gint64 max_normalized;
  gint64 value_normalized;
  gint64 max_centered;
  gint64 value_centered;
  gint64 divisor;

  g_assert (abs_info != NULL);

  /* Adapt the value and the maximum to a minimum of 0. */
  max_normalized = ((gint64) abs_info->maximum) - abs_info->minimum;
  value_normalized = ((gint64) value) - abs_info->minimum;

  max_centered = max_normalized / 2;
  value_centered = (value_normalized - max_normalized) + max_centered;

  if (value_centered > -abs_info->flat && value_centered < abs_info->flat)
    value_centered = 0;

  divisor = value_centered < 0 ? max_centered + 1 : max_centered;;

  return ((gdouble) value_centered) / ((gdouble) divisor);
}

static void
on_evdev_event (ManetteDevice      *self,
                struct input_event *evdev_event)
{
  ManetteEvent manette_event;

  manette_event.any.device = self;
  manette_event.any.time = evdev_event->input_event_sec * 1000 +
                           evdev_event->input_event_usec / 1000;
  manette_event.any.hardware_type = evdev_event->type;
  manette_event.any.hardware_code = evdev_event->code;
  manette_event.any.hardware_value = evdev_event->value;

  switch (evdev_event->type) {
  case EV_KEY:
    manette_event.any.type = evdev_event->value ?
      MANETTE_EVENT_BUTTON_PRESS :
      MANETTE_EVENT_BUTTON_RELEASE;
    manette_event.button.hardware_index =
      self->key_map[evdev_event->code - BTN_MISC];
    manette_event.button.button = evdev_event->code;

    break;
  case EV_ABS:
    switch (evdev_event->code) {
    case ABS_HAT0X:
    case ABS_HAT0Y:
    case ABS_HAT1X:
    case ABS_HAT1Y:
    case ABS_HAT2X:
    case ABS_HAT2Y:
    case ABS_HAT3X:
    case ABS_HAT3Y:
      manette_event.any.type = MANETTE_EVENT_HAT;
      manette_event.hat.hardware_index =
        self->key_map[(evdev_event->code - ABS_HAT0X) / 2] * 2 +
        (evdev_event->code - ABS_HAT0X) % 2;
      manette_event.hat.axis = evdev_event->code;
      manette_event.hat.value = evdev_event->value;

      break;
    default:
      manette_event.any.type = MANETTE_EVENT_ABSOLUTE;
      manette_event.absolute.hardware_index = evdev_event->code;
      manette_event.absolute.axis = evdev_event->code;
      manette_event.absolute.value =
        centered_absolute_value (&self->abs_info[self->abs_map[evdev_event->code]],
                                 evdev_event->value);

      break;
    }

    break;
  default:
    manette_event.any.type = MANETTE_EVENT_NOTHING;
  }

  // Send the unmapped event first.
  emit_event_signal_deferred (self, signals[SIG_EVENT], &manette_event);

  // Then map or forward the event using dedicated signals.
  if (self->mapping == NULL)
    forward_event (self, &manette_event);
  else
    map_event (self, &manette_event);
}

static gboolean
poll_events (GIOChannel    *source,
             GIOCondition   condition,
             ManetteDevice *self)
{
  struct input_event evdev_event;

  g_assert (MANETTE_IS_DEVICE (self));

  while (libevdev_has_event_pending (self->evdev_device))
    if (libevdev_next_event (self->evdev_device,
                             (guint) LIBEVDEV_READ_FLAG_NORMAL,
                             &evdev_event) == 0)
      on_evdev_event (self, &evdev_event);

  return TRUE;
}

/**
 * manette_device_new: (skip)
 * @filename: the filename of the device
 * @error: return location for a #GError, or %NULL
 *
 * Creates a new #ManetteDevice.
 *
 * Returns: (transfer full): a new #ManetteDevice
 */
ManetteDevice *
manette_device_new (const gchar  *filename,
                    GError      **error)
{
  g_autoptr (ManetteDevice) self = NULL;
  g_autoptr (GIOChannel) channel = NULL;
  gint buttons_number;
  gint axes_number;
  guint i;

  g_return_val_if_fail (filename != NULL, NULL);

  self = g_object_new (MANETTE_TYPE_DEVICE, NULL);

  self->fd = open (filename, O_RDWR | O_NONBLOCK, (mode_t) 0);
  if (self->fd < 0) {
    g_set_error (error,
                 G_FILE_ERROR,
                 g_file_error_from_errno (errno),
                 "Unable to open “%s”: %s",
                 filename,
                 strerror (errno));

    return NULL;
  }

  self->evdev_device = libevdev_new ();
  if (libevdev_set_fd (self->evdev_device, self->fd) < 0) {
    g_set_error (error,
                 G_FILE_ERROR,
                 g_file_error_from_errno (errno),
                 "Evdev is unable to open “%s”: %s",
                 filename,
                 strerror (errno));

    return NULL;
  }

  if (!is_game_controller (self->evdev_device)) {
    g_set_error (error,
                 G_FILE_ERROR,
                 G_FILE_ERROR_FAILED,
                 "“%s” is not a game controller.",
                 filename);

    return NULL;
  }

  self->event_source_id = -1;

  // Poll the events in the main loop.
  channel = g_io_channel_unix_new (self->fd);
  self->event_source_id = (glong) g_io_add_watch (channel, G_IO_IN, (GIOFunc) poll_events, self);
  buttons_number = 0;

  // Initialize the axes buttons and hats.
  for (i = BTN_JOYSTICK; i < KEY_MAX; i++)
    if (has_key (self->evdev_device, i)) {
      self->key_map[i - BTN_MISC] = (guint8) buttons_number;
      buttons_number++;
    }
  for (i = BTN_MISC; i < BTN_JOYSTICK; i++)
    if (has_key (self->evdev_device, i)) {
      self->key_map[i - BTN_MISC] = (guint8) buttons_number;
      buttons_number++;
    }

  // Get info about the axes.
  axes_number = 0;
  for (i = 0; i < ABS_MAX; i++) {
    // Skip hats
    if (i == ABS_HAT0X) {
      i = ABS_HAT3Y;

      continue;
    }
    if (has_abs (self->evdev_device, i)) {
      const struct input_absinfo *absinfo;

      absinfo = libevdev_get_abs_info (self->evdev_device, i);
      if (absinfo != NULL) {
        self->abs_map[i] = (guint8) axes_number;
        self->abs_info[axes_number] = *absinfo;
        axes_number++;
      }
    }
  }

  return g_steal_pointer (&self);
}

/**
 * manette_device_get_guid:
 * @self: a #ManetteDevice
 *
 * Gets the identifier used by SDL mappings to discriminate game controller
 * devices.
 *
 * Returns: (transfer none): the identifier used by SDL mappings
 */
const gchar *
manette_device_get_guid (ManetteDevice *self)
{
  g_return_val_if_fail (MANETTE_IS_DEVICE (self), NULL);

  if (self->guid == NULL)
    self->guid = compute_guid_string (self->evdev_device);

  return self->guid;
}

/**
 * manette_device_has_input:
 * @self: a #ManetteDevice
 * @type: the input type
 * @code: the input code
 *
 * Gets whether the device has the given input. If the input is present it means
 * that the device can send events for it regardless of whether the device is
 * mapped or not.
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
    libevdev_has_event_code (self->evdev_device, type, code);
}

/**
 * manette_device_get_name:
 * @self: a #ManetteDevice
 *
 * Gets the device's name.
 *
 * Returns: (transfer none): the name of @self, do not modify it or free it
 */
const gchar *
manette_device_get_name (ManetteDevice *self)
{
  g_return_val_if_fail (MANETTE_IS_DEVICE (self), NULL);

  return libevdev_get_name (self->evdev_device);
}

/**
 * manette_device_get_product_id:
 * @self: a #ManetteDevice
 *
 * Gets the device's product ID. You can find some product IDs defined in the
 * kernel's code, in drivers/hid/hid-ids.h. Just note that there isn't all
 * devices there, but sufficiently enough most of the time.
 *
 * Returns: the product ID of @self
 */
int
manette_device_get_product_id (ManetteDevice *self)
{
  g_return_val_if_fail (MANETTE_IS_DEVICE (self), 0);

  return libevdev_get_id_product (self->evdev_device);
}

/**
 * manette_device_get_vendor_id:
 * @self: a #ManetteDevice
 *
 * Gets the device's vendor ID. You can find some vendor IDs defined in the
 * kernel's code, in drivers/hid/hid-ids.h. Just note that there isn't all
 * devices there, but sufficiently enough most of the time.
 *
 * Returns: the vendor ID of @self
 */
int
manette_device_get_vendor_id (ManetteDevice *self)
{
  g_return_val_if_fail (MANETTE_IS_DEVICE (self), 0);

  return libevdev_get_id_vendor (self->evdev_device);
}

/**
 * manette_device_get_bustype_id:
 * @self: a #ManetteDevice
 *
 * Gets the device's bustype ID.
 * This corresponds to BUS_* as defined in linux/input.h
 *
 * Returns: the bustype ID of @self
 */
int
manette_device_get_bustype_id (ManetteDevice *self)
{
  g_return_val_if_fail (MANETTE_IS_DEVICE (self), 0);

  return libevdev_get_id_bustype (self->evdev_device);
}

/**
 * manette_device_get_version_id:
 * @self: a #ManetteDevice
 *
 * Gets the device's version ID.
 *
 * Returns: the version ID of @self
 */
int
manette_device_get_version_id (ManetteDevice *self)
{
  g_return_val_if_fail (MANETTE_IS_DEVICE (self), 0);

  return libevdev_get_id_version (self->evdev_device);
}

/**
 * manette_device_set_mapping:
 * @self: a #ManetteDevice
 * @mapping: a #ManetteMapping
 *
 * Associate @mapping to @self to map its hardware events into standard gamepad
 * ones.
 */
void
manette_device_set_mapping (ManetteDevice  *self,
                            ManetteMapping *mapping)
{
  g_return_if_fail (MANETTE_IS_DEVICE (self));

  g_set_object (&self->mapping, mapping);
}

/**
 * manette_device_has_user_mapping:
 * @self: a #ManetteDevice
 *
 * Gets whether @self has a user mapping.
 *
 * Returns: whether @self has a user mapping
 */
gboolean
manette_device_has_user_mapping (ManetteDevice *self)
{
  const gchar *guid;
  g_autoptr (ManetteMappingManager) mapping_manager = NULL;

  g_return_val_if_fail (MANETTE_IS_DEVICE (self), FALSE);

  guid = manette_device_get_guid (self);
  mapping_manager = manette_mapping_manager_new ();

  return manette_mapping_manager_has_user_mapping (mapping_manager, guid);
}

/**
 * manette_device_save_user_mapping:
 * @self: a #ManetteDevice
 * @mapping_string: the mapping string
 *
 * Saves @mapping_string as the user mapping for @self.
 */
void
manette_device_save_user_mapping (ManetteDevice *self,
                                  const gchar   *mapping_string)
{
  const gchar *guid;
  const gchar *name;
  g_autoptr (ManetteMappingManager) mapping_manager = NULL;

  g_return_if_fail (MANETTE_IS_DEVICE (self));
  g_return_if_fail (mapping_string != NULL);

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
 * @self: a #ManetteDevice
 *
 * Removes the user mapping for @self.
 */
void
manette_device_remove_user_mapping (ManetteDevice *self)
{
  const gchar *guid;
  g_autoptr (ManetteMappingManager) mapping_manager = NULL;

  g_return_if_fail (MANETTE_IS_DEVICE (self));

  guid = manette_device_get_guid (self);
  mapping_manager = manette_mapping_manager_new ();
  manette_mapping_manager_delete_mapping (mapping_manager, guid);
}

/**
 * manette_device_has_rumble:
 * @self: a #ManetteDevice
 *
 * Gets whether @self supports rumble.
 *
 * Returns: whether @self supports rumble
 */
gboolean
manette_device_has_rumble (ManetteDevice *self)
{
  gulong features[4];

  g_return_val_if_fail (MANETTE_IS_DEVICE (self), FALSE);

  if (ioctl (self->fd, EVIOCGBIT (EV_FF, sizeof (gulong) * 4), features) == -1)
    return FALSE;

  if (!((features[FF_RUMBLE / (sizeof (glong) * 8)] >> FF_RUMBLE % (sizeof (glong) * 8)) & 1))
    return FALSE;

  return TRUE;
}

/**
 * manette_device_rumble:
 * @self: a #ManetteDevice
 * @strong_magnitude: the magnitude for the heavy motor
 * @weak_magnitude: the magnitude for the light motor
 * @milliseconds: the rumble effect play time in milliseconds
 *
 * Make @self rumble during @milliseconds milliseconds, with the heavy and light
 * motors rumbling at their respectively defined magnitudes.
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
  struct input_event event;

  g_return_val_if_fail (MANETTE_IS_DEVICE (self), FALSE);
  g_return_val_if_fail (milliseconds <= G_MAXINT16, FALSE);

  self->rumble_effect.u.rumble.strong_magnitude = strong_magnitude;
  self->rumble_effect.u.rumble.weak_magnitude = weak_magnitude;
  self->rumble_effect.replay.length = milliseconds;

  if (ioctl (self->fd, EVIOCSFF, &self->rumble_effect) == -1) {
    g_debug ("Failed to upload the rumble effect.");

    return FALSE;
  }

  event.type = EV_FF;
  event.code = self->rumble_effect.id;
  /* 1 to play the event, 0 to stop it. */
  event.value = 1;

  if (write (self->fd, (const void*) &event, sizeof (event)) == -1) {
    g_debug ("Failed to start the rumble effect.");

    return FALSE;
  }

  return TRUE;
}
