/* manette-event-mapping.c
 *
 * Copyright (C) 2018 Adrien Plazas <kekun.plazas@laposte.net>
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

#include "manette-event-mapping-private.h"

GSList *
manette_map_button_event (ManetteMapping *mapping,
                          guint           index,
                          gboolean        pressed)
{
  const ManetteMappingBinding * const *bindings;
  const ManetteMappingBinding * binding;
  GSList *mapped_events = NULL;

  bindings = manette_mapping_get_bindings (mapping,
                                           MANETTE_MAPPING_INPUT_TYPE_BUTTON,
                                           index);
  if (bindings == NULL)
    return NULL;

  for (; *bindings != NULL; bindings++) {
    ManetteMappedEvent *mapped_event = g_new0 (ManetteMappedEvent, 1);

    binding = *bindings;

    mapped_event->type = binding->destination.type;

    switch (binding->destination.type) {
    case MANETTE_MAPPING_DESTINATION_TYPE_AXIS:
      mapped_event->axis.axis = binding->destination.code;

      switch (binding->destination.range) {
      case MANETTE_MAPPING_RANGE_NEGATIVE:
        mapped_event->axis.value = pressed ? -1 : 0;
        break;
      case MANETTE_MAPPING_RANGE_FULL:
      case MANETTE_MAPPING_RANGE_POSITIVE:
        mapped_event->axis.value = pressed ? 1 : 0;
        break;
      default:
        mapped_event->axis.value = 0;
        break;
      }

      break;
    case MANETTE_MAPPING_DESTINATION_TYPE_BUTTON:
      mapped_event->button.button = binding->destination.code;
      mapped_event->button.pressed = pressed;

      break;
    default:
      continue;
    }

    mapped_events = g_slist_append (mapped_events, mapped_event);
  }

  return mapped_events;
}

GSList *
manette_map_absolute_event (ManetteMapping *mapping,
                            guint           index,
                            double          value)
{
  const ManetteMappingBinding * const *bindings;
  const ManetteMappingBinding * binding;
  GSList *mapped_events = NULL;
  gboolean pressed;

  bindings = manette_mapping_get_bindings (mapping,
                                           MANETTE_MAPPING_INPUT_TYPE_AXIS,
                                           index);
  if (bindings == NULL)
    return NULL;

  for (; *bindings != NULL; bindings++) {
    ManetteMappedEvent *mapped_event;
    double absolute_value = value;

    binding = *bindings;

    if (binding->source.range == MANETTE_MAPPING_RANGE_NEGATIVE && absolute_value > 0.)
      absolute_value = 0;

    if (binding->source.range == MANETTE_MAPPING_RANGE_POSITIVE && absolute_value < 0.)
      absolute_value = 0;

    mapped_event = g_new0 (ManetteMappedEvent, 1);
    mapped_event->type = binding->destination.type;

    switch (binding->destination.type) {
    case MANETTE_MAPPING_DESTINATION_TYPE_AXIS:
      absolute_value = binding->source.invert ? -absolute_value : absolute_value;

      mapped_event->axis.axis = binding->destination.code;
      switch (binding->destination.range) {
      case MANETTE_MAPPING_RANGE_FULL:
        mapped_event->axis.value = absolute_value;
        break;
      case MANETTE_MAPPING_RANGE_NEGATIVE:
        mapped_event->axis.value = (absolute_value - 1) / 2;
        break;
      case MANETTE_MAPPING_RANGE_POSITIVE:
        mapped_event->axis.value = (absolute_value + 1) / 2;
        break;
      default:
        break;
      }

      break;
    case MANETTE_MAPPING_DESTINATION_TYPE_BUTTON:
      if (binding->source.range == MANETTE_MAPPING_RANGE_FULL) {
        pressed = binding->source.invert ? absolute_value < 0. :
                                           absolute_value > 0.;
      } else {
        pressed = binding->source.invert ? ABS (absolute_value) < 0.5 :
                                           ABS (absolute_value) > 0.5;
      }

      mapped_event->button.button = binding->destination.code;
      mapped_event->button.pressed = pressed;

      break;
    default:
      continue;
    }

    mapped_events = g_slist_append (mapped_events, mapped_event);
  }

  return mapped_events;
}

GSList *
manette_map_hat_event (ManetteMapping *mapping,
                       guint           index,
                       gint8           value)
{
  const ManetteMappingBinding * const *bindings;
  const ManetteMappingBinding * binding;
  GSList *mapped_events = NULL;

  bindings = manette_mapping_get_bindings (mapping,
                                           MANETTE_MAPPING_INPUT_TYPE_HAT,
                                           index);
  if (bindings == NULL)
    return NULL;

  for (; *bindings != NULL; bindings++) {
    ManetteMappedEvent *mapped_event = g_new0 (ManetteMappedEvent, 1);
    gboolean pressed;

    binding = *bindings;

    mapped_event->type = binding->destination.type;

    switch (binding->destination.type) {
    case MANETTE_MAPPING_DESTINATION_TYPE_AXIS:
      if (binding->source.range == MANETTE_MAPPING_RANGE_NEGATIVE && value > 0)
        continue;
      if (binding->source.range == MANETTE_MAPPING_RANGE_POSITIVE && value < 0)
        continue;

      mapped_event->axis.axis = binding->destination.code;
      mapped_event->axis.value = abs (value);

      break;
    case MANETTE_MAPPING_DESTINATION_TYPE_BUTTON:
      /* Since hat events are most of the time bound to multiple bindings, they
       * will share the same event value. Hence, if the hat is moved left, then
       * it'll be also processed by the mapping for the right dpad for example.
       * But if the dpad is moved quick enough it might skip the neutral point
       * and the direction that was moved from wouldn't be seen as unpressed.
       * Hence, the opposite direction to the current event must be processed
       * in a way that unpresses the direction that's no longer pressed.
       */
      if (binding->source.range == MANETTE_MAPPING_RANGE_NEGATIVE && value > 0)
        pressed = FALSE;
      else if (binding->source.range == MANETTE_MAPPING_RANGE_POSITIVE && value < 0)
        pressed = FALSE;
      else
        pressed = abs (value);

      mapped_event->button.button = binding->destination.code;
      mapped_event->button.pressed = pressed;

      break;
    default:
      continue;
    }

    mapped_events = g_slist_append (mapped_events, mapped_event);
  }

  return mapped_events;
}
