/* manette-event.h
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

#pragma once

#if !defined(__MANETTE_INSIDE__) && !defined(MANETTE_COMPILATION)
# error "Only <libmanette.h> can be included directly."
#endif

#include <glib-object.h>
#include "manette-device.h"

G_BEGIN_DECLS

#define MANETTE_TYPE_EVENT (manette_event_get_type())

#define MANETTE_TYPE_EVENT_TYPE (manette_event_type_get_type ())

typedef union  _ManetteEvent ManetteEvent;

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
typedef enum {
  MANETTE_EVENT_NOTHING = -1,
  MANETTE_EVENT_BUTTON_PRESS = 0,
  MANETTE_EVENT_BUTTON_RELEASE = 1,
  MANETTE_EVENT_ABSOLUTE = 2,
  MANETTE_EVENT_HAT = 3,
  MANETTE_LAST_EVENT,
} ManetteEventType;

GType manette_event_get_type (void) G_GNUC_CONST;
GType manette_event_type_get_type (void) G_GNUC_CONST;

ManetteEvent *manette_event_copy (const ManetteEvent *self);
void manette_event_free (ManetteEvent *self);
ManetteEventType manette_event_get_event_type (const ManetteEvent *self);
guint32 manette_event_get_time (const ManetteEvent *self);
ManetteDevice *manette_event_get_device (const ManetteEvent *self);
guint16 manette_event_get_hardware_type (const ManetteEvent *self);
guint16 manette_event_get_hardware_code (const ManetteEvent *self);
guint16 manette_event_get_hardware_value (const ManetteEvent *self);
guint16 manette_event_get_hardware_index (const ManetteEvent *self);
gboolean manette_event_get_button (const ManetteEvent *self,
                                   guint16            *button);
gboolean manette_event_get_absolute (const ManetteEvent *self,
                                     guint16            *axis,
                                     gdouble            *value);
gboolean manette_event_get_hat (const ManetteEvent *self,
                                guint16            *axis,
                                gint8              *value);

G_DEFINE_AUTOPTR_CLEANUP_FUNC (ManetteEvent, manette_event_free)

G_END_DECLS
