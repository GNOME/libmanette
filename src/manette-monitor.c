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

#include "config.h"

#include "manette-monitor.h"

#include <glib.h>
#include <glib-object.h>
#include <gio/gio.h>
#ifdef GUDEV_ENABLED
 #include <gudev/gudev.h>
#endif

#include "manette-backend-private.h"
#include "manette-device-private.h"
#include "manette-evdev-backend-private.h"
#include "manette-hid-backend-private.h"
#include "manette-mapping-manager-private.h"

#define DEV_DIRECTORY "/dev"
#define INPUT_DIRECTORY DEV_DIRECTORY "/input"

/**
 * ManetteMonitor:
 *
 * An object monitoring the availability of devices.
 *
 * See also: [class@Device].
 */

struct _ManetteMonitor {
  GObject parent_instance;

  GHashTable *devices;
  ManetteMappingManager *mapping_manager;
#ifdef GUDEV_ENABLED
  GUdevClient *client;
#endif
  GFileMonitor *dev_monitor;
  GFileMonitor *input_monitor;
  GHashTable *potential_devices;
};

G_DEFINE_FINAL_TYPE (ManetteMonitor, manette_monitor, G_TYPE_OBJECT)

enum {
  SIG_DEVICE_CONNECTED,
  SIG_DEVICE_DISCONNECTED,
  N_SIGNALS,
};

static guint signals[N_SIGNALS];

/* Private */

#if GUDEV_ENABLED
static inline gboolean
is_flatpak (void)
{
  return g_file_test ("/.flatpak-info", G_FILE_TEST_EXISTS);
}
#endif

static void
load_mapping (ManetteMonitor *self,
              ManetteDevice  *device)
{
  const char *guid;
  g_autofree char *mapping_string = NULL;
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
            const char     *filename,
            gboolean        is_hid)
{
  g_autoptr (ManetteDevice) device = NULL;
  g_autoptr (ManetteBackend) backend = NULL;
  g_autoptr (GError) error = NULL;

  g_assert (self != NULL);
  g_assert (filename != NULL);

  if (g_hash_table_contains (self->devices, filename))
    return;

  if (is_hid)
    backend = manette_hid_backend_new (filename);
  else
    backend = manette_evdev_backend_new (filename);

  if (!manette_backend_initialize (backend))
    return;

  device = manette_device_new (g_steal_pointer (&backend), &error);
  if (G_UNLIKELY (error != NULL)) {
    if (!g_error_matches (error, G_FILE_ERROR, G_FILE_ERROR_NXIO))
      g_debug ("Failed to open %s: %s", filename, error->message);

    return;
  }

  if (manette_device_supports_mapping (device))
    load_mapping (self, device);

  g_hash_table_insert (self->devices,
                       g_strdup (filename),
                       g_object_ref (device));
  g_signal_emit (self, signals[SIG_DEVICE_CONNECTED], 0, device);
}

static void
remove_device (ManetteMonitor *self,
               const char     *filename)
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

#ifdef GUDEV_ENABLED

static void
add_device_for_udev_device (ManetteMonitor *self,
                            GUdevDevice    *udev_device)
{
  const char *filename, *subsystem;

  g_assert (self != NULL);
  g_assert (udev_device != NULL);

  filename = g_udev_device_get_device_file (udev_device);
  subsystem = g_udev_device_get_subsystem (udev_device);

  add_device (self, filename, !g_strcmp0 (subsystem, "hidraw"));
}

static void
remove_device_for_udev_device (ManetteMonitor *self,
                               GUdevDevice    *udev_device)
{
  const char *filename;

  filename = g_udev_device_get_device_file (udev_device);
  remove_device (self, filename);
}

static gboolean
udev_device_property_is (GUdevDevice *udev_device,
                         const char  *property,
                         const char  *value)
{
  g_assert (property != NULL);
  g_assert (value != NULL);

  return g_strcmp0 (g_udev_device_get_property (udev_device, property), value) == 0;
}

static gboolean
udev_device_is_manette (GUdevDevice *udev_device)
{
  g_assert (udev_device != NULL);

  return !g_strcmp0 (g_udev_device_get_subsystem (udev_device), "hidraw") ||
         udev_device_property_is (udev_device, "ID_INPUT_JOYSTICK", "1") ||
         udev_device_property_is (udev_device, ".INPUT_CLASS", "joystick");
}

