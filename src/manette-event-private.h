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

#include "manette-event.h"

G_BEGIN_DECLS

typedef struct _ManetteEventAny ManetteEventAny;
typedef struct _ManetteEventButton ManetteEventButton;
typedef struct _ManetteEventAbsolute ManetteEventAbsolute;
typedef struct _ManetteEventHat ManetteEventHat;

struct _ManetteEventAny {
  ManetteEventType type;
  guint32 time;
  ManetteDevice *device;
  guint16 hardware_type;
  guint16 hardware_code;
  gint32 hardware_value;
  guint8 hardware_index;
};

struct _ManetteEventButton {
  ManetteEventType type;
  guint32 time;
  ManetteDevice *device;
  guint16 hardware_type;
  guint16 hardware_code;
  gint32 hardware_value;
  guint8 hardware_index;
  guint16 button;
};

struct _ManetteEventAbsolute {
  ManetteEventType type;
  guint32 time;
  ManetteDevice *device;
  guint16 hardware_type;
  guint16 hardware_code;
  gint32 hardware_value;
  guint8 hardware_index;
  guint16 axis;
  gdouble value;
};

struct _ManetteEventHat {
  ManetteEventType type;
  guint32 time;
  ManetteDevice *device;
  guint16 hardware_type;
  guint16 hardware_code;
  gint32 hardware_value;
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

G_END_DECLS
