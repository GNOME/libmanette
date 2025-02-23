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

#include "config.h"

#include "manette-device-type-private.h"

/**
 * ManetteDeviceType:
 * @MANETTE_DEVICE_GENERIC: Generic gamepads
 * @MANETTE_DEVICE_STEAM_DECK: Steam Deck
 *
 * Describes available types of a [class@Device].
 *
 * More values may be added to this enumeration over time.
 */

#define VENDOR_STEAM                  0x28DE
#define PRODUCT_JUPITER               0x1205
#define PRODUCT_STEAM_VIRTUAL_GAMEPAD 0x11FF

ManetteDeviceType
manette_device_type_guess (guint16 vendor,
                           guint16 product)
{
  if (vendor == VENDOR_STEAM) {
    if (product == PRODUCT_JUPITER)
      return MANETTE_DEVICE_STEAM_DECK;

    if (product == PRODUCT_STEAM_VIRTUAL_GAMEPAD)
      return MANETTE_DEVICE_UNSUPPORTED;
  }

  return MANETTE_DEVICE_GENERIC;
}
