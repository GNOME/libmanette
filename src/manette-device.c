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
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "manette-device-private.h"

#include <errno.h>
#include <fcntl.h>
#include <libevdev/libevdev.h>
#include <linux/input-event-codes.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "manette-event-private.h"

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

#define GUID_DATA_LENGTH 8
#define GUID_STRING_LENGTH 32 // (GUID_DATA_LENGTH * sizeof (guint16))

/* Private */

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

  g_return_val_if_fail (device != NULL, FALSE);

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
  switch (manette_event_get_event_type (event)) {
  case MANETTE_EVENT_ABSOLUTE:
    g_signal_emit (self, signals[SIG_ABSOLUTE_AXIS_EVENT], 0, event);

    return;
  case MANETTE_EVENT_BUTTON_PRESS:
    g_signal_emit (self, signals[SIG_BUTTON_PRESS_EVENT], 0, event);

    return;
  case MANETTE_EVENT_BUTTON_RELEASE:
    g_signal_emit (self, signals[SIG_BUTTON_RELEASE_EVENT], 0, event);

    return;
  case MANETTE_EVENT_HAT:
    g_signal_emit (self, signals[SIG_HAT_AXIS_EVENT], 0, event);

    return;
  default:
    return;
  }
}

static void
map_absolute_event (ManetteDevice        *self,
                    ManetteEventAbsolute *event)
{
  const ManetteMappingBinding * const *bindings;
  const ManetteMappingBinding * binding;
  ManetteEvent *mapped_event;
  guint signal;
  gdouble absolute_value;
  gboolean pressed;

  bindings = manette_mapping_get_bindings (self->mapping,
                                           MANETTE_MAPPING_INPUT_TYPE_AXIS,
                                           event->hardware_index);
  if (bindings == NULL)
    return;

  for (; *bindings != NULL; bindings++) {
    binding = *bindings;

    if (binding->source.range == MANETTE_MAPPING_RANGE_NEGATIVE &&
        event->value > 0.)
      continue;

    if (binding->source.range == MANETTE_MAPPING_RANGE_POSITIVE &&
        event->value < 0.)
      continue;


    mapped_event = manette_event_copy ((ManetteEvent *) event);

    switch (binding->destination.type) {
    case EV_ABS:
      absolute_value = binding->source.invert ? -event->value : event->value;

      signal = SIG_ABSOLUTE_AXIS_EVENT;
      mapped_event->any.type = MANETTE_EVENT_ABSOLUTE;
      mapped_event->absolute.axis = binding->destination.code;
      switch (binding->destination.range) {
      case MANETTE_MAPPING_RANGE_FULL:
        mapped_event->absolute.value = absolute_value;

        break;
      case MANETTE_MAPPING_RANGE_NEGATIVE:
        mapped_event->absolute.value = (absolute_value / 2) - 1;

        break;
      case MANETTE_MAPPING_RANGE_POSITIVE:
        mapped_event->absolute.value = (absolute_value / 2) + 1;

        break;
      default:
        break;
      }

      break;
    case EV_KEY:
      pressed = binding->source.invert ? event->value < -0. : event->value > 0.;

      signal = pressed ? SIG_BUTTON_PRESS_EVENT : SIG_BUTTON_RELEASE_EVENT;
      mapped_event->any.type = pressed ? MANETTE_EVENT_BUTTON_PRESS :
                                         MANETTE_EVENT_BUTTON_RELEASE;
      mapped_event->button.button = binding->destination.code;

      break;
    default:
      manette_event_free (mapped_event);

      return;
    }

    g_signal_emit (self, signals[signal], 0, mapped_event);

    manette_event_free (mapped_event);
  }
}

