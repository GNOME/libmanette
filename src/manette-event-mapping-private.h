/* manette-event-mapping-private.h
 *
 * Copyright (C) 2018 Adrien Plazas <kekun.plazas@laposte.net>
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

#include "manette-mapping-private.h"

#include "manette-inputs.h"

G_BEGIN_DECLS

typedef union {
  ManetteMappingDestinationType type;
  struct {
    ManetteMappingDestinationType type;
    ManetteButton button;
    gboolean pressed;
  } button;
  struct {
    ManetteMappingDestinationType type;
    ManetteAxis axis;
    double value;
  } axis;
} ManetteMappedEvent;

GSList *manette_map_button_event (ManetteMapping *mapping,
                                  guint           index,
                                  gboolean        pressed);

GSList *manette_map_absolute_event (ManetteMapping *mapping,
                                    guint           index,
                                    double          value);

GSList *manette_map_hat_event (ManetteMapping *mapping,
                               guint           index,
                               gint8           value);

G_END_DECLS
