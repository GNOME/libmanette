/* manette-mapping-manager-private.h
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

G_BEGIN_DECLS

#define MANETTE_TYPE_MAPPING_MANAGER (manette_mapping_manager_get_type())

G_DECLARE_FINAL_TYPE (ManetteMappingManager, manette_mapping_manager, MANETTE, MAPPING_MANAGER, GObject)

ManetteMappingManager *manette_mapping_manager_new (void);
gboolean manette_mapping_manager_has_user_mapping (ManetteMappingManager *self,
                                                   const char            *guid);
char *manette_mapping_manager_get_default_mapping (ManetteMappingManager *self,
                                                   const char            *guid);
char *manette_mapping_manager_get_user_mapping (ManetteMappingManager *self,
                                                const char            *guid);
char *manette_mapping_manager_get_mapping (ManetteMappingManager *self,
                                           const char            *guid);
void manette_mapping_manager_save_mapping (ManetteMappingManager *self,
                                           const char            *guid,
                                           const char            *name,
                                           const char            *mapping);
void manette_mapping_manager_delete_mapping (ManetteMappingManager *self,
                                             const char            *guid);
GList *manette_mapping_manager_get_default_mappings (ManetteMappingManager *self);

G_END_DECLS
