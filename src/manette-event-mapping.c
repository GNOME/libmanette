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

#include "manette-event-mapping-private.h"

#include <linux/input-event-codes.h>
#include "manette-event-private.h"

static GSList *
map_button_event (ManetteMapping     *mapping,
                  ManetteEventButton *event)
{
  const ManetteMappingBinding * const *bindings;
  const ManetteMappingBinding * binding;
  GSList *mapped_events = NULL;
  gboolean pressed;

  bindings = manette_mapping_get_bindings (mapping,
                                           MANETTE_MAPPING_INPUT_TYPE_BUTTON,
                                           event->hardware_index);
  if (bindings == NULL)
    return NULL;

  for (; *bindings != NULL; bindings++) {
    g_autoptr (ManetteEvent) mapped_event = manette_event_copy ((ManetteEvent *) event);

    binding = *bindings;

    pressed = event->type == MANETTE_EVENT_BUTTON_PRESS;

    switch (binding->destination.type) {
    case EV_ABS:
      mapped_event->any.type = MANETTE_EVENT_ABSOLUTE;
      mapped_event->absolute.axis = binding->destination.code;
      switch (binding->destination.range) {
      case MANETTE_MAPPING_RANGE_NEGATIVE:
        mapped_event->absolute.value = pressed ? -1 : 0;

        break;
      case MANETTE_MAPPING_RANGE_FULL:
      case MANETTE_MAPPING_RANGE_POSITIVE:
        mapped_event->absolute.value = pressed ? 1 : 0;

        break;
      default:
        mapped_event->absolute.value = 0;

        break;
      }

      break;
    case EV_KEY:
      mapped_event->any.type = pressed ? MANETTE_EVENT_BUTTON_PRESS :
                                         MANETTE_EVENT_BUTTON_RELEASE;
      mapped_event->button.button = binding->destination.code;

      break;
    default:
      continue;
    }

    mapped_events = g_slist_append (mapped_events, g_steal_pointer (&mapped_event));
  }

  return mapped_events;
}

static GSList *
map_absolute_event (ManetteMapping       *mapping,
                    ManetteEventAbsolute *event)
{
  const ManetteMappingBinding * const *bindings;
  const ManetteMappingBinding * binding;
  GSList *mapped_events = NULL;
  gdouble absolute_value;
  gboolean pressed;

  bindings = manette_mapping_get_bindings (mapping,
                                           MANETTE_MAPPING_INPUT_TYPE_AXIS,
                                           event->hardware_index);
  if (bindings == NULL)
    return NULL;

  for (; *bindings != NULL; bindings++) {
    g_autoptr (ManetteEvent) mapped_event = NULL;

    binding = *bindings;

    if (binding->source.range == MANETTE_MAPPING_RANGE_NEGATIVE &&
        event->value > 0.)
      continue;

    if (binding->source.range == MANETTE_MAPPING_RANGE_POSITIVE &&
        event->value < 0.)
      continue;

    mapped_event = manette_event_copy ((ManetteEvent *) event);

    switch (binding->destination.type) {
    case EV_ABS:
      absolute_value = binding->source.invert ? -event->value : event->value;

      mapped_event->any.type = MANETTE_EVENT_ABSOLUTE;
      mapped_event->absolute.axis = binding->destination.code;
      switch (binding->destination.range) {
      case MANETTE_MAPPING_RANGE_FULL:
        mapped_event->absolute.value = absolute_value;

        break;
      case MANETTE_MAPPING_RANGE_NEGATIVE:
        mapped_event->absolute.value = (absolute_value / 2) - 1;

        break;
      case MANETTE_MAPPING_RANGE_POSITIVE:
        mapped_event->absolute.value = (absolute_value / 2) + 1;

        break;
      default:
        break;
      }

      break;
    case EV_KEY:
      if (binding->source.range == MANETTE_MAPPING_RANGE_FULL)
        pressed = binding->source.invert ? event->value < 0. :
                                           event->value > 0.;
      else
        pressed = binding->source.invert ? event->value == 0. :
                                           event->value != 0.;

      mapped_event->any.type = pressed ? MANETTE_EVENT_BUTTON_PRESS :
                                         MANETTE_EVENT_BUTTON_RELEASE;
      mapped_event->button.button = binding->destination.code;

      break;
    default:
      continue;
    }

    mapped_events = g_slist_append (mapped_events, g_steal_pointer (&mapped_event));
  }

  return mapped_events;
}

static GSList *
map_hat_event (ManetteMapping  *mapping,
               ManetteEventHat *event)
{
  const ManetteMappingBinding * const *bindings;
  const ManetteMappingBinding * binding;
  GSList *mapped_events = NULL;
  gboolean pressed;

  bindings = manette_mapping_get_bindings (mapping,
                                           MANETTE_MAPPING_INPUT_TYPE_HAT,
                                           event->hardware_index);
  if (bindings == NULL)
    return NULL;

  for (; *bindings != NULL; bindings++) {
    g_autoptr (ManetteEvent) mapped_event = NULL;

    binding = *bindings;

    if (binding->source.range == MANETTE_MAPPING_RANGE_NEGATIVE &&
        event->value > 0)
      continue;

    if (binding->source.range == MANETTE_MAPPING_RANGE_POSITIVE &&
        event->value < 0)
      continue;

    mapped_event = manette_event_copy ((ManetteEvent *) event);

    pressed = abs (event->value);

    switch (binding->destination.type) {
    case EV_ABS:
      mapped_event->any.type = MANETTE_EVENT_ABSOLUTE;
      mapped_event->absolute.axis = binding->destination.code;
      mapped_event->absolute.value = abs (event->value);

      break;
    case EV_KEY:
      mapped_event->any.type = pressed ? MANETTE_EVENT_BUTTON_PRESS :
                                         MANETTE_EVENT_BUTTON_RELEASE;
      mapped_event->button.button = binding->destination.code;

      break;
    default:
      continue;
    }

    mapped_events = g_slist_append (mapped_events, g_steal_pointer (&mapped_event));
  }

  return mapped_events;
}

GSList *
manette_map_event (ManetteMapping *mapping,
                   ManetteEvent   *event)
{
  switch (manette_event_get_event_type (event)) {
  case MANETTE_EVENT_BUTTON_PRESS:
  case MANETTE_EVENT_BUTTON_RELEASE:
    return map_button_event (mapping, &event->button);
  case MANETTE_EVENT_ABSOLUTE:
    return map_absolute_event (mapping, &event->absolute);
  case MANETTE_EVENT_HAT:
    return map_hat_event (mapping, &event->hat);
  default:
    return NULL;
  }
}
