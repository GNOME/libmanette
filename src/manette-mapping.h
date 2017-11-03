/* manette-mapping.h
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
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef MANETTE_MAPPING_H
#define MANETTE_MAPPING_H

#if !defined(__MANETTE_INSIDE__) && !defined(MANETTE_COMPILATION)
# error "Only <libmanette.h> can be included directly."
#endif

#include <glib-object.h>
#include "manette-mapping-error.h"

G_BEGIN_DECLS

#define MANETTE_TYPE_MAPPING (manette_mapping_get_type())

G_DECLARE_FINAL_TYPE (ManetteMapping, manette_mapping, MANETTE, MAPPING, GObject)

typedef enum _ManetteMappingInputType ManetteMappingInputType;
typedef enum _ManetteMappingRange ManetteMappingRange;
typedef struct _ManetteMappingBinding ManetteMappingBinding;

enum _ManetteMappingInputType {
  MANETTE_MAPPING_INPUT_TYPE_AXIS,
  MANETTE_MAPPING_INPUT_TYPE_BUTTON,
  MANETTE_MAPPING_INPUT_TYPE_HAT,
};

enum _ManetteMappingRange {
  MANETTE_MAPPING_RANGE_NEGATIVE = -1,
  MANETTE_MAPPING_RANGE_FULL = 0,
  MANETTE_MAPPING_RANGE_POSITIVE = 1,
};

struct _ManetteMappingBinding {
  struct {
    ManetteMappingInputType type;
    guint16 index;
    ManetteMappingRange range;
    gboolean invert;
  } source;
  struct {
    guint16 type;
    guint16 code;
    ManetteMappingRange range;
  } destination;
};

ManetteMapping *manette_mapping_new (const gchar  *mapping_string,
                                     GError      **error);
const ManetteMappingBinding * const *manette_mapping_get_bindings (ManetteMapping          *self,
                                                                   ManetteMappingInputType  type,
                                                                   guint16                  index);

ManetteMappingBinding *manette_mapping_binding_copy (ManetteMappingBinding *self);
void manette_mapping_binding_free (ManetteMappingBinding *self);
gboolean manette_mapping_has_destination_input (ManetteMapping *self,
                                                guint           type,
                                                guint           code);

G_END_DECLS

#endif /* MANETTE_MAPPING_H */

