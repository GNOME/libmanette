/* manette-event-private.h
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

#if !defined(MANETTE_COMPILATION)
# error "This file is private, only <libmanette.h> can be included directly."
#endif

#include "manette-version.h"

#include <glib-object.h>

#include "manette-event-private.h"

G_BEGIN_DECLS

typedef enum {
  MANETTE_EVENT_NOTHING = -1,
  MANETTE_EVENT_BUTTON_PRESS = 0,
  MANETTE_EVENT_BUTTON_RELEASE = 1,
  MANETTE_EVENT_ABSOLUTE = 2,
  MANETTE_EVENT_HAT = 3,
  /*< private >*/
  MANETTE_LAST_EVENT,
} ManetteEventType;

typedef struct _ManetteEventAny ManetteEventAny;
typedef struct _ManetteEventButton ManetteEventButton;
typedef struct _ManetteEventAbsolute ManetteEventAbsolute;
typedef struct _ManetteEventHat ManetteEventHat;

struct _ManetteEventAny {
  ManetteEventType type;
  guint32 time;
  guint8 hardware_index;
};

struct _ManetteEventButton {
  ManetteEventType type;
  guint32 time;
  guint8 hardware_index;

  guint16 button;
};

struct _ManetteEventAbsolute {
  ManetteEventType type;
  guint32 time;
  guint8 hardware_index;

  guint16 axis;
  double value;
};

struct _ManetteEventHat {
  ManetteEventType type;
  guint32 time;
  guint8 hardware_index;

  guint16 axis;
  gint8 value;
};

union _ManetteEvent {
  ManetteEventAny any;
  ManetteEventButton button;
  ManetteEventAbsolute absolute;
  ManetteEventHat hat;
};

#define MANETTE_TYPE_EVENT (manette_event_get_type())

typedef union _ManetteEvent ManetteEvent;

GType manette_event_get_type (void) G_GNUC_CONST;

ManetteEvent *manette_event_copy (const ManetteEvent *self);

void manette_event_free (ManetteEvent *self);

G_DEFINE_AUTOPTR_CLEANUP_FUNC (ManetteEvent, manette_event_free)

G_END_DECLS

G_END_DECLS
