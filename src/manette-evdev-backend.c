/* manette-evdev-backend.c
 *
 * Copyright (C) 2017 Adrien Plazas <kekun.plazas@laposte.net>
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

#include "manette-evdev-backend-private.h"

#include <fcntl.h>
#include <libevdev/libevdev.h>
#include <linux/input.h>
#include <linux/input-event-codes.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include "manette-device-type-private.h"
#include "manette-event-mapping-private.h"

#define VENDOR_SONY       0x054C
#define PRODUCT_DUALSENSE 0x0CE6

struct _ManetteEvdevBackend
{
  GObject parent_instance;

  char *filename;

  int fd;
  guint event_source_id;
  struct libevdev *evdev_device;

  guint8 key_map[KEY_MAX];
  guint8 abs_map[ABS_MAX];
  struct input_absinfo abs_info[ABS_MAX];

  struct ff_effect rumble_effect;

  ManetteMapping *mapping;
};

static void manette_evdev_backend_backend_init (ManetteBackendInterface *iface);

G_DEFINE_FINAL_TYPE_WITH_CODE (ManetteEvdevBackend, manette_evdev_backend, G_TYPE_OBJECT,
                               G_IMPLEMENT_INTERFACE (MANETTE_TYPE_BACKEND, manette_evdev_backend_backend_init))

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
is_game_controller (struct libevdev *device,
                    int              vendor,
                    int              product)
{
  gboolean has_buttons;
  gboolean has_axes;

  g_assert (device != NULL);

  /* Same detection code as udev-builtin-input_id.c in systemd
   * joysticks don’t necessarily have buttons; e. g.
   * rudders/pedals are joystick-like, but buttonless; they have
   * other fancy axes. */
  has_buttons =
    has_key (device, BTN_TRIGGER) ||
    has_key (device, BTN_A) ||
    has_key (device, BTN_1);

  has_axes =
    has_abs (device, ABS_RX) ||
    has_abs (device, ABS_RY) ||
    has_abs (device, ABS_RZ) ||
    has_abs (device, ABS_THROTTLE) ||
    has_abs (device, ABS_RUDDER) ||
    has_abs (device, ABS_WHEEL) ||
    has_abs (device, ABS_GAS) ||
    has_abs (device, ABS_BRAKE);

  /* Filter out DualSense motion sensor and touchpad */
  if (vendor == VENDOR_SONY && product == PRODUCT_DUALSENSE)
    return has_buttons && has_axes;

  return has_buttons || has_axes;
}

static double
centered_absolute_value (struct input_absinfo *abs_info,
                         gint32                value)
{
  gint64 max_normalized;
  gint64 value_normalized;
  gint64 max_centered;
  gint64 value_centered;
  gint64 divisor;

  g_assert (abs_info != NULL);

  value = CLAMP (value, abs_info->minimum, abs_info->maximum);

  if (value > -abs_info->flat && value < abs_info->flat)
    value = 0;

  /* Adapt the value and the maximum to a minimum of 0. */
  max_normalized = ((gint64) abs_info->maximum) - abs_info->minimum;
  value_normalized = ((gint64) value) - abs_info->minimum;

  max_centered = max_normalized / 2;
  value_centered = (value_normalized - max_normalized) + max_centered;

  divisor = value_centered < 0 ? max_centered + 1 : max_centered;

  return (double) value_centered / (double) divisor;
}

static void
emit_mapped_events (ManetteEvdevBackend *self,
                    guint64              time,
                    GSList              *mapped_events)
{
  GSList *l = NULL;

  for (l = mapped_events; l != NULL; l = l->next) {
    ManetteMappedEvent *mapped_event = l->data;

    switch (mapped_event->type) {
    case MANETTE_MAPPING_DESTINATION_TYPE_AXIS:
      manette_backend_emit_axis_event (MANETTE_BACKEND (self), time,
                                       mapped_event->axis.axis,
                                       mapped_event->axis.value);
      break;

    case MANETTE_MAPPING_DESTINATION_TYPE_BUTTON:
      manette_backend_emit_button_event (MANETTE_BACKEND (self), time,
                                         mapped_event->button.button,
                                         mapped_event->button.pressed);
      break;

    default:
      g_assert_not_reached ();
    }
  }

  g_slist_free_full (mapped_events, (GDestroyNotify) g_free);
}

