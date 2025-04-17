/* manette-mapping.c
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

#include "manette-mapping-private.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>

struct _ManetteMapping {
  GObject parent_instance;

  GArray *axis_bindings;
  GArray *button_bindings;
  GArray *hat_bindings;
};

G_DEFINE_FINAL_TYPE (ManetteMapping, manette_mapping, G_TYPE_OBJECT)

G_DEFINE_BOXED_TYPE (ManetteMappingBinding, manette_mapping_binding, manette_mapping_binding_copy, manette_mapping_binding_free)

#ifdef G_DISABLE_CHECKS

#define manette_ensure_is_parseable(expr) G_STMT_START{ (void)0; }G_STMT_END

#else

#define manette_ensure_is_parseable(expr) \
  G_STMT_START { \
    if (G_LIKELY (expr != NULL)) { } \
    else  { \
      g_log (G_LOG_DOMAIN, \
             G_LOG_LEVEL_DEBUG, \
             "%s: expression '%s' not parseable", \
             G_STRFUNC, \
             #expr); \
      return FALSE; \
    } \
  } G_STMT_END

#endif

/* Private */

static void
manette_mapping_finalize (GObject *object)
{
  ManetteMapping *self = (ManetteMapping *)object;

  g_clear_pointer (&self->axis_bindings, g_array_unref);
  g_clear_pointer (&self->button_bindings, g_array_unref);
  g_clear_pointer (&self->hat_bindings, g_array_unref);

  G_OBJECT_CLASS (manette_mapping_parent_class)->finalize (object);
}

static void
manette_mapping_class_init (ManetteMappingClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = manette_mapping_finalize;
}

static void
manette_mapping_init (ManetteMapping *self)
{
}

static gboolean
bindings_array_has_destination_input (GArray                        *array,
                                      ManetteMappingDestinationType  type,
                                      guint                          code)
{
  gsize i;
  gsize j;
  GArray *sub_array;
  ManetteMappingBinding *binding;

  for (i = 0; i < array->len; i++) {
    sub_array = g_array_index (array, GArray *, i);
    if (sub_array == NULL)
      continue;

    for (j = 0; j < sub_array->len; j++) {
      binding = g_array_index (sub_array, ManetteMappingBinding *, j);
      if (binding == NULL)
        continue;

      if (binding->destination.type == type &&
          binding->destination.code == code)
        return TRUE;
    }
  }

  return FALSE;
}

static gboolean
try_str_to_guint16 (char     *start,
                    char    **end,
                    guint16  *result)
{
  manette_ensure_is_parseable (start);
  manette_ensure_is_parseable (end);
  manette_ensure_is_parseable (result);

  errno = 0;
  *result = strtol (start, end, 10);

  return errno == 0;
}

static gboolean
parse_mapping_input_type (char                     *start,
                          char                    **end,
                          ManetteMappingInputType  *input_type)
{
  manette_ensure_is_parseable (start);
  manette_ensure_is_parseable (end);
  manette_ensure_is_parseable (input_type);

  switch (*start) {
  case 'a':
    *input_type = MANETTE_MAPPING_INPUT_TYPE_AXIS;
    *end = start + 1;

    return TRUE;
  case 'b':
    *input_type = MANETTE_MAPPING_INPUT_TYPE_BUTTON;
    *end = start + 1;

    return TRUE;
  case 'h':
    *input_type = MANETTE_MAPPING_INPUT_TYPE_HAT;
    *end = start + 1;

    return TRUE;
  default:
    return FALSE;
  }
}

static gboolean
parse_mapping_index (char     *start,
                     char    **end,
                     guint16  *index)
{
  return try_str_to_guint16 (start, end, index);
}

static gboolean
parse_mapping_invert (char      *start,
                      char     **end,
                      gboolean  *invert)
{
  manette_ensure_is_parseable (start);
  manette_ensure_is_parseable (end);
  manette_ensure_is_parseable (invert);

  switch (*start) {
  case '~':
    *invert = TRUE;
    *end = start + 1;

    return TRUE;
  default:
    *invert = FALSE;

    return TRUE;
  }
}

