/* manette-inputs.h
 *
 * Copyright (C) 2025 Alice Mikhaylenko <alicem@gnome.org>
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
# error "This file is private, only <libmanette.h> can be included directly."
#endif

#include "manette-version.h"

#include <glib-object.h>

#include "manette-enums.h"

G_BEGIN_DECLS

typedef enum {
  MANETTE_BUTTON_DPAD_UP,
  MANETTE_BUTTON_DPAD_DOWN,
  MANETTE_BUTTON_DPAD_LEFT,
  MANETTE_BUTTON_DPAD_RIGHT,
  MANETTE_BUTTON_NORTH,
  MANETTE_BUTTON_SOUTH,
  MANETTE_BUTTON_WEST,
  MANETTE_BUTTON_EAST,
  MANETTE_BUTTON_SELECT,
  MANETTE_BUTTON_START,
  MANETTE_BUTTON_MODE,
  MANETTE_BUTTON_LEFT_SHOULDER,
  MANETTE_BUTTON_RIGHT_SHOULDER,
  MANETTE_BUTTON_LEFT_STICK,
  MANETTE_BUTTON_RIGHT_STICK,
  MANETTE_BUTTON_LEFT_PADDLE1,
  MANETTE_BUTTON_LEFT_PADDLE2,
  MANETTE_BUTTON_RIGHT_PADDLE1,
  MANETTE_BUTTON_RIGHT_PADDLE2,
  MANETTE_BUTTON_MISC1,
  MANETTE_BUTTON_MISC2,
  MANETTE_BUTTON_MISC3,
  MANETTE_BUTTON_MISC4,
  MANETTE_BUTTON_MISC5,
  MANETTE_BUTTON_MISC6,
  MANETTE_BUTTON_TOUCHPAD,
} ManetteButton;

typedef enum {
  MANETTE_AXIS_LEFT_X,
  MANETTE_AXIS_LEFT_Y,
  MANETTE_AXIS_RIGHT_X,
  MANETTE_AXIS_RIGHT_Y,
  MANETTE_AXIS_LEFT_TRIGGER,
  MANETTE_AXIS_RIGHT_TRIGGER,
} ManetteAxis;

G_END_DECLS
