/* manette-device.h
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

#define MANETTE_TYPE_DEVICE (manette_device_get_type())

G_DECLARE_FINAL_TYPE (ManetteDevice, manette_device, MANETTE, DEVICE, GObject)

gboolean manette_device_has_input (ManetteDevice *self,
                                   guint          type,
                                   guint          code);
const gchar *manette_device_get_name (ManetteDevice *self);
gboolean manette_device_has_user_mapping (ManetteDevice *self);
void manette_device_save_user_mapping (ManetteDevice *self,
                                       const gchar   *mapping_string);
void manette_device_remove_user_mapping (ManetteDevice *self);
gboolean manette_device_has_rumble (ManetteDevice *self);
gboolean manette_device_rumble (ManetteDevice *self,
                                guint16        strong_magnitude,
                                guint16        weak_magnitude,
                                guint16        milliseconds);

G_END_DECLS