static void
map_button_event (ManetteDevice      *self,
                  ManetteEventButton *event)
{
  const ManetteMappingBinding * const *bindings;
  const ManetteMappingBinding * binding;
  ManetteEvent *mapped_event;
  guint signal;
  gboolean pressed;

  bindings = manette_mapping_get_bindings (self->mapping,
                                           MANETTE_MAPPING_INPUT_TYPE_BUTTON,
                                           event->hardware_index);
  if (bindings == NULL)
    return;

  for (; *bindings != NULL; bindings++) {
    binding = *bindings;

    mapped_event = manette_event_copy ((ManetteEvent *) event);

    pressed = event->type == MANETTE_EVENT_BUTTON_PRESS;

    switch (binding->destination.type) {
    case EV_ABS:
      signal = SIG_ABSOLUTE_AXIS_EVENT;
      mapped_event->any.type = MANETTE_EVENT_ABSOLUTE;
      mapped_event->absolute.axis = binding->destination.code;
      switch (binding->destination.range) {
      case MANETTE_MAPPING_RANGE_NEGATIVE:
        mapped_event->absolute.value = pressed ? -1 : 0;

        break;
      case MANETTE_MAPPING_RANGE_FULL:
      case MANETTE_MAPPING_RANGE_POSITIVE:
        mapped_event->absolute.value = pressed ? 1 : 0;

        break;
      default:
        mapped_event->absolute.value = 0;

        break;
      }

      break;
    case EV_KEY:
      signal = pressed ? SIG_BUTTON_PRESS_EVENT : SIG_BUTTON_RELEASE_EVENT;
      mapped_event->any.type = pressed ? MANETTE_EVENT_BUTTON_PRESS :
                                         MANETTE_EVENT_BUTTON_RELEASE;
      mapped_event->button.button = binding->destination.code;

      break;
    default:
      manette_event_free (mapped_event);

      return;
    }

    g_signal_emit (self, signals[signal], 0, mapped_event);

    manette_event_free (mapped_event);
  }
}

static void
map_hat_event (ManetteDevice   *self,
               ManetteEventHat *event)
{
  const ManetteMappingBinding * const *bindings;
  const ManetteMappingBinding * binding;
  ManetteEvent *mapped_event;
  guint signal;
  gboolean pressed;

  bindings = manette_mapping_get_bindings (self->mapping,
                                           MANETTE_MAPPING_INPUT_TYPE_HAT,
                                           event->hardware_index);
  if (bindings == NULL)
    return;

  for (; *bindings != NULL; bindings++) {
    binding = *bindings;

    if (binding->source.range == MANETTE_MAPPING_RANGE_NEGATIVE &&
        event->value > 0)
      continue;

    if (binding->source.range == MANETTE_MAPPING_RANGE_POSITIVE &&
        event->value < 0)
      continue;

    mapped_event = manette_event_copy ((ManetteEvent *) event);

    pressed = abs (event->value);

    switch (binding->destination.type) {
    case EV_ABS:
      signal = SIG_ABSOLUTE_AXIS_EVENT;
      mapped_event->any.type = MANETTE_EVENT_ABSOLUTE;
      mapped_event->absolute.axis = binding->destination.code;
      mapped_event->absolute.value = abs (event->value);

      break;
    case EV_KEY:
      signal = pressed ? SIG_BUTTON_PRESS_EVENT : SIG_BUTTON_RELEASE_EVENT;
      mapped_event->any.type = pressed ? MANETTE_EVENT_BUTTON_PRESS :
                                         MANETTE_EVENT_BUTTON_RELEASE;
      mapped_event->button.button = binding->destination.code;

      break;
    default:
      manette_event_free (mapped_event);

      return;
    }

    g_signal_emit (self, signals[signal], 0, mapped_event);

    manette_event_free (mapped_event);
  }
}

static void
map_event (ManetteDevice *self,
           ManetteEvent    *event)
{
  switch (manette_event_get_event_type (event)) {
  case MANETTE_EVENT_BUTTON_PRESS:
  case MANETTE_EVENT_BUTTON_RELEASE:
    map_button_event (self, &event->button);

    break;
  case MANETTE_EVENT_ABSOLUTE:
    map_absolute_event (self, &event->absolute);

    break;
  case MANETTE_EVENT_HAT:
    map_hat_event (self, &event->hat);

    break;
  default:
    break;
  }
}

