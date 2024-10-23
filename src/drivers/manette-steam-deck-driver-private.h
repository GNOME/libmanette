/* manette-steam-deck-driver-private.c
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

#pragma once

#if !defined(MANETTE_COMPILATION)
# error "This file is private, only <libmanette.h> can be included directly."
#endif

#include <glib-object.h>
#include <hidapi.h>

#include "manette-hid-driver-private.h"

G_BEGIN_DECLS

#define MANETTE_TYPE_STEAM_DECK_DRIVER (manette_steam_deck_driver_get_type())

G_DECLARE_FINAL_TYPE (ManetteSteamDeckDriver, manette_steam_deck_driver, MANETTE, STEAM_DECK_DRIVER, GObject)

ManetteHidDriver *manette_steam_deck_driver_new (hid_device *hid);

G_END_DECLS
