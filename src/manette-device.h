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

#include "manette-version.h"

#include <glib-object.h>

#include "manette-device-type.h"
#include "manette-inputs.h"

G_BEGIN_DECLS

#define MANETTE_TYPE_DEVICE (manette_device_get_type())

MANETTE_AVAILABLE_IN_ALL
G_DECLARE_FINAL_TYPE (ManetteDevice, manette_device, MANETTE, DEVICE, GObject)

MANETTE_AVAILABLE_IN_ALL
gboolean manette_device_has_button (ManetteDevice *self,
                                    ManetteButton  button);

MANETTE_AVAILABLE_IN_ALL
gboolean manette_device_has_axis (ManetteDevice *self,
                                  ManetteAxis    axis);

MANETTE_AVAILABLE_IN_ALL
gboolean manette_device_has_input (ManetteDevice *self,
                                   guint          type,
                                   guint          code);

MANETTE_AVAILABLE_IN_ALL
const char *manette_device_get_name (ManetteDevice *self);

MANETTE_AVAILABLE_IN_ALL
const char *manette_device_get_guid (ManetteDevice *self);

MANETTE_AVAILABLE_IN_ALL
ManetteDeviceType manette_device_get_device_type (ManetteDevice *self);

MANETTE_AVAILABLE_IN_ALL
guint64 manette_device_get_current_event_time (ManetteDevice *self);

MANETTE_AVAILABLE_IN_ALL
gboolean manette_device_supports_mapping (ManetteDevice *self);

MANETTE_AVAILABLE_IN_ALL
char *manette_device_get_mapping (ManetteDevice *self);

MANETTE_AVAILABLE_IN_ALL
gboolean manette_device_has_user_mapping (ManetteDevice *self);

MANETTE_AVAILABLE_IN_ALL
void manette_device_save_user_mapping (ManetteDevice *self,
                                       const char   *mapping_string);

MANETTE_AVAILABLE_IN_ALL
void manette_device_remove_user_mapping (ManetteDevice *self);

MANETTE_AVAILABLE_IN_ALL
gboolean manette_device_has_rumble (ManetteDevice *self);

MANETTE_AVAILABLE_IN_ALL
gboolean manette_device_rumble (ManetteDevice *self,
                                double         strong_magnitude,
                                double         weak_magnitude,
                                guint16        milliseconds);

G_END_DECLS