static gboolean
parse_mapping_range (char                 *start,
                     char                **end,
                     ManetteMappingRange  *range)
{
  manette_ensure_is_parseable (start);
  manette_ensure_is_parseable (end);
  manette_ensure_is_parseable (range);

  switch (*start) {
  case '+':
    *range = MANETTE_MAPPING_RANGE_POSITIVE;
    *end = start + 1;

    return TRUE;
  case '-':
    *range = MANETTE_MAPPING_RANGE_NEGATIVE;
    *end = start + 1;

    return TRUE;
  default:
    *range = MANETTE_MAPPING_RANGE_FULL;
    *end = start;

    return TRUE;
  }
}

static gboolean
parse_mapping_hat (char                 *start,
                   char                **end,
                   guint16              *index,
                   ManetteMappingRange  *range,
                   gboolean             *invert)
{
  guint16 hat_index;
  guint16 hat_position_2pow;
  guint16 hat_position = 0;

  manette_ensure_is_parseable (start);
  manette_ensure_is_parseable (end);
  manette_ensure_is_parseable (index);
  manette_ensure_is_parseable (range);
  manette_ensure_is_parseable (invert);

  if (!try_str_to_guint16 (start, end, &hat_index))
    return FALSE;

  start = *end;

  if (start == NULL || *start != '.')
    return FALSE;

  start++;

  if (!try_str_to_guint16 (start, end, &hat_position_2pow))
    return FALSE;

  /* hat_position: 0 up, 1 right, 2 down, 3 left. */
  while (hat_position_2pow > 1) {
    hat_position_2pow >>= 1;
    hat_position++;
  }

  if (hat_position > 3)
    return FALSE;

  /* hat0x, hat0y, hat1x, hat1y… */
  *index = hat_index * 2 + (hat_position + 1) % 2;
  /* left or up: negative, right or down: positive. */
  *range = ((hat_position + 1) % 4) < 2 ? MANETTE_MAPPING_RANGE_NEGATIVE :
                                          MANETTE_MAPPING_RANGE_POSITIVE;
  *invert = *range == MANETTE_MAPPING_RANGE_NEGATIVE;

  return TRUE;
}

static gboolean
parse_destination_input (char                           *start,
                         char                          **end,
                         ManetteMappingDestinationType  *type,
                         int                            *code)
{
  const static struct {
    ManetteAxis axis;
    const char *string_value;
  } axis_values[] = {
    { MANETTE_AXIS_LEFT_X, "leftx" },
    { MANETTE_AXIS_LEFT_Y, "lefty" },
    { MANETTE_AXIS_RIGHT_X, "rightx" },
    { MANETTE_AXIS_RIGHT_Y, "righty" },
    { MANETTE_AXIS_LEFT_TRIGGER, "lefttrigger" },
    { MANETTE_AXIS_RIGHT_TRIGGER, "righttrigger" },
  };

  const static struct {
    ManetteButton button;
    const char *string_value;
  } button_values[] = {
    { MANETTE_BUTTON_DPAD_UP, "dpup" },
    { MANETTE_BUTTON_DPAD_DOWN, "dpdown" },
    { MANETTE_BUTTON_DPAD_LEFT, "dpleft" },
    { MANETTE_BUTTON_DPAD_RIGHT, "dpright" },
    { MANETTE_BUTTON_NORTH, "y" },
    { MANETTE_BUTTON_SOUTH, "a" },
    { MANETTE_BUTTON_WEST, "x" },
    { MANETTE_BUTTON_EAST, "b" },
    { MANETTE_BUTTON_SELECT, "back" },
    { MANETTE_BUTTON_START, "start" },
    { MANETTE_BUTTON_MODE, "guide" },
    { MANETTE_BUTTON_LEFT_SHOULDER, "leftshoulder" },
    { MANETTE_BUTTON_RIGHT_SHOULDER, "rightshoulder" },
    { MANETTE_BUTTON_LEFT_STICK, "leftstick" },
    { MANETTE_BUTTON_RIGHT_STICK, "rightstick" },
    { MANETTE_BUTTON_LEFT_PADDLE1, "paddle2" },
    { MANETTE_BUTTON_LEFT_PADDLE2, "paddle4" },
    { MANETTE_BUTTON_RIGHT_PADDLE1, "paddle1" },
    { MANETTE_BUTTON_RIGHT_PADDLE2, "paddle3" },
    { MANETTE_BUTTON_MISC1, "misc1" },
    { MANETTE_BUTTON_MISC2, "misc2" },
    { MANETTE_BUTTON_MISC3, "misc3" },
    { MANETTE_BUTTON_MISC4, "misc4" },
    { MANETTE_BUTTON_MISC5, "misc5" },
    { MANETTE_BUTTON_MISC6, "misc6" },
    { MANETTE_BUTTON_TOUCHPAD, "touchpad" },
  };

  int i;

  for (i = 0; i < G_N_ELEMENTS (axis_values); i++) {
    if (g_strcmp0 (start, axis_values[i].string_value) == 0) {
      *type = MANETTE_MAPPING_DESTINATION_TYPE_AXIS;
      *code = axis_values[i].axis;
      *end = start + strlen (axis_values[i].string_value);

      return TRUE;
    }
  }

  for (i = 0; i < G_N_ELEMENTS (button_values); i++) {
    if (g_strcmp0 (start, button_values[i].string_value) == 0) {
      *type = MANETTE_MAPPING_DESTINATION_TYPE_BUTTON;
      *code = button_values[i].button;
      *end = start + strlen (button_values[i].string_value);

      return TRUE;
    }
  }

  return FALSE;
}