static void
remove_event_source (ManetteDevice *self)
{
  g_return_if_fail (self != NULL);

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
  if (self->evdev_device != NULL)
    libevdev_free (self->evdev_device);
  g_free (self->guid);
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
}

static gchar
guint16_get_hex (guint16 value,
                 guint8  nibble)
{
  static const gchar hex_to_ascii_map[] = "0123456789abcdef";

  g_assert (nibble < 4);

  return hex_to_ascii_map[((value >> (4 * nibble)) & 0xf)];
}

static gchar *
guint16s_to_hex_string (guint16 *data)
{
  gchar *result;
  gint data_i;
  gint result_i;
  guint16 element;

  result = g_malloc (GUID_STRING_LENGTH + 1);
  result[GUID_STRING_LENGTH] = '\0';
  for (data_i = 0, result_i = 0; data_i < GUID_DATA_LENGTH; data_i++) {
    element = data[data_i];
    result[result_i++] = guint16_get_hex (element, 1);
    result[result_i++] = guint16_get_hex (element, 0);
    result[result_i++] = guint16_get_hex (element, 3);
    result[result_i++] = guint16_get_hex (element, 2);
  }

  return result;
}

// FIXME What about using 4 well crafted %x?
static gchar *
compute_guid_string (struct libevdev *device)
{
  guint16 guid_array[GUID_DATA_LENGTH] = { 0 };

  guid_array[0] = (guint16) GINT_TO_LE (libevdev_get_id_bustype (device));
  guid_array[1] = 0;
  guid_array[2] = (guint16) GINT_TO_LE (libevdev_get_id_vendor (device));
  guid_array[3] = 0;
  guid_array[4] = (guint16) GINT_TO_LE (libevdev_get_id_product (device));
  guid_array[5] = 0;
  guid_array[6] = (guint16) GINT_TO_LE (libevdev_get_id_version (device));
  guid_array[7] = 0;

  return guint16s_to_hex_string (guid_array);
}

static gdouble
centered_absolute_value (struct input_absinfo *abs_info,
                         gint32                value)
{
  gint64 min_absolute;
  gint64 max_normalized;
  gint64 value_normalized;
  gint64 max_centered;
  gint64 value_centered;
  gint64 divisor;

  g_return_val_if_fail (abs_info != NULL, 0.0);

  min_absolute = llabs ((gint64) abs_info->minimum);

  max_normalized = ((gint64) abs_info->maximum) + min_absolute;
  value_normalized = ((gint64) value) + min_absolute;

  max_centered = max_normalized / 2;
  value_centered = (value_normalized - max_normalized) + max_centered;

  divisor = value_centered < 0 ? max_centered + 1 : max_centered;;

  return ((gdouble) value_centered) / ((gdouble) divisor);
}

static void
on_evdev_event (ManetteDevice      *self,
                struct input_event *evdev_event)
{
  ManetteEvent manette_event;

  manette_event.any.device = self;
  manette_event.any.time = evdev_event->time.tv_sec * 1000 +
                           evdev_event->time.tv_usec / 1000;
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
  g_signal_emit (self, signals[SIG_EVENT], 0, &manette_event);

  // Then map or forward the event using dedicated signals.
  if (self->mapping == NULL)
    forward_event (self, &manette_event);
  else
    map_event (self, &manette_event);
}

static gboolean
poll_events (GIOChannel   *source,
             GIOCondition  condition,
             gpointer      data)
{
  ManetteDevice *self;
  struct input_event evdev_event;

  self = MANETTE_DEVICE (data);

  g_return_val_if_fail (MANETTE_IS_DEVICE (self), FALSE);

  while (libevdev_has_event_pending (self->evdev_device))
    if (libevdev_next_event (self->evdev_device,
                             (guint) LIBEVDEV_READ_FLAG_NORMAL,
                             &evdev_event) == 0)
      on_evdev_event (self, &evdev_event);

  return TRUE;
}

