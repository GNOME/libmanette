/* manette-monitor-iter.h
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
#include "manette-device.h"

G_BEGIN_DECLS

#define MANETTE_TYPE_MONITOR_ITER (manette_monitor_iter_get_type())

GType manette_monitor_iter_get_type (void);

typedef struct _ManetteMonitorIter ManetteMonitorIter;

gboolean manette_monitor_iter_next (ManetteMonitorIter  *self,
                                    ManetteDevice      **device);
void manette_monitor_iter_free (ManetteMonitorIter *self);

G_DEFINE_AUTOPTR_CLEANUP_FUNC (ManetteMonitorIter, manette_monitor_iter_free)

G_END_DECLS
