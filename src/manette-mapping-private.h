/* manette-mapping-private.h
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

#include "manette-inputs.h"
#include "manette-mapping-error-private.h"

G_BEGIN_DECLS

#define MANETTE_TYPE_MAPPING (manette_mapping_get_type())

G_DECLARE_FINAL_TYPE (ManetteMapping, manette_mapping, MANETTE, MAPPING, GObject)

typedef struct _ManetteMappingBinding ManetteMappingBinding;

typedef enum {
  MANETTE_MAPPING_INPUT_TYPE_AXIS,
  MANETTE_MAPPING_INPUT_TYPE_BUTTON,
  MANETTE_MAPPING_INPUT_TYPE_HAT,
} ManetteMappingInputType;

typedef enum {
  MANETTE_MAPPING_DESTINATION_TYPE_AXIS,
  MANETTE_MAPPING_DESTINATION_TYPE_BUTTON,
} ManetteMappingDestinationType;

typedef enum {
  MANETTE_MAPPING_RANGE_NEGATIVE = -1,
  MANETTE_MAPPING_RANGE_FULL = 0,
  MANETTE_MAPPING_RANGE_POSITIVE = 1,
} ManetteMappingRange;

struct _ManetteMappingBinding {
  struct {
    ManetteMappingInputType type;
    guint16 index;
    ManetteMappingRange range;
    gboolean invert;
  } source;
  struct {
    ManetteMappingDestinationType type;
    int code;
    ManetteMappingRange range;
  } destination;
};

ManetteMapping *manette_mapping_new (const char  *mapping_string,
                                     GError     **error);
const ManetteMappingBinding * const *manette_mapping_get_bindings (ManetteMapping          *self,
                                                                   ManetteMappingInputType  type,
                                                                   guint16                  index);

ManetteMappingBinding *manette_mapping_binding_copy (ManetteMappingBinding *self);
void manette_mapping_binding_free (ManetteMappingBinding *self);
gboolean manette_mapping_has_destination_button (ManetteMapping *self,
                                                 ManetteButton   button);
gboolean manette_mapping_has_destination_axis (ManetteMapping *self,
                                               ManetteAxis     axis);

G_END_DECLS