static ManetteButton
evdev_code_to_button (guint code)
{
  switch (code) {
  case BTN_DPAD_UP:
    return MANETTE_BUTTON_DPAD_UP;
  case BTN_DPAD_DOWN:
    return MANETTE_BUTTON_DPAD_DOWN;
  case BTN_DPAD_LEFT:
    return MANETTE_BUTTON_DPAD_LEFT;
  case BTN_DPAD_RIGHT:
    return MANETTE_BUTTON_DPAD_RIGHT;
  case BTN_NORTH:
    return MANETTE_BUTTON_NORTH;
  case BTN_SOUTH:
    return MANETTE_BUTTON_SOUTH;
  case BTN_WEST:
    return MANETTE_BUTTON_WEST;
  case BTN_EAST:
    return MANETTE_BUTTON_EAST;
  case BTN_SELECT:
    return MANETTE_BUTTON_SELECT;
  case BTN_START:
    return MANETTE_BUTTON_START;
  case BTN_MODE:
    return MANETTE_BUTTON_MODE;
  case BTN_TL:
    return MANETTE_BUTTON_LEFT_SHOULDER;
  case BTN_TR:
    return MANETTE_BUTTON_RIGHT_SHOULDER;
  case BTN_THUMBL:
    return MANETTE_BUTTON_LEFT_STICK;
  case BTN_THUMBR:
    return MANETTE_BUTTON_RIGHT_STICK;
  default:
    return -1;
  }
}

static guint
manette_button_to_evdev_code (ManetteButton button)
{
  switch (button) {
  case MANETTE_BUTTON_DPAD_UP:
    return BTN_DPAD_UP;
  case MANETTE_BUTTON_DPAD_DOWN:
    return BTN_DPAD_DOWN;
  case MANETTE_BUTTON_DPAD_LEFT:
    return BTN_DPAD_LEFT;
  case MANETTE_BUTTON_DPAD_RIGHT:
    return BTN_DPAD_RIGHT;
  case MANETTE_BUTTON_NORTH:
    return BTN_NORTH;
  case MANETTE_BUTTON_SOUTH:
    return BTN_SOUTH;
  case MANETTE_BUTTON_WEST:
    return BTN_WEST;
  case MANETTE_BUTTON_EAST:
    return BTN_EAST;
  case MANETTE_BUTTON_SELECT:
    return BTN_SELECT;
  case MANETTE_BUTTON_START:
    return BTN_START;
  case MANETTE_BUTTON_MODE:
    return BTN_MODE;
  case MANETTE_BUTTON_LEFT_SHOULDER:
    return BTN_TL;
  case MANETTE_BUTTON_RIGHT_SHOULDER:
    return BTN_TR;
  case MANETTE_BUTTON_LEFT_STICK:
    return BTN_THUMBL;
  case MANETTE_BUTTON_RIGHT_STICK:
    return BTN_THUMBR;
  default:
    return 0;
  }
}

static ManetteAxis
evdev_code_to_axis (guint code)
{
  switch (code) {
  case ABS_X:
    return MANETTE_AXIS_LEFT_X;
  case ABS_Y:
    return MANETTE_AXIS_LEFT_Y;
  case ABS_RX:
    return MANETTE_AXIS_RIGHT_X;
  case ABS_RY:
    return MANETTE_AXIS_RIGHT_Y;
  case ABS_Z:
    return MANETTE_AXIS_LEFT_TRIGGER;
  case ABS_RZ:
    return MANETTE_AXIS_RIGHT_TRIGGER;
  default:
    return 0;
  }
}

static guint
manette_axis_to_evdev_code (ManetteAxis axis)
{
  switch (axis) {
  case MANETTE_AXIS_LEFT_X:
    return ABS_X;
  case MANETTE_AXIS_LEFT_Y:
    return ABS_Y;
  case MANETTE_AXIS_RIGHT_X:
    return ABS_RX;
  case MANETTE_AXIS_RIGHT_Y:
    return ABS_RY;
  case MANETTE_AXIS_LEFT_TRIGGER:
    return ABS_Z;
  case MANETTE_AXIS_RIGHT_TRIGGER:
    return ABS_RZ;
  default:
    return 0;
  }
}

static void
on_evdev_event (ManetteEvdevBackend *self,
                struct input_event  *evdev_event)
{
  guint64 time = evdev_event->input_event_sec * 1000 +
                 evdev_event->input_event_usec / 1000;

