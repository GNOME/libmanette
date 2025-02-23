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

#include "config.h"

#include "manette-mapping-manager-private.h"

#include <glib.h>
#include <glib-object.h>
#include <stdlib.h>
#include <string.h>
#include <gio/gio.h>

struct _ManetteMappingManager {
  GObject parent_instance;

  /* Associates the GUID with the device's name */
  GHashTable *names;
  /* Those two associates a GUID with its full corresponding SDL mapping string */
  GHashTable *default_mappings;
  GHashTable *user_mappings;

  char *user_mappings_uri;
  GFileMonitor *user_mappings_monitor;
};

G_DEFINE_FINAL_TYPE (ManetteMappingManager, manette_mapping_manager, G_TYPE_OBJECT);

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
             const char            *mapping_string,
             GHashTable            *mappings)
{
  const char *platform;
  const char *hint;
  g_auto (GStrv) split = NULL;

  g_assert (mapping_string != NULL);

  if (mapping_string[0] == '\0' || mapping_string[0] == '#')
    return;

  platform = g_strstr_len (mapping_string, -1, "platform");
  if (platform != NULL && !g_str_has_prefix (platform, "platform:Linux")) {
    g_debug ("Mappings for other platforms than Linux aren´t supported: ignoring mapping `%s`", mapping_string);
    return;
  }

  hint = g_strstr_len (mapping_string, -1, "hint");
  if (hint != NULL && !g_str_has_prefix (hint, "hint:SDL_GAMECONTROLLER_USE_BUTTON_LABELS:=1")) {
    g_debug ("Mappings reporting face buttons by label instead of position aren´t supported: ignoring mapping `%s`", mapping_string);
    return;
  }

  /* GUID | device name | the rest of the mapping string */
  split = g_strsplit (mapping_string, ",", 3);
  g_hash_table_insert (self->names,
                       g_strdup (split[0]),
                       g_strdup (split[1]));
  g_hash_table_insert (mappings,
                       g_strdup (split[0]),
                       g_strdup (mapping_string));
}

static void
add_from_input_stream (ManetteMappingManager  *self,
                       GInputStream           *input_stream,
                       GHashTable             *mappings,
                       GError                **error)
{
  g_autoptr (GDataInputStream) data_stream = NULL;
  GError *inner_error = NULL;

  g_assert (input_stream != NULL);

  data_stream = g_data_input_stream_new (input_stream);
  while (TRUE) {
    g_autofree char *mapping_string = g_data_input_stream_read_line (data_stream,
                                                                     NULL, NULL,
                                                                     &inner_error);
    if (G_UNLIKELY (inner_error != NULL)) {
      g_assert (mapping_string == NULL);
      g_propagate_error (error, inner_error);

      return;
    }

    if (mapping_string == NULL)
      break;

    g_strstrip (mapping_string);

    add_mapping (self, mapping_string, mappings);
  }
}

static void
add_from_file_uri (ManetteMappingManager  *self,
                   const char             *file_uri,
                   GHashTable             *mappings,
                   GError                **error)
{
  g_autoptr (GFile) file = NULL;
  g_autoptr (GFileInputStream) stream = NULL;
  GError *inner_error = NULL;

  g_assert (file_uri != NULL);

  file = g_file_new_for_uri (file_uri);
  stream = g_file_read (file, NULL, &inner_error);
  if (G_UNLIKELY (inner_error != NULL)) {
    g_propagate_error (error, inner_error);

    return;
  }

  add_from_input_stream (self, G_INPUT_STREAM (stream), mappings, &inner_error);
  if (G_UNLIKELY (inner_error != NULL)) {
    g_propagate_error (error, inner_error);

    return;
  }
}

