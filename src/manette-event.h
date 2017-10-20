/* manette-event.h
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

#ifndef MANETTE_EVENT_H
#define MANETTE_EVENT_H

#if !defined(__MANETTE_INSIDE__) && !defined(MANETTE_COMPILATION)
# error "Only <libmanette.h> can be included directly."
#endif

#include <glib-object.h>
#include "manette-device.h"

G_BEGIN_DECLS

#define MANETTE_TYPE_EVENT (manette_event_get_type())

typedef union  _ManetteEvent ManetteEvent;

GType manette_event_get_type (void) G_GNUC_CONST;

ManetteEvent *manette_event_copy (const ManetteEvent *self);
void manette_event_free (ManetteEvent *self);
ManetteDevice *manette_event_get_device (const ManetteEvent *self);
gboolean manette_event_get_button (const ManetteEvent *self,
                                   guint16            *button);
gboolean manette_event_get_absolute (const ManetteEvent *self,
                                     guint16            *axis,
                                     gdouble            *value);
gboolean manette_event_get_hat (const ManetteEvent *self,
                                guint16            *axis,
                                gint8              *value);

G_DEFINE_AUTOPTR_CLEANUP_FUNC (ManetteEvent, manette_event_free)

G_END_DECLS

#endif /* MANETTE_EVENT_H */
