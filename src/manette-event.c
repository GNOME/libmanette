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
 * @MANETTE_EVENT_BUTTON_PRESS: a button has been pressed
 * @MANETTE_EVENT_BUTTON_RELEASE: a button has been released
 * @MANETTE_EVENT_ABSOLUTE: an absolute axis has been moved
 * @MANETTE_EVENT_HAT: a hat axis has been moved
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

  g_slice_free (ManetteEvent, self);
}
