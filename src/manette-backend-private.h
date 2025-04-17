/* manette-backend-private.h
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

#include "manette-inputs.h"
#include "manette-mapping-private.h"

G_BEGIN_DECLS

#define MANETTE_TYPE_BACKEND (manette_backend_get_type ())

G_DECLARE_INTERFACE (ManetteBackend, manette_backend, MANETTE, BACKEND, GObject)

struct _ManetteBackendInterface
{
  GTypeInterface parent;

  gboolean (* initialize) (ManetteBackend *self);

  const char * (* get_name) (ManetteBackend *self);
  int (* get_vendor_id) (ManetteBackend *self);
  int (* get_product_id) (ManetteBackend *self);
  int (* get_bustype_id) (ManetteBackend *self);
  int (* get_version_id) (ManetteBackend *self);

  void (* set_mapping) (ManetteBackend *self,
                        ManetteMapping *mapping);

  gboolean (* has_button) (ManetteBackend *self,
                           ManetteButton  button);
  gboolean (* has_axis)   (ManetteBackend *self,
                           ManetteAxis     axis);

  gboolean (* has_input) (ManetteBackend *self,
                          guint           type,
                          guint           code);

  gboolean (* has_rumble) (ManetteBackend *self);
  gboolean (* rumble) (ManetteBackend *self,
                       guint16         strong_magnitude,
                       guint16         weak_magnitude,
                       guint16         milliseconds);
};

gboolean manette_backend_initialize (ManetteBackend *self);

const char *manette_backend_get_name (ManetteBackend *self);
int manette_backend_get_vendor_id (ManetteBackend *self);
int manette_backend_get_product_id (ManetteBackend *self);
int manette_backend_get_bustype_id (ManetteBackend *self);
int manette_backend_get_version_id (ManetteBackend *self);

void manette_backend_set_mapping (ManetteBackend *self,
                                  ManetteMapping *mapping);

gboolean manette_backend_has_button (ManetteBackend *self,
                                     ManetteButton   button);
gboolean manette_backend_has_axis   (ManetteBackend *self,
                                     ManetteAxis     axis);

gboolean manette_backend_has_input (ManetteBackend *self,
                                    guint           type,
                                    guint           code);

gboolean manette_backend_has_rumble (ManetteBackend *self);
gboolean manette_backend_rumble     (ManetteBackend *self,
                                     guint16         strong_magnitude,
                                     guint16         weak_magnitude,
                                     guint16         milliseconds);

void manette_backend_emit_button_event (ManetteBackend *self,
                                        guint64         time,
                                        ManetteButton   button,
                                        gboolean        pressed);
void manette_backend_emit_axis_event   (ManetteBackend *self,
                                        guint64         time,
                                        ManetteAxis     axis,
                                        double          value);

void manette_backend_emit_unmapped_button_event   (ManetteBackend *self,
                                                   guint64         time,
                                                   guint           index,
                                                   gboolean        pressed);
void manette_backend_emit_unmapped_absolute_event (ManetteBackend *self,
                                                   guint64         time,
                                                   guint           index,
                                                   double          value);
void manette_backend_emit_unmapped_hat_event      (ManetteBackend *self,
                                                   guint64         time,
                                                   guint           index,
                                                   gint8           value);

G_END_DECLS
