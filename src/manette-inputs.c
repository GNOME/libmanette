/* manette-inputs.c
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

#include "config.h"

#include "manette-inputs.h"

/**
 * ManetteButton:
 * @MANETTE_BUTTON_DPAD_UP: D-pad (up)
 * @MANETTE_BUTTON_DPAD_DOWN: D-pad (down)
 * @MANETTE_BUTTON_DPAD_LEFT: D-pad (left)
 * @MANETTE_BUTTON_DPAD_RIGHT: D-pad (right)
 * @MANETTE_BUTTON_NORTH: Top face button
 *     (XBox Y, Nintendo X, PlayStation triangle)
 * @MANETTE_BUTTON_SOUTH: Bottom face button
 *     (XBox A, Nintendo B, PlayStation X)
 * @MANETTE_BUTTON_WEST: Left face button
 *     (XBox X, Nintendo Y, PlayStation square)
 * @MANETTE_BUTTON_EAST: Right face button
 *     (XBox B, Nintendo A, PlayStation circle)
 * @MANETTE_BUTTON_SELECT: Left menu button
 * @MANETTE_BUTTON_START: Right menu button
 * @MANETTE_BUTTON_MODE: Center menu button (Home, Guide, Steam etc)
 * @MANETTE_BUTTON_LEFT_SHOULDER: Left shoulder button (L, L1 or LB)
 * @MANETTE_BUTTON_RIGHT_SHOULDER: Right shoulder button (R, R1 or RB)
 * @MANETTE_BUTTON_LEFT_STICK: Left stick
 * @MANETTE_BUTTON_RIGHT_STICK: Right stick
 * @MANETTE_BUTTON_LEFT_PADDLE1: Upper left paddle
 *     (Steam Deck L4 or XBox Elite P3)
 * @MANETTE_BUTTON_LEFT_PADDLE2: Lower left paddle
 *     (Steam Deck L5 or XBox Elite P4)
 * @MANETTE_BUTTON_RIGHT_PADDLE1: Upper right paddle
 *     (Steam Deck R4 or XBox Elite P1)
 * @MANETTE_BUTTON_RIGHT_PADDLE2: Lower right paddle
 *     (Steam Deck R5 or XBox Elite P2)
 * @MANETTE_BUTTON_MISC1: Additional button
 *     (Steam Deck QAM button, Xbox Series X share button etc)
 * @MANETTE_BUTTON_MISC2: Additional button
 * @MANETTE_BUTTON_MISC3: Additional button
 * @MANETTE_BUTTON_MISC4: Additional button
 * @MANETTE_BUTTON_MISC5: Additional button
 * @MANETTE_BUTTON_MISC6: Additional button
 * @MANETTE_BUTTON_TOUCHPAD: PS4/PS5 touchpad button
 *
 * Describes available buttons a [class@Device] can have.
 *
 * More values may be added to this enumeration over time.
 */

/**
 * ManetteAxis:
 * @MANETTE_AXIS_LEFT_X: Left analog stick, horizontal axis
 * @MANETTE_AXIS_LEFT_Y: Left analog stick, vertical axis
 * @MANETTE_AXIS_RIGHT_X: Right analog stick, horizontal axis
 * @MANETTE_AXIS_RIGHT_Y: Right analog stick, vertical axis
 * @MANETTE_AXIS_LEFT_TRIGGER: Left trigger (L2, LT or ZL)
 * @MANETTE_AXIS_RIGHT_TRIGGER: Right trigger (R2, RT or ZR)
 *
 * Describes available axes a [class@Device] can have.
 *
 * More values may be added to this enumeration over time.
 */
