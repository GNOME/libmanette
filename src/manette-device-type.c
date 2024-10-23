/* manette-device-type.c
 *
 * Copyright (C) 2024 Alice Mikhaylenko <alicem@gnome.org>
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

#include "manette-device-type-private.h"

/**
 * ManetteDeviceType:
 * @MANETTE_DEVICE_GENERIC: Generic gamepads
 *
 * Describes available types of a #ManetteDevice.
 *
 * More values may be added to this enumeration over time.
 *
 * Since: 0.3
 */

G_DEFINE_ENUM_TYPE (ManetteDeviceType, manette_device_type,
  G_DEFINE_ENUM_VALUE (MANETTE_DEVICE_GENERIC, "generic"))

ManetteDeviceType
manette_device_type_guess (guint16 vendor,
                           guint16 product)
{
  return MANETTE_DEVICE_GENERIC;
}