  switch (evdev_event->type) {
  case EV_KEY:
    gboolean pressed = !!evdev_event->value;
    guint index = evdev_event->code < BTN_MISC ?
      evdev_event->code + BTN_MISC :
      evdev_event->code - BTN_MISC;

    manette_backend_emit_unmapped_button_event (MANETTE_BACKEND (self), time,
                                                self->key_map[index], pressed);

    if (self->mapping == NULL) {
      ManetteButton button = evdev_code_to_button (evdev_event->code);

      if (button != -1) {
        manette_backend_emit_button_event (MANETTE_BACKEND (self),
                                           time, button, pressed);
      }
    } else {
      GSList *mapped = manette_map_button_event (self->mapping,
                                                 self->key_map[index],
                                                 pressed);

      emit_mapped_events (self, time, mapped);
    }

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
      guint index =
        self->key_map[(evdev_event->code - ABS_HAT0X) / 2] * 2 +
        (evdev_event->code - ABS_HAT0X) % 2;

      manette_backend_emit_unmapped_hat_event (MANETTE_BACKEND (self), time,
                                               index, evdev_event->value);

      // We don't send unmapped hat events
      if (self->mapping != NULL) {
        GSList *mapped = manette_map_hat_event (self->mapping, index,
                                                evdev_event->value);

        emit_mapped_events (self, time, mapped);
      }

      break;
    default:
      double value =
        centered_absolute_value (&self->abs_info[self->abs_map[evdev_event->code]],
                                 evdev_event->value);

      manette_backend_emit_unmapped_absolute_event (MANETTE_BACKEND (self), time,
                                                    evdev_event->code, value);

      if (self->mapping == NULL) {
        ManetteAxis axis = evdev_code_to_axis (evdev_event->code);

        if (axis == -1)
          break;

        if (axis == MANETTE_AXIS_LEFT_TRIGGER || axis == MANETTE_AXIS_RIGHT_TRIGGER) {
          /* Triggers only use the positive range */
          value = (value + 1.0) / 2.0;
        }

        manette_backend_emit_axis_event (MANETTE_BACKEND (self),
                                         time, axis, value);
      } else {
        GSList *mapped = manette_map_absolute_event (self->mapping,
                                                     evdev_event->code, value);

        emit_mapped_events (self, time, mapped);
      }

      break;
    }

    break;
  default:
    return;
  }
}

static gboolean
poll_events (GIOChannel          *source,
             GIOCondition         condition,
             ManetteEvdevBackend *self)
{
  struct input_event evdev_event;

  g_assert (MANETTE_IS_EVDEV_BACKEND (self));

  while (libevdev_has_event_pending (self->evdev_device)) {
    if (libevdev_next_event (self->evdev_device,
                             (guint) LIBEVDEV_READ_FLAG_NORMAL,
                             &evdev_event) == 0) {
      on_evdev_event (self, &evdev_event);
    }
  }

  return TRUE;
}

static void
manette_evdev_backend_finalize (GObject *object)
{
  ManetteEvdevBackend *self = MANETTE_EVDEV_BACKEND (object);

  g_clear_object (&self->mapping);
  g_clear_handle_id (&self->event_source_id, g_source_remove);
  close (self->fd);
  libevdev_free (self->evdev_device);
  g_free (self->filename);

  G_OBJECT_CLASS (manette_evdev_backend_parent_class)->finalize (object);
}

static void
manette_evdev_backend_class_init (ManetteEvdevBackendClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = manette_evdev_backend_finalize;
}

static void
manette_evdev_backend_init (ManetteEvdevBackend *self)
{
  self->rumble_effect.type = FF_RUMBLE;
  self->rumble_effect.id = -1;
}

