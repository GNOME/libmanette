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
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/**
 * SECTION:manette-monitor
 * @short_description: An object monitoring the availability of devices
 * @title: ManetteMonitor
 * @See_also: #ManetteDevice
 */

#include "manette-monitor.h"

#include <glib.h>
#include <glib-object.h>
#ifdef GUDEV_ENABLED
 #include <gudev/gudev.h>
#else
 #include <gio/gio.h>
#endif
#include "manette-device-private.h"
#include "manette-mapping-manager-private.h"
#include "manette-monitor-iter-private.h"

#ifndef GUDEV_ENABLED
#define INPUT_DIRECTORY "/dev/input"
#endif

struct _ManetteMonitor {
  GObject parent_instance;

  GHashTable *devices;
  ManetteMappingManager *mapping_manager;
#ifdef GUDEV_ENABLED
  GUdevClient *client;
#else
  GFileMonitor *monitor;
  GHashTable *potential_devices;
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
  g_autofree gchar *mapping_string = NULL;
  g_autoptr (ManetteMapping) mapping = NULL;
  g_autoptr (GError) error = NULL;

  guid = manette_device_get_guid (device);
  mapping_string = manette_mapping_manager_get_mapping (self->mapping_manager,
                                                        guid);
  mapping = manette_mapping_new (mapping_string, &error);
  if (G_UNLIKELY (error != NULL)) {
    g_debug ("%s", error->message);

    return;
  }

  manette_device_set_mapping (device, mapping);
}

static void
add_device (ManetteMonitor *self,
            const gchar    *filename)
{
  g_autoptr (ManetteDevice) device = NULL;
  g_autoptr (GError) error = NULL;

  g_assert (self != NULL);
  g_assert (filename != NULL);

  if (g_hash_table_contains (self->devices, filename))
    return;

  device = manette_device_new (filename, &error);
  if (G_UNLIKELY (error != NULL)) {
    if (!g_error_matches (error, G_FILE_ERROR, G_FILE_ERROR_NXIO))
      g_debug ("Failed to open %s: %s", filename, error->message);

    return;
  }

  load_mapping (self, device);

  g_hash_table_insert (self->devices,
                       g_strdup (filename),
                       g_object_ref (device));
  g_signal_emit (self, signals[SIG_DEVICE_CONNECTED], 0, device);
}

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

#ifdef GUDEV_ENABLED /* BACKEND GUDEV */

static void
add_device_for_udev_device (ManetteMonitor *self,
                            GUdevDevice    *udev_device)
{
  const gchar *filename;

  g_assert (self != NULL);
  g_assert (udev_device != NULL);

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
  g_assert (property != NULL);
  g_assert (value != NULL);

  return g_strcmp0 (g_udev_device_get_property (udev_device, property), value) == 0;
}

static gboolean
udev_device_is_manette (GUdevDevice *udev_device)
{
  g_assert (udev_device != NULL);

  return udev_device_property_is (udev_device, "ID_INPUT_JOYSTICK", "1") ||
         udev_device_property_is (udev_device, ".INPUT_CLASS", "joystick");
}

