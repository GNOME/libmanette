/* manette-mapping-manager.c
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

#include "manette-mapping-manager-private.h"

#include <glib.h>
#include <glib-object.h>
#include <stdlib.h>
#include <string.h>
#include <gio/gio.h>

struct _ManetteMappingManager {
  GObject parent_instance;

  GHashTable *names;
  GHashTable *default_mappings;
  GHashTable *user_mappings;
  gchar *user_mappings_uri;
  GFileMonitor *user_mappings_monitor;
};

G_DEFINE_TYPE (ManetteMappingManager, manette_mapping_manager, G_TYPE_OBJECT);

enum {
  SIG_CHANGED,
  N_SIGNALS,
};

static guint signals[N_SIGNALS];

#define CONFIG_DIR "libmanette"
#define MAPPING_CONFIG_FILE "gamecontrollerdb"
#define MAPPING_RESOURCE_URI "resource:///org/gnome/Manette/gamecontrollerdb"

/* Private */

static void
add_mapping (ManetteMappingManager *self,
             const gchar           *mapping_string,
             GHashTable            *mappings)
{
  const gchar *platform;
  gchar **split;

  g_return_if_fail (MANETTE_IS_MAPPING_MANAGER (self));
  g_return_if_fail (mapping_string != NULL);

  if (mapping_string[0] == '\0' || mapping_string[0] == '#')
    return;

  platform = g_strstr_len (mapping_string, -1, "platform");
  if (platform != NULL && !g_str_has_prefix (platform, "platform:Linux"))
    return;

  split = g_strsplit (mapping_string, ",", 3);
  g_hash_table_insert (self->names,
                       g_strdup (split[0]),
                       g_strdup (split[1]));
  g_hash_table_insert (mappings,
                       g_strdup (split[0]),
                       g_strdup (mapping_string));
  g_strfreev (split);
}

static void
add_from_input_stream (ManetteMappingManager  *self,
                       GInputStream           *input_stream,
                       GHashTable             *mappings,
                       GError                **error)
{
  GDataInputStream *data_stream;
  gchar *mapping_string;
  GError *inner_error = NULL;

  g_return_if_fail (MANETTE_IS_MAPPING_MANAGER (self));
  g_return_if_fail (input_stream != NULL);

  data_stream = g_data_input_stream_new (input_stream);
  while (TRUE) {
    mapping_string = g_data_input_stream_read_line (data_stream,
                                                    NULL, NULL,
                                                    &inner_error);
    if (G_UNLIKELY (inner_error != NULL)) {
      g_assert (mapping_string == NULL);
      g_propagate_error (error, inner_error);
      g_object_unref (data_stream);

      return;
    }

    if (mapping_string == NULL)
      break;

    add_mapping (self, mapping_string, mappings);
    g_free (mapping_string);
  }
  g_object_unref (data_stream);
}

static void
add_from_file_uri (ManetteMappingManager  *self,
                   const gchar            *file_uri,
                   GHashTable             *mappings,
                   GError                **error)
{
  GFile *file;
  GFileInputStream *stream;
  GError *inner_error = NULL;

  g_return_if_fail (MANETTE_IS_MAPPING_MANAGER (self));
  g_return_if_fail (file_uri != NULL);

  file = g_file_new_for_uri (file_uri);
  stream = g_file_read (file, NULL, &inner_error);
  if (G_UNLIKELY (inner_error != NULL)) {
    g_propagate_error (error, inner_error);
    g_object_unref (file);

    return;
  }

  add_from_input_stream (self, G_INPUT_STREAM (stream), mappings, &inner_error);
  if (G_UNLIKELY (inner_error != NULL)) {
    g_propagate_error (error, inner_error);
    g_object_unref (stream);
    g_object_unref (file);

    return;
  }

  g_object_unref (stream);
  g_object_unref (file);
}

