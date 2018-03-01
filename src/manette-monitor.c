/* manette-monitor.c
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

#include "manette-monitor.h"

#include <glib.h>
#include <glib-object.h>
#ifdef GUDEV_ENABLED
 #include <gudev/gudev.h>
#endif
#include "manette-device-private.h"
#include "manette-mapping-manager.h"
#include "manette-monitor-iter-private.h"

struct _ManetteMonitor {
  GObject parent_instance;

  GHashTable *devices;
  ManetteMappingManager *mapping_manager;
#ifdef GUDEV_ENABLED
  GUdevClient *client;
#endif
};

G_DEFINE_TYPE (ManetteMonitor, manette_monitor, G_TYPE_OBJECT)

enum {
  SIG_DEVICE_CONNECTED,
  SIG_DEVICE_DISCONNECTED,
  N_SIGNALS,
};

static guint signals[N_SIGNALS];

/* Private */

static void
manette_monitor_init (ManetteMonitor *self)
{
}

static void
load_mapping (ManetteMonitor *self,
              ManetteDevice  *device)
{
  const gchar *guid;
  gchar *mapping_string;
  ManetteMapping *mapping = NULL;
  GError *error = NULL;

  guid = manette_device_get_guid (device);
  mapping_string = manette_mapping_manager_get_mapping (self->mapping_manager,
                                                        guid);
  mapping = manette_mapping_new (mapping_string, &error);
  if (G_UNLIKELY (error != NULL)) {
    g_debug ("%s", error->message);
    g_clear_error (&error);
  }

  if (mapping_string != NULL)
    g_free (mapping_string);

  manette_device_set_mapping (device, mapping);

  if (mapping != NULL)
    g_object_unref (mapping);
}

static void
add_device (ManetteMonitor *self,
            const gchar    *filename)
{
  ManetteDevice *device;
  GError *error = NULL;

  g_return_if_fail (self != NULL);
  g_return_if_fail (filename != NULL);

  if (g_hash_table_contains (self->devices, filename))
    return;

  device = manette_device_new (filename, &error);
  if (G_UNLIKELY (error != NULL)) {
    if (!g_error_matches (error, G_FILE_ERROR, G_FILE_ERROR_NXIO))
      g_debug ("Failed to open %s: %s", filename, error->message);

    g_error_free (error);
    error = NULL;

    return;
  }

  load_mapping (self, device);

  g_hash_table_insert (self->devices,
                       g_strdup (filename),
                       g_object_ref (device));
  g_signal_emit (self, signals[SIG_DEVICE_CONNECTED], 0, device);
  g_object_unref (device);
}

#ifdef GUDEV_ENABLED /* BACKEND GUDEV */

static void
remove_device (ManetteMonitor *self,
               const gchar    *filename)
{
  ManetteDevice *device;

  device = g_hash_table_lookup (self->devices, filename);
  if (device == NULL)
    return;

  g_object_ref (device);
  g_hash_table_remove (self->devices, filename);
  g_signal_emit_by_name (device, "disconnected");
  g_signal_emit (self, signals[SIG_DEVICE_DISCONNECTED], 0, device);
  g_object_unref (device);
}

static void
add_device_for_udev_device (ManetteMonitor *self,
                            GUdevDevice    *udev_device)
{
  const gchar *filename;

  g_return_if_fail (self != NULL);
  g_return_if_fail (udev_device != NULL);

  filename = g_udev_device_get_device_file (udev_device);
  add_device (self, filename);
}

static void
remove_device_for_udev_device (ManetteMonitor *self,
                               GUdevDevice    *udev_device)
{
  const gchar *filename;

  filename = g_udev_device_get_device_file (udev_device);
  remove_device (self, filename);
}

static gboolean
udev_device_property_is (GUdevDevice *udev_device,
                         const gchar *property,
                         const gchar *value)
{
  return g_udev_device_has_property (udev_device, property) &&
         (g_strcmp0 (g_udev_device_get_property (udev_device, property), value) == 0);
}

static gboolean
udev_device_is_manette (GUdevDevice *udev_device)
{
  g_return_val_if_fail (udev_device != NULL, FALSE);

  return udev_device_property_is (udev_device, "ID_INPUT_JOYSTICK", "1") ||
         udev_device_property_is (udev_device, ".INPUT_CLASS", "joystick");
}

static void
handle_udev_client_callback (GUdevClient *sender,
                             const gchar *action,
                             GUdevDevice *udev_device,
                             gpointer     data)
{
  ManetteMonitor *self;

  self = MANETTE_MONITOR (data);

  g_return_if_fail (self != NULL);
  g_return_if_fail (action != NULL);
  g_return_if_fail (udev_device != NULL);

  if (g_udev_device_get_device_file (udev_device) == NULL)
    return;

  if (!udev_device_is_manette (udev_device))
    return;

  if (g_strcmp0 (action, "add") == 0)
    add_device_for_udev_device (self, udev_device);
  else if (g_strcmp0 (action, "remove") == 0)
    remove_device_for_udev_device (self, udev_device);
}