static void
manette_mapping_binding_try_free (ManetteMappingBinding **binding)
{
  if (G_LIKELY (binding))
    g_clear_pointer (binding, manette_mapping_binding_free);
}

static void
append_binding (GArray                *type_array,
                ManetteMappingBinding *binding)
{
  GArray *binding_array;
  ManetteMappingBinding *binding_copy;
  guint16 index = binding->source.index;

  if (type_array->len <= index)
    g_array_set_size (type_array, index + 1);

  if (g_array_index (type_array, GArray *, index) == NULL) {
    binding_array = g_array_new (TRUE, TRUE, sizeof (ManetteMappingBinding *));
    g_array_set_clear_func (binding_array, (GDestroyNotify) manette_mapping_binding_try_free);
    g_array_index (type_array, GArray *, index) = binding_array;
  }
  else
    binding_array = g_array_index (type_array, GArray *, index);

  binding_copy = manette_mapping_binding_copy (binding);
  g_array_append_val (binding_array, binding_copy);
}

static gboolean
parse_mapping_destination (char                  *destination,
                           ManetteMappingBinding *binding)
{
  if (!parse_mapping_range (destination,
                            &destination,
                            &binding->destination.range))
    return FALSE;

  if (!parse_destination_input (destination,
                                &destination,
                                &binding->destination.type,
                                &binding->destination.code))
    return FALSE;

  /* Triggers always use positive range */
  if (binding->destination.type == MANETTE_MAPPING_DESTINATION_TYPE_AXIS &&
      (binding->destination.code == MANETTE_AXIS_LEFT_TRIGGER ||
       binding->destination.code == MANETTE_AXIS_RIGHT_TRIGGER)) {
    binding->destination.range = MANETTE_MAPPING_RANGE_POSITIVE;
  }

  if (*destination != '\0')
    return FALSE;

  return TRUE;
}

static gboolean
parse_mapping_source (char                  *source,
                      ManetteMappingBinding *binding)
{
  if (!parse_mapping_range (source,
                            &source,
                            &binding->source.range))
    return FALSE;

  if (!parse_mapping_input_type (source,
                                 &source,
                                 &binding->source.type))
    return FALSE;

  switch (binding->source.type) {
  case MANETTE_MAPPING_INPUT_TYPE_AXIS:
    if (!parse_mapping_index (source,
                              &source,
                              &binding->source.index))
      return FALSE;

    if (!parse_mapping_invert (source,
                               &source,
                               &binding->source.invert))
      return FALSE;

    break;
  case MANETTE_MAPPING_INPUT_TYPE_BUTTON:
    if (binding->source.range != MANETTE_MAPPING_RANGE_FULL)
      return FALSE;

    if (!parse_mapping_index (source,
                              &source,
                              &binding->source.index))
      return FALSE;

    binding->source.invert = FALSE;

    break;
  case MANETTE_MAPPING_INPUT_TYPE_HAT:
    if (binding->source.range != MANETTE_MAPPING_RANGE_FULL)
      return FALSE;

    if (!parse_mapping_hat (source,
                            &source,
                            &binding->source.index,
                            &binding->source.range,
                            &binding->source.invert))
      return FALSE;

    break;
  default:
    return FALSE;
  }

  if (*source != '\0')
    return FALSE;

  return TRUE;
}

