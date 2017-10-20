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
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef MANETTE_DEVICE_H
#define MANETTE_DEVICE_H

#if !defined(__MANETTE_INSIDE__) && !defined(MANETTE_COMPILATION)
# error "Only <libmanette.h> can be included directly."
#endif

#include <glib-object.h>

G_BEGIN_DECLS

#define MANETTE_TYPE_DEVICE (manette_device_get_type())

G_DECLARE_FINAL_TYPE (ManetteDevice, manette_device, MANETTE, DEVICE, GObject)

const gchar *manette_device_get_name (ManetteDevice *self);

G_END_DECLS

#endif /* MANETTE_DEVICE_H */
