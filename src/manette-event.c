/* manette-event.c
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
 * SECTION:manette-event
 * @short_description: An event emitted by a device
 * @title: ManetteEvent
 * @See_also: #ManetteDevice
 */

#include "manette-event-private.h"

#include <string.h>

G_DEFINE_BOXED_TYPE (ManetteEvent, manette_event, manette_event_copy, manette_event_free)

/**
 * manette_event_new:
 *
 * Creates a new #ManetteEvent.
 *
 * Returns: (transfer full): a new #ManetteEvent
 */
ManetteEvent *
manette_event_new (void)
{
  return g_slice_new0 (ManetteEvent);
}

/**
 * manette_event_copy: (skip)
 * @self: a #ManetteEvent
 *
 * Creates a copy of a #ManetteEvent.
 *
 * Returns: (transfer full): a new #ManetteEvent
 */
ManetteEvent *
manette_event_copy (const ManetteEvent *self)
{
  ManetteEvent *copy;

  g_return_val_if_fail (self, NULL);

  copy = manette_event_new ();
  memcpy(copy, self, sizeof (ManetteEvent));
  if (self->any.device != NULL)
    copy->any.device = g_object_ref (self->any.device);

  return copy;
}

/**
 * manette_event_free: (skip)
 * @self: a #ManetteEvent
 *
 * Frees @self.
 */
void
manette_event_free (ManetteEvent *self)
{
  g_return_if_fail (self);

  g_clear_object (&self->any.device);

  g_slice_free (ManetteEvent, self);
}

/**
 * manette_event_get_event_type:
 * @self: a #ManetteEvent
 *
 * Gets the event type of @self.
 *
 * Returns: the event type of @self
 */
ManetteEventType
manette_event_get_event_type (const ManetteEvent *self)
{
  g_return_val_if_fail (self, MANETTE_EVENT_NOTHING);

  return self->any.type;
}

/**
 * manette_event_get_time:
 * @self: a #ManetteEvent
 *
 * Gets the timestamp of when @self was received by the input driver that takes
 * care of its device. Use this timestamp to ensure external factors such as
 * synchronous disk writes don't influence your timing computations.
 *
 * Returns: the timestamp of when @self was received by the input driver
 */
guint32
manette_event_get_time (const ManetteEvent *self)
{
  g_return_val_if_fail (self, 0);

  return self->any.time;
}

/**
 * manette_event_get_device:
 * @self: a #ManetteEvent
 *
 * Gets the #ManetteDevice associated with the @self.
 *
 * Returns: (transfer none): the #ManetteDevice associated with the @self
 */
ManetteDevice *
manette_event_get_device (const ManetteEvent *self)
{
  g_return_val_if_fail (self, NULL);

  return self->any.device;
}

/**
 * manette_event_get_hardware_type:
 * @self: a #ManetteEvent
 *
 * Gets the hardware type of @self.
 *
 * Returns: the hardware type of @self
 */
guint16
manette_event_get_hardware_type (const ManetteEvent *self)
{
  g_return_val_if_fail (self, 0);

  return self->any.hardware_type;
}

/**
 * manette_event_get_hardware_code:
 * @self: a #ManetteEvent
 *
 * Gets the hardware code of @self.
 *
 * Returns: the hardware code of @self
 */
guint16
manette_event_get_hardware_code (const ManetteEvent *self)
{
  g_return_val_if_fail (self, 0);

  return self->any.hardware_code;
}

/**
 * manette_event_get_hardware_value:
 * @self: a #ManetteEvent
 *
 * Gets the hardware value of @self.
 *
 * Returns: the hardware value of @self
 */
guint16
manette_event_get_hardware_value (const ManetteEvent *self)
{
  g_return_val_if_fail (self, 0);

  return self->any.hardware_value;
}

/**
 * manette_event_get_hardware_index:
 * @self: a #ManetteEvent
 *
 * Gets the hardware index of @self.
 *
 * Returns: the hardware index of @self
 */
guint16
manette_event_get_hardware_index (const ManetteEvent *self)
{
  g_return_val_if_fail (self, 0);

  return self->any.hardware_index;
}

/**
 * manette_event_get_button:
 * @self: a #ManetteEvent
 * @button: (out): return location for the button
 *
 * Gets the button of @self, if any.
 *
 * Returns: whether the button was retrieved
 */
gboolean
manette_event_get_button (const ManetteEvent *self,
                          guint16            *button)
{
  g_return_val_if_fail (self, FALSE);
  g_return_val_if_fail (button, FALSE);

  switch (self->any.type) {
  case MANETTE_EVENT_BUTTON_PRESS:
  case MANETTE_EVENT_BUTTON_RELEASE:
    *button = self->button.button;

    return TRUE;
  default:
    return FALSE;
  }
}

/**
 * manette_event_get_absolute:
 * @self: a #ManetteEvent
 * @axis: (out): return location for the axis
 * @value: (out): return location for the axis
 *
 * Gets the axis of @self, if any.
 *
 * Returns: whether the axis was retrieved
 */
gboolean
manette_event_get_absolute (const ManetteEvent *self,
                            guint16            *axis,
                            gdouble            *value)
{
  g_return_val_if_fail (self, FALSE);
  g_return_val_if_fail (axis, FALSE);
  g_return_val_if_fail (value, FALSE);

  switch (self->any.type) {
  case MANETTE_EVENT_ABSOLUTE:
    *axis = self->absolute.axis;
    *value = self->absolute.value;

    return TRUE;
  default:
    return FALSE;
  }
}

/**
 * manette_event_get_hat:
 * @self: a #ManetteEvent
 * @axis: (out): return location for the hat
 * @value: (out): return location for the hat
 *
 * Gets the hat of @self, if any.
 *
 * Returns: whether the hat was retrieved
 */
gboolean
manette_event_get_hat (const ManetteEvent *self,
                       guint16            *axis,
                       gint8              *value)
{
  g_return_val_if_fail (self, FALSE);
  g_return_val_if_fail (axis, FALSE);
  g_return_val_if_fail (value, FALSE);

  switch (self->any.type) {
  case MANETTE_EVENT_HAT:
    *axis = self->hat.axis;
    *value = self->hat.value;

    return TRUE;
  default:
    return FALSE;
  }
}

GType
manette_event_type_get_type (void)
{
  static volatile gsize manette_event_type_type = 0;

  if (g_once_init_enter (&manette_event_type_type)) {
    static const GEnumValue values[] = {
      { MANETTE_EVENT_NOTHING, "MANETTE_EVENT_NOTHING", "event-nothing" },
      { MANETTE_EVENT_BUTTON_PRESS, "MANETTE_EVENT_BUTTON_PRESS", "event-button-press" },
      { MANETTE_EVENT_BUTTON_RELEASE, "MANETTE_EVENT_BUTTON_RELEASE", "event-button-release" },
      { MANETTE_EVENT_ABSOLUTE, "MANETTE_EVENT_ABSOLUTE", "event-absolute" },
      { MANETTE_EVENT_HAT, "MANETTE_EVENT_HAT", "event-hat" },
      { 0, NULL, NULL },
    };
    GType type;

    type = g_enum_register_static ("ManetteEventType", values);

    g_once_init_leave (&manette_event_type_type, type);
  }

  return manette_event_type_type;
}