static gboolean
manette_evdev_backend_initialize (ManetteBackend *backend)
{
  ManetteEvdevBackend *self = MANETTE_EVDEV_BACKEND (backend);
  g_autoptr (GIOChannel) channel = NULL;
  int vendor, product;
  int buttons_number;
  int axes_number;
  guint i;

  self->fd = open (self->filename, O_RDWR | O_NONBLOCK, (mode_t) 0);
  if (self->fd < 0) {
    g_debug ("Failed to open %s: %s", self->filename, strerror (errno));

    return FALSE;
  }

  self->evdev_device = libevdev_new ();
  if (libevdev_set_fd (self->evdev_device, self->fd) < 0)
    return FALSE;

  vendor = libevdev_get_id_vendor (self->evdev_device);
  product = libevdev_get_id_product (self->evdev_device);

  if (!is_game_controller (self->evdev_device, vendor, product))
    return FALSE;

  /* Other types are handled via hid backend or skipped */
  if (manette_device_type_guess (vendor, product) != MANETTE_DEVICE_GENERIC)
    return FALSE;

  // Poll the events in the main loop.
  channel = g_io_channel_unix_new (self->fd);
  self->event_source_id = g_io_add_watch (channel, G_IO_IN, (GIOFunc) poll_events, self);

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
  for (i = 0; i < BTN_MISC; i++)
    if (has_key (self->evdev_device, i)) {
      self->key_map[i + BTN_MISC] = (guint8) buttons_number;
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

  return TRUE;
}

static const char *
manette_evdev_backend_get_name (ManetteBackend *backend)
{
  ManetteEvdevBackend *self = MANETTE_EVDEV_BACKEND (backend);

  return libevdev_get_name (self->evdev_device);
}

static int
manette_evdev_backend_get_vendor_id (ManetteBackend *backend)
{
  ManetteEvdevBackend *self = MANETTE_EVDEV_BACKEND (backend);

  return libevdev_get_id_vendor (self->evdev_device);
}

static int
manette_evdev_backend_get_product_id (ManetteBackend *backend)
{
  ManetteEvdevBackend *self = MANETTE_EVDEV_BACKEND (backend);

  return libevdev_get_id_product (self->evdev_device);
}

static int
manette_evdev_backend_get_bustype_id (ManetteBackend *backend)
{
  ManetteEvdevBackend *self = MANETTE_EVDEV_BACKEND (backend);

  return libevdev_get_id_bustype (self->evdev_device);
}

static int
manette_evdev_backend_get_version_id (ManetteBackend *backend)
{
  ManetteEvdevBackend *self = MANETTE_EVDEV_BACKEND (backend);

  return libevdev_get_id_version (self->evdev_device);
}

void
manette_evdev_backend_set_mapping (ManetteBackend *backend,
                                   ManetteMapping *mapping)
{
  ManetteEvdevBackend *self = MANETTE_EVDEV_BACKEND (backend);

  g_set_object (&self->mapping, mapping);
}

gboolean
manette_evdev_backend_has_button (ManetteBackend *backend,
                                  ManetteButton   button)
{
  ManetteEvdevBackend *self = MANETTE_EVDEV_BACKEND (backend);
  guint code;

  if (self->mapping)
    return manette_mapping_has_destination_button (self->mapping, button);

  code = manette_button_to_evdev_code (button);
  if (code == 0)
    return FALSE;

  return libevdev_has_event_code (self->evdev_device, EV_KEY, code);
}

gboolean
manette_evdev_backend_has_axis (ManetteBackend *backend,
                                ManetteAxis     axis)
{
  ManetteEvdevBackend *self = MANETTE_EVDEV_BACKEND (backend);
  guint code;

  if (self->mapping)
    return manette_mapping_has_destination_axis (self->mapping, axis);

  code = manette_axis_to_evdev_code (axis);
  if (code == 0)
    return FALSE;

  return libevdev_has_event_code (self->evdev_device, EV_ABS, code);
}

gboolean
manette_evdev_backend_has_input (ManetteBackend *backend,
                                 guint           type,
                                 guint           code)
{
  ManetteEvdevBackend *self = MANETTE_EVDEV_BACKEND (backend);

  return libevdev_has_event_code (self->evdev_device, type, code);
}

static gboolean
manette_evdev_backend_has_rumble (ManetteBackend *backend)
{
  ManetteEvdevBackend *self = MANETTE_EVDEV_BACKEND (backend);
  gulong features[4];

  if (ioctl (self->fd, EVIOCGBIT (EV_FF, sizeof (gulong) * 4), features) == -1)
    return FALSE;

  if (!((features[FF_RUMBLE / (sizeof (glong) * 8)] >> FF_RUMBLE % (sizeof (glong) * 8)) & 1))
    return FALSE;

  return TRUE;
}

static gboolean
manette_evdev_backend_rumble (ManetteBackend *backend,
                              guint16         strong_magnitude,
                              guint16         weak_magnitude,
                              guint16         milliseconds)
{
  ManetteEvdevBackend *self = MANETTE_EVDEV_BACKEND (backend);
  struct input_event event;

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

static void
manette_evdev_backend_backend_init (ManetteBackendInterface *iface)
{
  iface->initialize = manette_evdev_backend_initialize;
  iface->get_name = manette_evdev_backend_get_name;
  iface->get_vendor_id = manette_evdev_backend_get_vendor_id;
  iface->get_product_id = manette_evdev_backend_get_product_id;
  iface->get_bustype_id = manette_evdev_backend_get_bustype_id;
  iface->get_version_id = manette_evdev_backend_get_version_id;
  iface->set_mapping = manette_evdev_backend_set_mapping;
  iface->has_button = manette_evdev_backend_has_button;
  iface->has_axis = manette_evdev_backend_has_axis;
  iface->has_input = manette_evdev_backend_has_input;
  iface->has_rumble = manette_evdev_backend_has_rumble;
  iface->rumble = manette_evdev_backend_rumble;
}

ManetteBackend *
manette_evdev_backend_new (const char *filename)
{
  ManetteEvdevBackend *self = g_object_new (MANETTE_TYPE_EVDEV_BACKEND, NULL);

  self->filename = g_strdup (filename);

  return MANETTE_BACKEND (self);
}
