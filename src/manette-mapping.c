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
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "manette-mapping.h"

#include <errno.h>
#include <linux/input-event-codes.h>
#include <stdlib.h>
#include <string.h>

struct _ManetteMapping {
  GObject parent_instance;

  GArray *axis_bindings;
  GArray *button_bindings;
  GArray *hat_bindings;
};

G_DEFINE_TYPE (ManetteMapping, manette_mapping, G_TYPE_OBJECT)

G_DEFINE_BOXED_TYPE (ManetteMappingBinding, manette_mapping_binding, manette_mapping_binding_copy, manette_mapping_binding_free)

/* Private */

static void
manette_mapping_finalize (GObject *object)
{
  ManetteMapping *self = (ManetteMapping *)object;

  if (self->axis_bindings != NULL)
    g_array_free (self->axis_bindings, TRUE);
  if (self->button_bindings != NULL)
    g_array_free (self->button_bindings, TRUE);
  if (self->hat_bindings != NULL)
    g_array_free (self->hat_bindings, TRUE);

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
bindings_array_has_destination_input (GArray *array,
                                      guint   type,
                                      guint   code)
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
try_str_to_guint16 (gchar    *start,
                    gchar   **end,
                    guint16  *result)
{
  g_return_val_if_fail (start != NULL, FALSE);
  g_return_val_if_fail (end != NULL, FALSE);
  g_return_val_if_fail (result != NULL, FALSE);

  errno = 0;
  *result = strtol (start, end, 10);

  return errno == 0;
}

static gboolean
parse_mapping_input_type (gchar                    *start,
                          gchar                   **end,
                          ManetteMappingInputType  *input_type)
{
  g_return_val_if_fail (start != NULL, FALSE);
  g_return_val_if_fail (end != NULL, FALSE);
  g_return_val_if_fail (input_type != NULL, FALSE);

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
parse_mapping_index (gchar    *start,
                     gchar   **end,
                     guint16  *index)
{
  return try_str_to_guint16 (start, end, index);
}

static gboolean
parse_mapping_invert (gchar     *start,
                      gchar    **end,
                      gboolean  *invert)
{
  g_return_val_if_fail (start != NULL, FALSE);
  g_return_val_if_fail (end != NULL, FALSE);
  g_return_val_if_fail (invert != NULL, FALSE);

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
parse_mapping_range (gchar                *start,
                     gchar               **end,
                     ManetteMappingRange  *range)
{
  g_return_val_if_fail (start != NULL, FALSE);
  g_return_val_if_fail (end != NULL, FALSE);
  g_return_val_if_fail (range != NULL, FALSE);

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
parse_mapping_hat (gchar                *start,
                   gchar               **end,
                   guint16              *index,
                   ManetteMappingRange  *range,
                   gboolean             *invert)
{
  guint16 hat_index;
  guint16 hat_position_2pow;
  guint16 hat_position = 0;

  g_return_val_if_fail (start != NULL, FALSE);
  g_return_val_if_fail (end != NULL, FALSE);
  g_return_val_if_fail (index != NULL, FALSE);
  g_return_val_if_fail (range != NULL, FALSE);
  g_return_val_if_fail (invert != NULL, FALSE);

  if (!try_str_to_guint16 (start, end, &hat_index))
    return FALSE;

  start = *end;

  if (start == NULL || *start != '.')
    return FALSE;

  start++;

  if (!try_str_to_guint16 (start, end, &hat_position_2pow))
    return FALSE;

  // hat_position: 0 up, 1 right, 2 down, 3 left.
  while (hat_position_2pow > 1) {
    hat_position_2pow >>= 1;
    hat_position++;
  }

  if (hat_position > 3)
    return FALSE;

  // hat0x, hat0y, hat1x, hat1y…
  *index = hat_index * 2 + (hat_position + 1) % 2;
  // left or up: negative, right or down: position.
  *range = ((hat_position + 1) % 4) < 2 ? MANETTE_MAPPING_RANGE_NEGATIVE :
                                          MANETTE_MAPPING_RANGE_POSITIVE;
  *invert = *range == MANETTE_MAPPING_RANGE_NEGATIVE;

  return TRUE;
}

static gboolean
parse_destination_input (gchar    *start,
                         gchar   **end,
                         guint16  *type,
                         guint16  *code)
{
  const static struct {
    guint16 type;
    guint16 code;
    const gchar *string_value;
  } values[] = {
    { EV_ABS, ABS_X, "leftx" },
    { EV_ABS, ABS_Y, "lefty" },
    { EV_ABS, ABS_RX, "rightx" },
    { EV_ABS, ABS_RY, "righty" },
    { EV_KEY, BTN_A, "a" },
    { EV_KEY, BTN_B, "b" },
    { EV_KEY, BTN_DPAD_DOWN, "dpdown" },
    { EV_KEY, BTN_DPAD_LEFT, "dpleft" },
    { EV_KEY, BTN_DPAD_RIGHT, "dpright" },
    { EV_KEY, BTN_DPAD_UP, "dpup" },
    { EV_KEY, BTN_MODE, "guide" },
    { EV_KEY, BTN_SELECT, "back" },
    { EV_KEY, BTN_TL, "leftshoulder" },
    { EV_KEY, BTN_TR, "rightshoulder" },
    { EV_KEY, BTN_START, "start" },
    { EV_KEY, BTN_THUMBL, "leftstick" },
    { EV_KEY, BTN_THUMBR, "rightstick" },
    { EV_KEY, BTN_TL2, "lefttrigger" },
    { EV_KEY, BTN_TR2, "righttrigger" },
    { EV_KEY, BTN_Y, "x" },
    { EV_KEY, BTN_X, "y" },
  };
  const gint length = sizeof (values) / sizeof (values[0]);
  gint i;

  for (i = 0; i < length; i++)
    if (g_strcmp0 (start, values[i].string_value) == 0) {
      *type = values[i].type;
      *code = values[i].code;
      *end = start + strlen (values[i].string_value);

      return TRUE;
    }

  return FALSE;
}

static void
manette_mapping_binding_try_free (ManetteMappingBinding **binding)
{
  if (binding == NULL)
    return;

  if (*binding == NULL)
    return;

  manette_mapping_binding_free (*binding);
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
parse_mapping_destination (gchar                 *destination,
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

  if (*destination != '\0')
    return FALSE;

  return TRUE;
}

static gboolean
parse_mapping_source (gchar                 *source,
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

// This function doesn't take care of cleaning up the object's state before
// setting it.
static void
set_from_mapping_string (ManetteMapping *self,
                         const gchar    *mapping_string)
{
  gchar **mappings;
  guint mappings_length;
  guint i = 0;
  gchar **splitted_mapping;
  gchar *destination_string;
  gchar *source_string;
  ManetteMappingBinding binding = {};

  mappings = g_strsplit (mapping_string, ",", 0);
  mappings_length = g_strv_length (mappings);
  for (i = 0; i < mappings_length; i++) {

    splitted_mapping = g_strsplit (mappings[i], ":", 0);

    if (g_strv_length (splitted_mapping) != 2) {
      g_strfreev (splitted_mapping);

      continue;
    }

    destination_string = splitted_mapping[0];
    source_string = splitted_mapping[1];

    /* Skip the platform key. */
    if (g_strcmp0 ("platform", splitted_mapping[0]) == 0) {
      g_strfreev (splitted_mapping);

      continue;
    }

    if (!parse_mapping_destination (destination_string, &binding)) {
      g_critical ("Invalid mapping destination: %s", splitted_mapping[0]);
      g_strfreev (splitted_mapping);

      continue;
    }

    if  (binding.destination.type == EV_MAX) {
      g_debug ("Invalid token: %s", destination_string);
      g_strfreev (splitted_mapping);

      continue;
    }

    if (!parse_mapping_source (source_string, &binding)) {
      g_critical ("Invalid mapping source: %s", splitted_mapping[1]);
      g_strfreev (splitted_mapping);

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

    g_strfreev (splitted_mapping);
  }

  g_strfreev (mappings);
}

static void
g_array_try_free (GArray **array)
{
  if (array == NULL)
    return;

  if (*array == NULL)
    return;

  g_array_free (*array, TRUE);
}

ManetteMapping *
manette_mapping_new (const gchar  *mapping_string,
                     GError      **error)
{
  ManetteMapping *self = NULL;

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

  set_from_mapping_string (self, mapping_string);

  return self;
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
 * manette_mapping_has_destination_input:
 * @self: a #ManetteMapping
 * @type: the input type
 * @code: the input code
 *
 * Gets whether the mapping has the given destination input.
 *
 * Returns: whether the device has the given destination input
 */
gboolean
manette_mapping_has_destination_input (ManetteMapping *self,
                                       guint           type,
                                       guint           code)
{
  g_return_val_if_fail (MANETTE_IS_MAPPING (self), FALSE);

  if (bindings_array_has_destination_input (self->axis_bindings, type, code))
    return TRUE;

  if (bindings_array_has_destination_input (self->button_bindings, type, code))
    return TRUE;

  if (bindings_array_has_destination_input (self->hat_bindings, type, code))
    return TRUE;

  return FALSE;
}