static gboolean
is_valid_guid (const char *string)
{
  if (!string)
    return FALSE;

  for (guint i = 0; i < 32; i++)
    if (!g_ascii_isxdigit (string[i]))
      return FALSE;

  return TRUE;
}

/* This function doesn't take care of cleaning up the object's state before
 * setting it.
 */
static void
set_from_mapping_string (ManetteMapping  *self,
                         const char      *mapping_string,
                         GError         **error)
{
  g_auto(GStrv) mappings = g_strsplit (mapping_string, ",", 0);
  guint mappings_length = g_strv_length (mappings);
  guint i = 0;
  char *destination_string;
  char *source_string;
  ManetteMappingBinding binding = {};

  if (mappings_length < 2) {
    g_set_error (error,
                 MANETTE_MAPPING_ERROR,
                 MANETTE_MAPPING_ERROR_NOT_A_MAPPING,
                 "Invalid mapping string: %s",
                 mapping_string);

    return;
  }

  if (!is_valid_guid (mappings[0])) {
    g_set_error (error,
                 MANETTE_MAPPING_ERROR,
                 MANETTE_MAPPING_ERROR_NOT_A_MAPPING,
                 "Invalid mapping string: no GUID: %s",
                 mapping_string);

    return;
  }

  for (i = 2; i < mappings_length; i++) {
    g_auto(GStrv) splitted_mapping = g_strsplit (mappings[i], ":", 0);

    if (g_strv_length (splitted_mapping) != 2)
      continue;

    destination_string = splitted_mapping[0];
    source_string = splitted_mapping[1];

    /* Skip the platform key. */
    if (g_strcmp0 ("platform", splitted_mapping[0]) == 0)
      continue;

    if (!parse_mapping_destination (destination_string, &binding)) {
      g_critical ("Invalid binding destination: %s:%s in %s", destination_string, source_string, mapping_string);

      continue;
    }

    if (!parse_mapping_source (source_string, &binding)) {
      g_critical ("Invalid binding source: %s:%s in %s", destination_string, source_string, mapping_string);

      continue;
    }

    switch (binding.source.type) {
    case MANETTE_MAPPING_INPUT_TYPE_AXIS:
      append_binding (self->axis_bindings, &binding);

      break;
    case MANETTE_MAPPING_INPUT_TYPE_BUTTON:
      append_binding (self->button_bindings, &binding);

      break;
    case MANETTE_MAPPING_INPUT_TYPE_HAT:
      append_binding (self->hat_bindings, &binding);

      break;
    default:
      g_assert_not_reached ();
    }
  }
}

static void
g_array_try_free (GArray **array)
{
  if (G_LIKELY (array))
    g_clear_pointer (array, g_array_unref);
}

static gboolean
has_destination_input (ManetteMapping                *self,
                       ManetteMappingDestinationType  type,
                       int                            code)
{
  if (bindings_array_has_destination_input (self->axis_bindings, type, code))
    return TRUE;

  if (bindings_array_has_destination_input (self->button_bindings, type, code))
    return TRUE;

  if (bindings_array_has_destination_input (self->hat_bindings, type, code))
    return TRUE;

  return FALSE;
}

