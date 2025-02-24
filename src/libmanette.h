/* libmanette.h
 *
 * Copyright (C) 2017 Adrien Plazas
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 2.1 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include <glib.h>

G_BEGIN_DECLS

#define __MANETTE_INSIDE__
# include "manette-device.h"
# include "manette-device-type.h"
# include "manette-inputs.h"
# include "manette-monitor.h"
# include "manette-version.h"
#undef __MANETTE_INSIDE__

G_END_DECLS