static void
save_user_mappings (ManetteMappingManager  *self,
                    GError                **error)
{
  GHashTableIter iter;
  char *guid;
  char *sdl_string;
  const char *name;
  g_autofree char *mapping_string = NULL;

  g_autoptr (GFile) file = NULL;
  g_autoptr (GFile) directory = NULL;
  g_autoptr (GFileOutputStream) stream = NULL;
  g_autoptr (GDataOutputStream) data_stream = NULL;
  GError *inner_error = NULL;

  file = g_file_new_for_uri (self->user_mappings_uri);
  directory = g_file_get_parent (file);

  if (!g_file_query_exists (directory, NULL)) {
    g_file_make_directory_with_parents (directory, NULL, &inner_error);
    if (G_UNLIKELY (inner_error != NULL)) {
      g_propagate_error (error, inner_error);

      return;
    }
  }

  stream = g_file_replace (file, NULL, FALSE, G_FILE_CREATE_NONE, NULL, &inner_error);
  if (G_UNLIKELY (inner_error != NULL)) {
    g_propagate_error (error, inner_error);

    return;
  }
  data_stream = g_data_output_stream_new (G_OUTPUT_STREAM (stream));

  g_hash_table_iter_init (&iter, self->user_mappings);
  while (g_hash_table_iter_next (&iter, (gpointer) &guid, (gpointer) &sdl_string)) {
    name = g_hash_table_lookup (self->names, guid);

    mapping_string = g_strdup_printf ("%s,%s,%s\n", guid, name, sdl_string);

    g_data_output_stream_put_string (data_stream, mapping_string, NULL, &inner_error);
    if (G_UNLIKELY (inner_error != NULL)) {
      g_propagate_error (error, inner_error);

      return;
    }
  }
}

static void
user_mappings_changed_cb (GFileMonitor          *monitor,
                          GFile                 *file,
                          GFile                 *other_file,
                          GFileMonitorEvent      event_type,
                          ManetteMappingManager *self)
{
  g_autoptr (GFile) user_mappings_file = NULL;
  g_autoptr (GError) error = NULL;

  g_hash_table_remove_all (self->user_mappings);

  if (G_UNLIKELY (event_type == G_FILE_MONITOR_EVENT_DELETED)) {
    g_signal_emit (self, signals[SIG_CHANGED], 0);

    return;
  }

  user_mappings_file = g_file_new_for_uri (self->user_mappings_uri);
  if (g_file_query_exists (user_mappings_file, NULL))
    add_from_file_uri (self, self->user_mappings_uri, self->user_mappings, &error);
  if (G_UNLIKELY (error != NULL)) {
    g_debug ("ManetteMappingManager: Can’t add mappings from %s: %s",
             self->user_mappings_uri,
             error->message);
  }

  g_signal_emit (self, signals[SIG_CHANGED], 0);
}

/* Public */

ManetteMappingManager *
manette_mapping_manager_new (void)
{
  ManetteMappingManager *self = NULL;
  g_autofree char *path = NULL;
  g_autoptr (GFile) user_mappings_file = NULL;
  GError *error = NULL;

  self = (ManetteMappingManager*) g_object_new (MANETTE_TYPE_MAPPING_MANAGER, NULL);

  if (self->names == NULL)
    self->names = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, g_free);

  if (self->default_mappings == NULL)
    self->default_mappings = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, g_free);
  if (self->user_mappings == NULL)
    self->user_mappings = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, g_free);

  add_from_file_uri (self, MAPPING_RESOURCE_URI, self->default_mappings, &error);
  if (G_UNLIKELY (error != NULL)) {
    g_critical ("ManetteMappingManager: Can’t add mappings from %s: %s",
                MAPPING_RESOURCE_URI,
                error->message);
    g_clear_error (&error);
  }

  path = g_build_filename (g_get_user_config_dir (), CONFIG_DIR, MAPPING_CONFIG_FILE, NULL);

  self->user_mappings_uri = g_filename_to_uri (path, NULL, &error);
  if (G_UNLIKELY (error != NULL)) {
    g_debug ("ManetteMappingManager: Can't build path for user config: %s",
             error->message);
    g_clear_error (&error);

    return self;
  }

  user_mappings_file = g_file_new_for_uri (self->user_mappings_uri);
  self->user_mappings_monitor = g_file_monitor_file (user_mappings_file,
                                                     G_FILE_MONITOR_NONE,
                                                     NULL,
                                                     &error);
  if (G_UNLIKELY (error != NULL)) {
    g_debug ("ManetteMappingManager: Can’t monitor mappings from %s: %s",
             self->user_mappings_uri,
             error->message);
    g_clear_error (&error);
  }

  g_signal_connect (self->user_mappings_monitor,
                    "changed",
                    G_CALLBACK (user_mappings_changed_cb),
                    self);

  if (g_file_query_exists (user_mappings_file, NULL))
    add_from_file_uri (self, self->user_mappings_uri, self->user_mappings, &error);
  if (G_UNLIKELY (error != NULL)) {
    g_debug ("ManetteMappingManager: Can’t add mappings from %s: %s",
             self->user_mappings_uri,
             error->message);
    g_clear_error (&error);
  }

  return self;
}