static void
coldplug_devices (ManetteMonitor *self)
{
  GList *initial_devices_list;
  GList *device_it = NULL;
  GUdevDevice *udev_device = NULL;

  initial_devices_list = g_udev_client_query_by_subsystem (self->client,
                                                           "input");

  for (device_it = initial_devices_list;
       device_it != NULL;
       device_it = device_it->next) {
    udev_device = G_UDEV_DEVICE (device_it->data);
    if (g_udev_device_get_device_file (udev_device) == NULL)
      continue;

    if (!udev_device_is_manette (udev_device))
      continue;

    add_device_for_udev_device (self, udev_device);
  }

  g_list_free_full (initial_devices_list, g_object_unref);
}

#else /* BACKEND FALLBACK */

static void
coldplug_devices (ManetteMonitor *self)
{
  static const gchar *directory = "/dev/input";
  GDir *dir;
  const gchar *name = NULL;
  gchar *filename;
  GError *error = NULL;

  dir = g_dir_open (directory, (guint) 0, &error);
  if (G_UNLIKELY (error != NULL)) {
    g_debug ("%s", error->message);
    g_error_free (error);

    return;
  }

  while ((name = g_dir_read_name (dir)) != NULL) {
    filename = g_build_filename (directory, name, NULL);
    add_device (self, filename);
    g_free (filename);
  }

  g_dir_close (dir);
}

#endif /* BACKEND */

static void
on_mappings_changed (ManetteMappingManager *mapping_manager,
                     ManetteMonitor        *self)
{
  ManetteMonitorIter *iterator;
  ManetteDevice *device = NULL;

  g_return_if_fail (MANETTE_IS_MONITOR (self));

  iterator = manette_monitor_iterate (self);
  while (manette_monitor_iter_next (iterator, &device))
    load_mapping (self, device);
  manette_monitor_iter_free (iterator);
}

/* Public */

ManetteMonitor *
manette_monitor_new (void)
{
  ManetteMonitor *self = NULL;

  self = (ManetteMonitor*) g_object_new (MANETTE_TYPE_MONITOR, NULL);
  self->devices = g_hash_table_new_full (g_str_hash, g_str_equal,
                                         g_free, g_object_unref);
  self->mapping_manager = manette_mapping_manager_new ();

  g_signal_connect (self->mapping_manager,
                    "changed",
                    G_CALLBACK (on_mappings_changed),
                    self);

#ifdef GUDEV_ENABLED
  self->client = g_udev_client_new ((const gchar *[]) { "input", NULL });
  g_signal_connect_object (self->client,
                           "uevent",
                           (GCallback) handle_udev_client_callback,
                           self,
                           0);
#endif

  coldplug_devices (self);

  return self;
}

/* Type */

static void
finalize (GObject *object)
{
  ManetteMonitor *self = MANETTE_MONITOR (object);

#ifdef GUDEV_ENABLED
  if (self->client != NULL)
    g_object_unref (self->client);
#endif
  g_object_unref (self->mapping_manager);
  g_hash_table_unref (self->devices);

  G_OBJECT_CLASS (manette_monitor_parent_class)->finalize (object);
}

static void
manette_monitor_class_init (ManetteMonitorClass *klass)
{
  manette_monitor_parent_class = g_type_class_peek_parent (klass);
  G_OBJECT_CLASS (klass)->finalize = finalize;

  /**
   * ManetteMonitor::device-connected:
   * @self: a #ManetteMonitor
   * @device: a #ManetteDevice
   *
   * Emitted when a device is connected.
   */
  signals[SIG_DEVICE_CONNECTED] =
    g_signal_new ("device-connected",
                  MANETTE_TYPE_MONITOR,
                  G_SIGNAL_RUN_LAST,
                  0, NULL, NULL,
                  g_cclosure_marshal_VOID__OBJECT,
                  G_TYPE_NONE, 1,
                  MANETTE_TYPE_DEVICE);

  /**
   * ManetteMonitor::device-disconnected:
   * @self: a #ManetteMonitor
   * @device: a #ManetteDevice
   *
   * Emitted when a device is disconnected.
   */
  signals[SIG_DEVICE_DISCONNECTED] =
    g_signal_new ("device-disconnected",
                  MANETTE_TYPE_MONITOR,
                  G_SIGNAL_RUN_LAST,
                  0, NULL, NULL,
                  g_cclosure_marshal_VOID__OBJECT,
                  G_TYPE_NONE, 1,
                  MANETTE_TYPE_DEVICE);
}

/**
 * manette_monitor_iterate:
 * @self: a #ManetteMonitor
 *
 * Returns: (transfer full): a new #ManetteMonitorIter
 */
ManetteMonitorIter *
manette_monitor_iterate (ManetteMonitor *self)
{
  g_return_val_if_fail (MANETTE_IS_MONITOR (self), NULL);

  return manette_monitor_iter_new (self->devices);
}
