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

#include "manette-version.h"

#include <glib-object.h>

#include "manette-device.h"
#include "manette-enums.h"

G_BEGIN_DECLS

#define MANETTE_TYPE_EVENT (manette_event_get_type())

#define MANETTE_TYPE_EVENT_TYPE (manette_event_type_get_type())

typedef union  _ManetteEvent ManetteEvent;

typedef enum {
  MANETTE_EVENT_NOTHING = -1,
  MANETTE_EVENT_BUTTON_PRESS = 0,
  MANETTE_EVENT_BUTTON_RELEASE = 1,
  MANETTE_EVENT_ABSOLUTE = 2,
  MANETTE_EVENT_HAT = 3,
  /*< private >*/
  MANETTE_LAST_EVENT,
} ManetteEventType;

MANETTE_AVAILABLE_IN_ALL
GType manette_event_get_type (void) G_GNUC_CONST;

MANETTE_AVAILABLE_IN_ALL
ManetteEvent *manette_event_copy (const ManetteEvent *self);

MANETTE_AVAILABLE_IN_ALL
void manette_event_free (ManetteEvent *self);

MANETTE_AVAILABLE_IN_ALL
ManetteEventType manette_event_get_event_type (const ManetteEvent *self);

MANETTE_AVAILABLE_IN_ALL
guint16 manette_event_get_hardware_index (const ManetteEvent *self);

MANETTE_AVAILABLE_IN_ALL
gboolean manette_event_get_button (const ManetteEvent *self,
                                   guint16            *button);

MANETTE_AVAILABLE_IN_ALL
gboolean manette_event_get_absolute (const ManetteEvent *self,
                                     guint16            *axis,
                                     double             *value);

MANETTE_AVAILABLE_IN_ALL
gboolean manette_event_get_hat (const ManetteEvent *self,
                                guint16            *axis,
                                gint8              *value);

G_DEFINE_AUTOPTR_CLEANUP_FUNC (ManetteEvent, manette_event_free)

G_END_DECLS