gboolean
manette_mapping_manager_has_user_mapping (ManetteMappingManager *self,
                                          const char            *guid)
{
  g_return_val_if_fail (MANETTE_IS_MAPPING_MANAGER (self), FALSE);
  g_return_val_if_fail (guid != NULL, FALSE);

  return g_hash_table_contains (self->user_mappings, guid);
}

char *
manette_mapping_manager_get_default_mapping (ManetteMappingManager *self,
                                             const char            *guid)
{
  const char *mapping;

  g_return_val_if_fail (MANETTE_IS_MAPPING_MANAGER (self), NULL);
  g_return_val_if_fail (guid != NULL, NULL);

  mapping = g_hash_table_lookup (self->default_mappings, guid);

  return g_strdup (mapping);
}

char *
manette_mapping_manager_get_user_mapping (ManetteMappingManager *self,
                                          const char            *guid)
{
  const char *mapping;

  g_return_val_if_fail (MANETTE_IS_MAPPING_MANAGER (self), NULL);
  g_return_val_if_fail (guid != NULL, NULL);

  mapping = g_hash_table_lookup (self->user_mappings, guid);

  return g_strdup (mapping);
}

char *
manette_mapping_manager_get_mapping (ManetteMappingManager *self,
                                     const char            *guid)
{
  char *mapping;

  g_return_val_if_fail (MANETTE_IS_MAPPING_MANAGER (self), NULL);
  g_return_val_if_fail (guid != NULL, NULL);

  mapping = manette_mapping_manager_get_user_mapping (self, guid);
  if (mapping == NULL)
    mapping = manette_mapping_manager_get_default_mapping (self, guid);

  return mapping;
}

void
manette_mapping_manager_save_mapping (ManetteMappingManager *self,
                                      const char            *guid,
                                      const char            *name,
                                      const char            *mapping)
{
  g_autoptr (GError) error = NULL;

  g_return_if_fail (MANETTE_IS_MAPPING_MANAGER (self));
  g_return_if_fail (guid != NULL);
  g_return_if_fail (name != NULL);
  g_return_if_fail (mapping != NULL);

  g_hash_table_insert (self->user_mappings, g_strdup (guid), g_strdup (mapping));
  g_hash_table_insert (self->names, g_strdup (guid), g_strdup (name));

  save_user_mappings (self, &error);
  if (G_UNLIKELY (error != NULL))
    g_critical ("ManetteMappingManager: Can’t save user mappings: %s", error->message);
}

void
manette_mapping_manager_delete_mapping (ManetteMappingManager *self,
                                        const char            *guid)
{
  g_autoptr (GError) error = NULL;

  g_return_if_fail (MANETTE_IS_MAPPING_MANAGER (self));
  g_return_if_fail (guid != NULL);

  g_hash_table_remove (self->user_mappings, guid);
  g_hash_table_remove (self->names, guid);

  save_user_mappings (self, &error);
  if (G_UNLIKELY (error != NULL))
    g_critical ("ManetteMappingManager: Can’t save user mappings: %s", error->message);
}

GList *
manette_mapping_manager_get_default_mappings (ManetteMappingManager *self)
{
  g_return_val_if_fail (MANETTE_IS_MAPPING_MANAGER (self), NULL);

  return g_hash_table_get_values (self->default_mappings);
}

/* Type */

static void
manette_mapping_manager_finalize (GObject *object)
{
  ManetteMappingManager *self = MANETTE_MAPPING_MANAGER (object);

  g_clear_pointer (&self->names, g_hash_table_unref);
  g_clear_pointer (&self->default_mappings, g_hash_table_unref);
  g_clear_pointer (&self->user_mappings, g_hash_table_unref);
  g_clear_pointer (&self->user_mappings_uri, g_free);
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

  G_OBJECT_CLASS (klass)->finalize = manette_mapping_manager_finalize;
}

static void
manette_mapping_manager_init (ManetteMappingManager *self)
{
}