ManetteMapping *
manette_mapping_new (const char   *mapping_string,
                     GError      **error)
{
  g_autoptr (ManetteMapping) self = NULL;
  GError *inner_error = NULL;

  if (mapping_string == NULL) {
    g_set_error_literal (error,
                         MANETTE_MAPPING_ERROR,
                         MANETTE_MAPPING_ERROR_NOT_A_MAPPING,
                         "The mapping string can’t be null.");

    return NULL;
  }

  if (mapping_string[0] == '\0') {
    g_set_error_literal (error,
                         MANETTE_MAPPING_ERROR,
                         MANETTE_MAPPING_ERROR_NOT_A_MAPPING,
                         "The mapping string can’t be empty.");

    return NULL;
  }

  self = (ManetteMapping*) g_object_new (MANETTE_TYPE_MAPPING, NULL);

  self->axis_bindings = g_array_new (FALSE, TRUE, sizeof (GArray *));
  g_array_set_clear_func (self->axis_bindings, (GDestroyNotify) g_array_try_free);
  self->button_bindings = g_array_new (FALSE, TRUE, sizeof (GArray *));
  g_array_set_clear_func (self->button_bindings, (GDestroyNotify) g_array_try_free);
  self->hat_bindings = g_array_new (FALSE, TRUE, sizeof (GArray *));
  g_array_set_clear_func (self->hat_bindings, (GDestroyNotify) g_array_try_free);

  set_from_mapping_string (self, mapping_string, &inner_error);
  if (G_UNLIKELY (inner_error != NULL)) {
    g_propagate_error (error, inner_error);

    return NULL;
  }

  return g_steal_pointer (&self);
}

const ManetteMappingBinding * const *
manette_mapping_get_bindings (ManetteMapping          *self,
                              ManetteMappingInputType  type,
                              guint16                  index)
{
  GArray *type_array;
  GArray *bindings_array;

  switch (type)
  {
  case MANETTE_MAPPING_INPUT_TYPE_AXIS:
    type_array = self->axis_bindings;

    break;
  case MANETTE_MAPPING_INPUT_TYPE_BUTTON:
    type_array = self->button_bindings;

    break;
  case MANETTE_MAPPING_INPUT_TYPE_HAT:
    type_array = self->hat_bindings;

    break;
  default:
    return NULL;
  }

  if (type_array == NULL)
    return NULL;

  if (index >= type_array->len)
    return NULL;

  bindings_array = g_array_index (type_array, GArray *, index);

  if (bindings_array == NULL)
    return NULL;

  return (const ManetteMappingBinding * const *) bindings_array->data;
}

ManetteMappingBinding *
manette_mapping_binding_new (void)
{
  ManetteMappingBinding *self;

  self = g_slice_new0 (ManetteMappingBinding);

  return self;
}

ManetteMappingBinding *
manette_mapping_binding_copy (ManetteMappingBinding *self)
{
  ManetteMappingBinding *copy;

  g_return_val_if_fail (self, NULL);

  copy = manette_mapping_binding_new ();
  memcpy (copy, self, sizeof (ManetteMappingBinding));

  return copy;
}

void
manette_mapping_binding_free (ManetteMappingBinding *self)
{
  g_return_if_fail (self);

  g_slice_free (ManetteMappingBinding, self);
}

/**
 * manette_mapping_has_destination_button:
 * @self: a mapping
 * @button: a button
 *
 * Gets whether the mapping has the given destination button.
 *
 * Returns: whether the device has the given destination button
 */
gboolean
manette_mapping_has_destination_button (ManetteMapping *self,
                                        ManetteButton   button)
{
  g_return_val_if_fail (MANETTE_IS_MAPPING (self), FALSE);

  return has_destination_input (self, MANETTE_MAPPING_DESTINATION_TYPE_BUTTON, button);
}

/**
 * manette_mapping_has_destination_axis:
 * @self: a mapping
 * @axis: an axis
 *
 * Gets whether the mapping has the given destination axis.
 *
 * Returns: whether the device has the given destination button
 */
gboolean
manette_mapping_has_destination_axis (ManetteMapping *self,
                                      ManetteAxis     axis)
{
  g_return_val_if_fail (MANETTE_IS_MAPPING (self), FALSE);

  return has_destination_input (self, MANETTE_MAPPING_DESTINATION_TYPE_AXIS, axis);
}
