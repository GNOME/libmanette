/* manette-monitor-iter.c
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

/**
 * SECTION:manette-monitor-iter
 * @short_description: An object iterating over the available devices
 * @title: ManetteMonitorIter
 * @See_also: #ManetteDevice, #ManetteMonitor
 */

#include "manette-monitor-iter-private.h"

struct _ManetteMonitorIter
{
  GHashTableIter iter;
};

G_DEFINE_BOXED_TYPE (ManetteMonitorIter, manette_monitor_iter, manette_monitor_iter_copy, manette_monitor_iter_free)

/* Private */

/**
 * manette_monitor_iter_new:
 * @devices: (element-type utf8 ManetteDevice): a #GHashTable
 *
 * Creates a new #ManetteMonitorIter.
 *
 * Returns: (transfer full): a new #ManetteMonitorIter
 */
ManetteMonitorIter *
manette_monitor_iter_new (GHashTable *devices)
{
  ManetteMonitorIter *self;

  self = g_slice_new0 (ManetteMonitorIter);
  g_hash_table_ref (devices);
  g_hash_table_iter_init (&self->iter, devices);

  return self;
}

/**
 * manette_monitor_iter_copy: (skip)
 * @self: a #ManetteMonitorIter
 *
 * Creates a copy of a #ManetteMonitorIter.
 *
 * Returns: (transfer full): a new #ManetteMonitorIter
 */
ManetteMonitorIter *
manette_monitor_iter_copy (ManetteMonitorIter *self)
{
  GHashTable *devices;

  ManetteMonitorIter *copy;

  g_return_val_if_fail (self, NULL);

  devices = g_hash_table_iter_get_hash_table (&self->iter);
  copy = manette_monitor_iter_new (devices);

  return copy;
}

/* Public */

/**
 * manette_monitor_iter_free: (skip)
 * @self: a #ManetteMonitorIter
 *
 * Frees a #ManetteMonitorIter.
 */
void
manette_monitor_iter_free (ManetteMonitorIter *self)
{
  GHashTable *devices;

  g_return_if_fail (self);

  devices = g_hash_table_iter_get_hash_table (&self->iter);
  g_hash_table_unref (devices);

  g_slice_free (ManetteMonitorIter, self);
}

/**
 * manette_monitor_iter_next:
 * @self: a #ManetteMonitorIter
 * @device: (out) (nullable) (transfer none): return location for the device
 *
 * Gets the next device from the device monitor iterator.
 *
 * Returns: whether the next device was retrieved, if not, the end was reached
 */
gboolean
manette_monitor_iter_next (ManetteMonitorIter  *self,
                           ManetteDevice      **device)
{
  g_return_val_if_fail (self, FALSE);

  return g_hash_table_iter_next (&self->iter, NULL, (gpointer) device);
}