static void
udev_client_uevent_cb (GUdevClient    *sender,
                       const gchar    *action,
                       GUdevDevice    *udev_device,
                       ManetteMonitor *self)
{
  g_assert (action != NULL);
  g_assert (udev_device != NULL);
  g_assert (self != NULL);

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

static void
init_backend (ManetteMonitor *self)
{
  self->client = g_udev_client_new ((const gchar *[]) { "input", NULL });
  g_signal_connect_object (self->client,
                           "uevent",
                           (GCallback) udev_client_uevent_cb,
                           self,
                           0);
}

#else /* BACKEND FALLBACK */

static gboolean
is_accessible (GFile *file)
{
  g_autoptr (GFileInfo) info = NULL;
  g_autoptr (GError) error = NULL;

  info = g_file_query_info (file,
                            G_FILE_ATTRIBUTE_ACCESS_CAN_READ "," G_FILE_ATTRIBUTE_ACCESS_CAN_WRITE,
                            G_FILE_QUERY_INFO_NONE,
                            NULL,
                            &error);

  if (G_UNLIKELY (error != NULL))
    return FALSE;

  return g_file_info_get_attribute_boolean (info, G_FILE_ATTRIBUTE_ACCESS_CAN_READ) &&
         g_file_info_get_attribute_boolean (info, G_FILE_ATTRIBUTE_ACCESS_CAN_WRITE);
}

static void
file_created (ManetteMonitor *self,
              GFile          *file)
{
  g_autofree gchar *path = g_file_get_path (file);

  if (is_accessible (file)) {
    add_device (self, path);

    return;
  }

  g_hash_table_add (self->potential_devices, g_steal_pointer (&path));
}

static void
file_attribute_changed (ManetteMonitor *self,
                        GFile          *file)
{
  g_autofree gchar *path = g_file_get_path (file);

  if (!g_hash_table_contains (self->potential_devices, path))
    return;

  if (!is_accessible (file))
    return;

  add_device (self, path);

  g_hash_table_remove (self->potential_devices, path);
}

static void
file_deleted (ManetteMonitor *self,
              GFile          *file)
{
  g_autofree gchar *path = g_file_get_path (file);

  remove_device (self, path);
}

static void
file_monitor_changed_cb (GFileMonitor      *monitor,
                         GFile             *file,
                         GFile             *other_file,
                         GFileMonitorEvent  event_type,
                         ManetteMonitor    *self)
{
  switch (event_type) {
  case G_FILE_MONITOR_EVENT_CREATED:
    file_created (self, file);
    break;

  case G_FILE_MONITOR_EVENT_ATTRIBUTE_CHANGED:
    file_attribute_changed (self, file);
    break;

  case G_FILE_MONITOR_EVENT_DELETED:
    file_deleted (self, file);
    break;

  default:
    return;
  }
}

static void
coldplug_devices (ManetteMonitor *self)
{
  g_autoptr (GDir) dir = NULL;
  const gchar *name = NULL;
  g_autoptr (GError) error = NULL;

  dir = g_dir_open (INPUT_DIRECTORY, (guint) 0, &error);
  if (G_UNLIKELY (error != NULL)) {
    g_debug ("%s", error->message);

    return;
  }

  while ((name = g_dir_read_name (dir)) != NULL) {
    g_autofree gchar *filename = NULL;
    filename = g_build_filename (INPUT_DIRECTORY, name, NULL);
    add_device (self, filename);
  }
}

static void
init_backend (ManetteMonitor *self)
{
  g_autoptr (GFile) file = g_file_new_for_path (INPUT_DIRECTORY);
  g_autoptr (GError) error = NULL;

  self->monitor = g_file_monitor_directory (file,
                                            G_FILE_MONITOR_NONE,
                                            NULL,
                                            &error);

  if (G_UNLIKELY (error != NULL))
    g_debug ("Couldn't monitor %s: %s", INPUT_DIRECTORY, error->message);
  else
    g_signal_connect_object (self->monitor,
                             "changed",
                             (GCallback) file_monitor_changed_cb,
                             self,
                             0);

  self->potential_devices = g_hash_table_new_full (g_str_hash, g_str_equal,
                                                   g_free, NULL);
}

#endif /* BACKEND */

static void
mappings_changed_cb (ManetteMappingManager *mapping_manager,
                     ManetteMonitor        *self)
{
  g_autoptr (ManetteMonitorIter) iterator = NULL;
  ManetteDevice *device = NULL;

  iterator = manette_monitor_iterate (self);
  while (manette_monitor_iter_next (iterator, &device))
    load_mapping (self, device);
}

/* Public */

/**
 * manette_monitor_new:
 *
 * Creates a new #ManetteMonitor object.
 *
 * Returns: (transfer full): a new #ManetteMonitor
 */
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
                    G_CALLBACK (mappings_changed_cb),
                    self);

  init_backend (self);
  coldplug_devices (self);

  return self;
}

/* Type */

static void
manette_monitor_finalize (GObject *object)
{
  ManetteMonitor *self = MANETTE_MONITOR (object);

#ifdef GUDEV_ENABLED
  g_clear_object (&self->client);
#else
  g_clear_object (&self->monitor);
  g_clear_pointer (&self->potential_devices, g_hash_table_unref);
#endif

  g_clear_object (&self->mapping_manager);
  g_clear_pointer (&self->devices, g_hash_table_unref);

  G_OBJECT_CLASS (manette_monitor_parent_class)->finalize (object);
}

static void
manette_monitor_class_init (ManetteMonitorClass *klass)
{
  manette_monitor_parent_class = g_type_class_peek_parent (klass);
  G_OBJECT_CLASS (klass)->finalize = manette_monitor_finalize;

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
 * Creates a new #ManetteMonitorIter iterating on @self.
 *
 * Returns: (transfer full): a new #ManetteMonitorIter iterating on @self
 */
ManetteMonitorIter *
manette_monitor_iterate (ManetteMonitor *self)
{
  g_return_val_if_fail (MANETTE_IS_MONITOR (self), NULL);

  return manette_monitor_iter_new (self->devices);
}
