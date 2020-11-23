/* manette-device-private.h
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

#include "manette-device.h"
#include "manette-mapping-private.h"

G_BEGIN_DECLS

ManetteDevice *manette_device_new (const gchar  *filename,
                                   GError      **error);
int manette_device_get_product_id (ManetteDevice *self);
int manette_device_get_vendor_id (ManetteDevice *self);
int manette_device_get_bustype_id (ManetteDevice *self);
int manette_device_get_version_id (ManetteDevice *self);
const gchar *manette_device_get_guid (ManetteDevice *self);
void manette_device_set_mapping (ManetteDevice  *self,
                                 ManetteMapping *mapping);

G_END_DECLS