static void
save_user_mappings (ManetteMappingManager  *self,
                    GError                **error)
{
  GHashTableIter iter;
  gpointer key, value;
  gchar *guid;
  const gchar *name;
  gchar *sdl_string;
  gchar *mapping_string;

  GFile *file;
  GFile *directory;
  GFileOutputStream *stream;
  GDataOutputStream *data_stream;
  GError *inner_error = NULL;

  g_return_if_fail (MANETTE_IS_MAPPING_MANAGER (self));

  file = g_file_new_for_uri (self->user_mappings_uri);
  directory = g_file_get_parent (file);

  if (!g_file_query_exists (directory, NULL)) {
    g_file_make_directory_with_parents (directory, NULL, &inner_error);
    if (G_UNLIKELY (inner_error != NULL)) {
      g_propagate_error (error, inner_error);
      g_object_unref (file);
      g_object_unref (directory);

      return;
    }
  }

  g_object_unref (directory);

  stream = g_file_replace (file, NULL, FALSE, G_FILE_CREATE_NONE, NULL, &inner_error);
  if (G_UNLIKELY (inner_error != NULL)) {
    g_propagate_error (error, inner_error);
    g_object_unref (file);

    return;
  }
  data_stream = g_data_output_stream_new (G_OUTPUT_STREAM (stream));

  g_hash_table_iter_init (&iter, self->user_mappings);
  while (g_hash_table_iter_next (&iter, &key, &value)) {
    guid = (gchar *) key;
    name = g_hash_table_lookup (self->names, guid);
    sdl_string = (gchar *) value;

    mapping_string = g_strdup_printf ("%s,%s,%s\n", guid, name, sdl_string);

    g_data_output_stream_put_string (data_stream, mapping_string, NULL, &inner_error);
    if (G_UNLIKELY (inner_error != NULL)) {
      g_propagate_error (error, inner_error);
      g_free (mapping_string);
      g_object_unref (file);
      g_object_unref (stream);
      g_object_unref (data_stream);

      return;
    }

    g_free (mapping_string);
  }

  g_object_unref (file);
  g_object_unref (stream);
  g_object_unref (data_stream);
}

static void
user_mappings_changed_cb (GFileMonitor          *monitor,
                          GFile                 *file,
                          GFile                 *other_file,
                          GFileMonitorEvent      event_type,
                          ManetteMappingManager *self)
{
  GError *inner_error = NULL;

  g_return_if_fail (MANETTE_IS_MAPPING_MANAGER (self));

  g_hash_table_remove_all (self->user_mappings);

  if (!g_file_query_exists (file, NULL)) {
    g_signal_emit (self, signals[SIG_CHANGED], 0);

    return;
  }

  add_from_file_uri (self, self->user_mappings_uri, self->user_mappings, &inner_error);
  if (G_UNLIKELY (inner_error != NULL)) {
    g_debug ("ManetteMappingManager: Can’t add mappings from %s: %s",
             self->user_mappings_uri,
             inner_error->message);
    g_clear_error (&inner_error);
  }

  g_signal_emit (self, signals[SIG_CHANGED], 0);
}

/* Public */

ManetteMappingManager *
manette_mapping_manager_new (void)
{
  ManetteMappingManager *self = NULL;
  gchar *path;
  GFile *user_mappings_file;
  GError *inner_error = NULL;

  self = (ManetteMappingManager*) g_object_new (MANETTE_TYPE_MAPPING_MANAGER, NULL);

  if (self->names == NULL)
    self->names = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, g_free);

  if (self->default_mappings == NULL)
    self->default_mappings = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, g_free);
  if (self->user_mappings == NULL)
    self->user_mappings = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, g_free);

  add_from_file_uri (self, MAPPING_RESOURCE_URI, self->default_mappings, &inner_error);
  if (G_UNLIKELY (inner_error != NULL)) {
    g_critical ("ManetteMappingManager: Can’t add mappings from %s: %s",
                MAPPING_RESOURCE_URI,
                inner_error->message);
    g_clear_error (&inner_error);
  }

  path = g_build_filename (g_get_user_config_dir (), CONFIG_DIR, MAPPING_CONFIG_FILE, NULL);

  self->user_mappings_uri = g_filename_to_uri (path, NULL, &inner_error);
  if (G_UNLIKELY (inner_error != NULL)) {
    g_debug ("ManetteMappingManager: Can't build path for user config: %s",
             inner_error->message);
    g_free (path);
    g_clear_error (&inner_error);

    return self;
  }

  g_free (path);

  user_mappings_file = g_file_new_for_uri (self->user_mappings_uri);
  self->user_mappings_monitor = g_file_monitor_file (user_mappings_file,
                                                     G_FILE_MONITOR_NONE,
                                                     NULL,
                                                     &inner_error);
  if (G_UNLIKELY (inner_error != NULL)) {
    g_debug ("ManetteMappingManager: Can’t monitor mappings from %s: %s",
             self->user_mappings_uri,
             inner_error->message);
    g_clear_error (&inner_error);
  }

  g_object_unref (user_mappings_file);

  g_signal_connect (self->user_mappings_monitor,
                    "changed",
                    G_CALLBACK (user_mappings_changed_cb),
                    self);

  add_from_file_uri (self, self->user_mappings_uri, self->user_mappings, &inner_error);
  if (G_UNLIKELY (inner_error != NULL)) {
    g_debug ("ManetteMappingManager: Can’t add mappings from %s: %s",
             self->user_mappings_uri,
             inner_error->message);
    g_clear_error (&inner_error);
  }

  return self;
}

