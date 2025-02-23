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

#include "config.h"

#include "manette-event-private.h"

#include <string.h>

/**
 * ManetteEventType:
 * @MANETTE_EVENT_NOTHING: a special code to indicate a null event
 * @MANETTE_EVENT_BUTTON_PRESS: a button has been pressed
 * @MANETTE_EVENT_BUTTON_RELEASE: a button has been released
 * @MANETTE_EVENT_ABSOLUTE: an absolute axis has been moved
 * @MANETTE_EVENT_HAT: a hat axis has been moved
 * @MANETTE_LAST_EVENT: the number of event types
 *
 * Specifies the type of the event.
 */

/**
 * ManetteEvent:
 *
 * An event emitted by a [class@Device].
 */

G_DEFINE_BOXED_TYPE (ManetteEvent, manette_event, manette_event_copy, manette_event_free)

/**
 * manette_event_new:
 *
 * Creates a new [union@Event].
 *
 * Returns: (transfer full): a new event
 */
ManetteEvent *
manette_event_new (void)
{
  return g_slice_new0 (ManetteEvent);
}

/**
 * manette_event_copy: (skip)
 * @self: an event
 *
 * Creates a copy of @self.
 *
 * Returns: (transfer full): a new event
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
 * @self: an event
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
 * @self: an event
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
 * @self: an event
 *
 * Gets the timestamp of when @self was received by the input driver that takes
 * care of its device.
 *
 * Use this timestamp to ensure external factors such as synchronous disk writes
 * don't influence your timing computations.
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
 * @self: an event
 *
 * Gets the [class@Device] associated with the @self.
 *
 * Returns: (transfer none): the device associated with the @self
 */
ManetteDevice *
manette_event_get_device (const ManetteEvent *self)
{
  g_return_val_if_fail (self, NULL);

  return self->any.device;
}

/**
 * manette_event_get_hardware_type:
 * @self: an event
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
 * @self: an event
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
 * @self: an event
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
 * @self: an event
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
 * @self: an event
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
 * @self: an event
 * @axis: (out): return location for the axis
 * @value: (out): return location for the axis value
 *
 * Gets the axis of @self, if any.
 *
 * Returns: whether the axis was retrieved
 */
gboolean
manette_event_get_absolute (const ManetteEvent *self,
                            guint16            *axis,
                            double             *value)
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
 * @self: an event
 * @axis: (out): return location for the hat
 * @value: (out): return location for the hat value
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