/**
 * manette_device_new: (skip):
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
  ManetteDevice *self = NULL;
  GIOChannel *channel;
  gint buttons_number;
  gint axes_number;
  guint i;

  g_return_val_if_fail (filename != NULL, NULL);

  self = g_object_new (MANETTE_TYPE_DEVICE, NULL);

  self->fd = open (filename, O_RDONLY | O_NONBLOCK, (mode_t) 0);
  if (self->fd < 0) {
    g_set_error (error,
                 G_FILE_ERROR,
                 G_FILE_ERROR_FAILED,
                 "Unable to open “%s”: %s",
                 filename,
                 strerror (errno));
    g_object_unref (self);

    return NULL;
  }

  self->evdev_device = libevdev_new ();
  if (libevdev_set_fd (self->evdev_device, self->fd) < 0) {
    g_set_error (error,
                 G_FILE_ERROR,
                 G_FILE_ERROR_FAILED,
                 "Evdev is unable to open “%s”: %s",
                 filename,
                 strerror (errno));
    g_object_unref (self);

    return NULL;
  }

  if (!is_game_controller (self->evdev_device)) {
    g_set_error (error,
                 G_FILE_ERROR,
                 G_FILE_ERROR_NXIO,
                 "“%s” is not a game controller.",
                 filename);
    g_object_unref (self);

    return NULL;
  }

  self->event_source_id = -1;

  // Poll the events in the main loop.
  channel = g_io_channel_unix_new (self->fd);
  self->event_source_id = (glong) g_io_add_watch (channel, G_IO_IN, poll_events, self);
  buttons_number = 0;

  // Initialize the axes buttons and hats.
  for (i = BTN_JOYSTICK; i < KEY_MAX; i++)
    if (libevdev_has_event_code (self->evdev_device, (guint) EV_KEY, i)) {
      self->key_map[i - BTN_MISC] = (guint8) buttons_number;
      buttons_number++;
    }
  for (i = BTN_MISC; i < BTN_JOYSTICK; i++)
    if (libevdev_has_event_code (self->evdev_device, (guint) EV_KEY, i)) {
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
    if (libevdev_has_event_code (self->evdev_device, (guint) EV_ABS, i)) {
      const struct input_absinfo *absinfo;

      absinfo = libevdev_get_abs_info (self->evdev_device, i);
      if (absinfo != NULL) {
        self->abs_map[i] = (guint8) axes_number;
        self->abs_info[axes_number] = *absinfo;
        axes_number++;
      }
    }
  }

  g_io_channel_unref (channel);

  return self;
}

// FIXME move to ManetteMappingManager?
const gchar *
manette_device_get_guid (ManetteDevice *self)
{
  g_return_val_if_fail (MANETTE_IS_DEVICE (self), NULL);

  if (self->guid == NULL)
    self->guid = compute_guid_string (self->evdev_device);

  return self->guid;
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
 * Gets the device's product ID.
 *
 * Returns: the product ID of @self
 */
int
manette_device_get_product_id (ManetteDevice *self)
{
  return libevdev_get_id_product (self->evdev_device);
}

/**
 * manette_device_get_vendor_id:
 * @self: a #ManetteDevice
 *
 * Gets the device's vendor ID.
 *
 * Returns: the vendor ID of @self
 */
int
manette_device_get_vendor_id (ManetteDevice *self)
{
  return libevdev_get_id_vendor (self->evdev_device);
}

/**
 * manette_device_get_bustype_id:
 * @self: a #ManetteDevice
 *
 * Gets the device's bustype ID.
 *
 * Returns: the bustype ID of @self
 */
int
manette_device_get_bustype_id (ManetteDevice *self)
{
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
  return libevdev_get_id_version (self->evdev_device);
}

// FIXME documentation
void
manette_device_set_mapping (ManetteDevice  *self,
                            ManetteMapping *mapping)
{
  if (self->mapping != NULL)
    g_object_unref (self->mapping);

  self->mapping = mapping ? g_object_ref (mapping) : NULL;
}