gboolean
manette_mapping_manager_has_user_mapping (ManetteMappingManager *self,
                                          const gchar           *guid)
{
  g_return_val_if_fail (MANETTE_IS_MAPPING_MANAGER (self), FALSE);
  g_return_val_if_fail (guid != NULL, FALSE);

  return g_hash_table_contains (self->user_mappings, guid);
}

gchar *
manette_mapping_manager_get_default_mapping (ManetteMappingManager *self,
                                             const gchar           *guid)
{
  const gchar *mapping;

  g_return_val_if_fail (MANETTE_IS_MAPPING_MANAGER (self), NULL);
  g_return_val_if_fail (guid != NULL, NULL);

  mapping = g_hash_table_lookup (self->default_mappings, guid);

  return g_strdup (mapping);
}

gchar *
manette_mapping_manager_get_user_mapping (ManetteMappingManager *self,
                                          const gchar           *guid)
{
  const gchar *mapping;

  g_return_val_if_fail (MANETTE_IS_MAPPING_MANAGER (self), NULL);
  g_return_val_if_fail (guid != NULL, NULL);

  mapping = g_hash_table_lookup (self->user_mappings, guid);

  return g_strdup (mapping);
}

gchar *
manette_mapping_manager_get_mapping (ManetteMappingManager *self,
                                     const gchar           *guid)
{
  gchar *mapping;

  g_return_val_if_fail (MANETTE_IS_MAPPING_MANAGER (self), NULL);
  g_return_val_if_fail (guid != NULL, NULL);

  mapping = manette_mapping_manager_get_user_mapping (self, guid);
  if (mapping == NULL)
    mapping = manette_mapping_manager_get_default_mapping (self, guid);

  return mapping;
}

void
manette_mapping_manager_save_mapping (ManetteMappingManager *self,
                                      const gchar           *guid,
                                      const gchar           *name,
                                      const gchar           *mapping)
{
  GError *inner_error = NULL;

  g_return_if_fail (MANETTE_IS_MAPPING_MANAGER (self));
  g_return_if_fail (guid != NULL);
  g_return_if_fail (name != NULL);
  g_return_if_fail (mapping != NULL);

  g_hash_table_insert (self->user_mappings, g_strdup (guid), g_strdup (mapping));
  g_hash_table_insert (self->names, g_strdup (guid), g_strdup (name));

  save_user_mappings (self, &inner_error);
  if (G_UNLIKELY (inner_error != NULL)) {
    g_critical ("ManetteMappingManager: Can’t save user mappings: %s", inner_error->message);
    g_clear_error (&inner_error);
  }
}

void
manette_mapping_manager_delete_mapping (ManetteMappingManager *self,
                                        const gchar           *guid)
{
  GError *inner_error = NULL;

  g_return_if_fail (MANETTE_IS_MAPPING_MANAGER (self));
  g_return_if_fail (guid != NULL);

  g_hash_table_remove (self->user_mappings, guid);
  g_hash_table_remove (self->names, guid);

  save_user_mappings (self, &inner_error);
  if (G_UNLIKELY (inner_error != NULL)) {
    g_critical ("ManetteMappingManager: Can’t save user mappings: %s", inner_error->message);
    g_clear_error (&inner_error);
  }
}

/* Type */

static void
finalize (GObject *object)
{
  ManetteMappingManager *self = MANETTE_MAPPING_MANAGER (object);

  g_hash_table_unref (self->names);
  g_hash_table_unref (self->default_mappings);
  g_hash_table_unref (self->user_mappings);
  g_free (self->user_mappings_uri);
  g_clear_object (&self->user_mappings_monitor);

  G_OBJECT_CLASS (manette_mapping_manager_parent_class)->finalize (object);
}

static void
manette_mapping_manager_class_init (ManetteMappingManagerClass *klass)
{
  manette_mapping_manager_parent_class = g_type_class_peek_parent (klass);

  /**
   * ManetteMappingManager::changed:
   * @self: a #ManetteMappingManager
   *
   * Emitted when some mappings changed.
   */
  signals[SIG_CHANGED] =
    g_signal_new ("changed",
                  MANETTE_TYPE_MAPPING_MANAGER,
                  G_SIGNAL_RUN_LAST,
                  0, NULL, NULL,
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);

  G_OBJECT_CLASS (klass)->finalize = finalize;
}

static void
manette_mapping_manager_init (ManetteMappingManager *self)
{
}