static void
udev_client_uevent_cb (GUdevClient    *sender,
                       const char     *action,
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
coldplug_gudev_devices_for_subsystem (ManetteMonitor *self,
                                      const char     *subsystem)
{
  GList *initial_devices_list;
  GList *device_it = NULL;
  GUdevDevice *udev_device = NULL;

  initial_devices_list = g_udev_client_query_by_subsystem (self->client, subsystem);

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
coldplug_gudev_devices (ManetteMonitor *self)
{
  coldplug_gudev_devices_for_subsystem (self, "input");
  coldplug_gudev_devices_for_subsystem (self, "hidraw");
}

static void
init_gudev_backend (ManetteMonitor *self)
{
  self->client = g_udev_client_new ((const char *[]) { "input", "hidraw", NULL });
  g_signal_connect_object (self->client,
                           "uevent",
                           (GCallback) udev_client_uevent_cb,
                           self,
                           0);
}

#endif

/* This eliminates all other files that can be in /dev/input, like js*, mouse*,
 * by-id/, etc.
 * There isn't really any need to check if there's only digits after "event", as
 * it would induce a bit of performance loss for this hypothetical case…
 */
static gboolean
is_evdev_device (GFile *file)
{
  const char *event_file_path = g_file_peek_path (file);

  return g_str_has_prefix (event_file_path, INPUT_DIRECTORY "/event");
}

static gboolean
is_hid_device (GFile *file)
{
  const char *event_file_path = g_file_peek_path (file);

  return g_str_has_prefix (event_file_path, DEV_DIRECTORY "/hidraw");
}

static inline gboolean
is_eligible_device (GFile *file)
{
  return is_evdev_device (file) || is_hid_device (file);
}

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
  g_autofree char *path = g_file_get_path (file);

  if (is_accessible (file)) {
    add_device (self, path, is_hid_device (file));

    return;
  }

  g_hash_table_add (self->potential_devices, g_steal_pointer (&path));
}

static void
file_attribute_changed (ManetteMonitor *self,
                        GFile          *file)
{
  g_autofree char *path = g_file_get_path (file);

  if (!g_hash_table_contains (self->potential_devices, path))
    return;

  if (!is_accessible (file))
    return;

  add_device (self, path, is_hid_device (file));

  g_hash_table_remove (self->potential_devices, path);
}

static void
file_deleted (ManetteMonitor *self,
              GFile          *file)
{
  g_autofree char *path = g_file_get_path (file);

  remove_device (self, path);
}

static void
file_monitor_changed_cb (GFileMonitor      *monitor,
                         GFile             *file,
                         GFile             *other_file,
                         GFileMonitorEvent  event_type,
                         ManetteMonitor    *self)
{
  if (!is_eligible_device (file))
    return;

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
coldplug_files_from_dir (ManetteMonitor *self,
                         const char     *path)
{
  g_autoptr (GDir) dir = NULL;
  const char *name = NULL;
  g_autoptr (GError) error = NULL;

  dir = g_dir_open (path, (guint) 0, &error);
  if (G_UNLIKELY (error != NULL)) {
    g_debug ("%s", error->message);

    return;
  }

  while ((name = g_dir_read_name (dir)) != NULL) {
    g_autofree char *filename = NULL;
    g_autoptr (GFile) file = NULL;

    filename = g_build_filename (path, name, NULL);
    file = g_file_new_for_path (filename);
    if (is_eligible_device (file) && is_accessible (file))
      add_device (self, filename, is_hid_device (file));
  }
}

static void
coldplug_file_devices (ManetteMonitor *self)
{
  coldplug_files_from_dir (self, DEV_DIRECTORY);
  coldplug_files_from_dir (self, INPUT_DIRECTORY);
}

static void
init_file_backend (ManetteMonitor *self)
{
  g_autoptr (GFile) dev_dir = g_file_new_for_path (DEV_DIRECTORY);
  g_autoptr (GFile) input_dir = g_file_new_for_path (INPUT_DIRECTORY);
  g_autoptr (GError) error = NULL;

  self->dev_monitor = g_file_monitor_directory (dev_dir,
                                                G_FILE_MONITOR_NONE,
                                                NULL,
                                                &error);

  if (G_UNLIKELY (error != NULL))
    g_debug ("Couldn't monitor %s: %s", DEV_DIRECTORY, error->message);
  else
    g_signal_connect_object (self->dev_monitor,
                             "changed",
                             (GCallback) file_monitor_changed_cb,
                             self,
                             0);

  g_clear_error (&error);

  self->input_monitor = g_file_monitor_directory (input_dir,
                                                  G_FILE_MONITOR_NONE,
                                                  NULL,
                                                  &error);

  if (G_UNLIKELY (error != NULL))
    g_debug ("Couldn't monitor %s: %s", INPUT_DIRECTORY, error->message);
  else
    g_signal_connect_object (self->input_monitor,
                             "changed",
                             (GCallback) file_monitor_changed_cb,
                             self,
                             0);

  self->potential_devices = g_hash_table_new_full (g_str_hash, g_str_equal,
                                                   g_free, NULL);
}

static void
mappings_changed_cb (ManetteMappingManager *mapping_manager,
                     ManetteMonitor        *self)
{
  GHashTableIter iter;
  ManetteDevice *device;

  g_hash_table_iter_init (&iter, self->devices);
  while (g_hash_table_iter_next (&iter, NULL, (gpointer *) &device)) {
    if (!manette_device_supports_mapping (device))
      continue;

    load_mapping (self, device);
  }
}

/* Public */

/**
 * manette_monitor_new:
 *
 * Creates a new `ManetteMonitor`.
 *
 * Returns: (transfer full): a new `ManetteMonitor`
 */
ManetteMonitor *
manette_monitor_new (void)
{
  return g_object_new (MANETTE_TYPE_MONITOR, NULL);
}

/* Type */

static void
manette_monitor_init (ManetteMonitor *self)
{
  gboolean use_file_backend = FALSE;

  self->devices = g_hash_table_new_full (g_str_hash, g_str_equal,
                                         g_free, g_object_unref);
  self->mapping_manager = manette_mapping_manager_new ();

  g_signal_connect_object (self->mapping_manager,
                           "changed",
                           G_CALLBACK (mappings_changed_cb),
                           self, 0);

#if GUDEV_ENABLED
  use_file_backend = is_flatpak ();
#else
  use_file_backend = TRUE;
#endif

  if (use_file_backend) {
    init_file_backend (self);
    coldplug_file_devices (self);
  } else {
#if GUDEV_ENABLED
    init_gudev_backend (self);
    coldplug_gudev_devices (self);
#endif
  }
}

static void
manette_monitor_finalize (GObject *object)
{
  ManetteMonitor *self = MANETTE_MONITOR (object);

#ifdef GUDEV_ENABLED
  g_clear_object (&self->client);
#endif

  g_clear_object (&self->dev_monitor);
  g_clear_object (&self->input_monitor);
  g_clear_pointer (&self->potential_devices, g_hash_table_unref);

  g_clear_object (&self->mapping_manager);
  g_clear_pointer (&self->devices, g_hash_table_unref);

  G_OBJECT_CLASS (manette_monitor_parent_class)->finalize (object);
}

static void
manette_monitor_class_init (ManetteMonitorClass *klass)
{
  manette_monitor_parent_class = g_type_class_peek_parent (klass);
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = manette_monitor_finalize;

  /**
   * ManetteMonitor::device-connected:
   * @self: a monitor
   * @device: a device
   *
   * Emitted when @device is connected.
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
   * @self: a monitor
   * @device: a device
   *
   * Emitted when @device is disconnected.
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
 * manette_monitor_list_devices:
 * @self: a monitor
 * @n_devices: (out): return location for the length of the array
 *
 * Lists the currently connected devices.
 *
 * Returns: (transfer container) (array length=n_devices): the list of devices
 */
ManetteDevice **
manette_monitor_list_devices (ManetteMonitor *self,
                              gsize          *n_devices)
{
  GPtrArray *devices;

  g_return_val_if_fail (MANETTE_IS_MONITOR (self), NULL);
  g_return_val_if_fail (n_devices != NULL, NULL);

  devices = g_hash_table_get_values_as_ptr_array (self->devices);

  return (ManetteDevice **) g_ptr_array_steal (devices, n_devices);
}
