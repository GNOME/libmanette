/* test-event-mapping.c
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

#include "../src/manette-event-mapping-private.h"

#define MAPPING_EMPTY "00000000000000000000000000000000,empty,"
#define MAPPING_BUTTON "00000000000000000000000000000000,button,a:b0,b:b1,x:b2,y:b3,"
#define MAPPING_AXIS "00000000000000000000000000000000,axis,leftx:a0,lefty:a1,-rightx:-a2,+rightx:+a2,-righty:+a3~,+righty:-a3~,"
#define MAPPING_HAT "00000000000000000000000000000000,hat,dpleft:h0.8,dpright:h0.2,dpup:h0.1,dpdown:h0.4,"
#define MAPPING_AXIS_DPAD "00000000000000000000000000000000,button,dpleft:-a0,dpright:+a0,dpup:-a1,dpdown:+a1,"
#define MAPPING_AXIS_TRIGGER "00000000000000000000000000000000,trigger,lefttrigger:a0,righttrigger:a1,"

static void
test_null (void)
{
  ManetteMapping *mapping;
  g_autoptr (GError) error = NULL;

  mapping = manette_mapping_new (NULL, &error);
  g_assert_error (error,
                  MANETTE_MAPPING_ERROR,
                  MANETTE_MAPPING_ERROR_NOT_A_MAPPING);

  g_assert_null (mapping);
}

static void
test_empty_mapping (void)
{
  g_autoptr (ManetteMapping) mapping = NULL;
  GSList *mapped_events;
  GError *error = NULL;

  mapping = manette_mapping_new (MAPPING_EMPTY, &error);
  g_assert_no_error (error);
  g_assert_nonnull (mapping);
  g_assert_true (MANETTE_IS_MAPPING (mapping));

  mapped_events = manette_map_button_event (mapping, 0, TRUE);
  g_assert_cmpint (g_slist_length (mapped_events), ==, 0);
}

static void
test_button_mapping (void)
{
  g_autoptr (ManetteMapping) mapping = NULL;
  GSList *mapped_events;
  ManetteMappedEvent *mapped_event;
  GError *error = NULL;

  mapping = manette_mapping_new (MAPPING_BUTTON, &error);
  g_assert_no_error (error);
  g_assert_nonnull (mapping);
  g_assert_true (MANETTE_IS_MAPPING (mapping));

  mapped_events = manette_map_button_event (mapping, 0, TRUE);
  g_assert_cmpint (g_slist_length (mapped_events), ==, 1);

  mapped_event = mapped_events->data;
  g_assert_cmpint (mapped_event->type, ==, MANETTE_MAPPING_DESTINATION_TYPE_BUTTON);
  g_assert_cmpint (mapped_event->button.button, ==, MANETTE_BUTTON_SOUTH);
  g_assert_true (mapped_event->button.pressed);

  g_slist_free_full (mapped_events, (GDestroyNotify) g_free);
}

static void
test_axis_mapping (void)
{
  g_autoptr (ManetteMapping) mapping = NULL;
  GSList *mapped_events;
  ManetteMappedEvent *mapped_event;
  GError *error = NULL;

  mapping = manette_mapping_new (MAPPING_AXIS, &error);
  g_assert_no_error (error);
  g_assert_nonnull (mapping);
  g_assert_true (MANETTE_IS_MAPPING (mapping));

  mapped_events = manette_map_absolute_event (mapping, 0, 0.0);
  g_assert_cmpint (g_slist_length (mapped_events), ==, 1);

  mapped_event = mapped_events->data;
  g_assert_cmpint (mapped_event->type, ==, MANETTE_MAPPING_DESTINATION_TYPE_AXIS);
  g_assert_cmpint (mapped_event->axis.axis, ==, MANETTE_AXIS_LEFT_X);
  g_assert_cmpfloat (mapped_event->axis.value, ==, 0);

  g_slist_free_full (mapped_events, (GDestroyNotify) g_free);

  mapped_events = manette_map_absolute_event (mapping, 1, 0.0);
  g_assert_cmpint (g_slist_length (mapped_events), ==, 1);

  mapped_event = mapped_events->data;
  g_assert_cmpint (mapped_event->type, ==, MANETTE_MAPPING_DESTINATION_TYPE_AXIS);
  g_assert_cmpint (mapped_event->axis.axis, ==, MANETTE_AXIS_LEFT_Y);
  g_assert_cmpfloat (mapped_event->axis.value, ==, 0);

  g_slist_free_full (mapped_events, (GDestroyNotify) g_free);
}

static void
test_hat_mapping (void)
{
  g_autoptr (ManetteMapping) mapping = NULL;
  GSList *mapped_events;
  ManetteMappedEvent *mapped_event;
  GError *error = NULL;

  mapping = manette_mapping_new (MAPPING_HAT, &error);
  g_assert_no_error (error);
  g_assert_nonnull (mapping);
  g_assert_true (MANETTE_IS_MAPPING (mapping));

  mapped_events = manette_map_hat_event (mapping, 0, 0);
  g_assert_cmpint (g_slist_length (mapped_events), ==, 2);

  mapped_event = mapped_events->data;
  g_assert_cmpint (mapped_event->type, ==, MANETTE_MAPPING_DESTINATION_TYPE_BUTTON);
  g_assert_cmpint (mapped_event->button.button, ==, MANETTE_BUTTON_DPAD_LEFT);
  g_assert_false (mapped_event->button.pressed);

  mapped_event = mapped_events->next->data;
  g_assert_cmpint (mapped_event->type, ==, MANETTE_MAPPING_DESTINATION_TYPE_BUTTON);
  g_assert_cmpint (mapped_event->button.button, ==, MANETTE_BUTTON_DPAD_RIGHT);
  g_assert_false (mapped_event->button.pressed);

  g_slist_free_full (mapped_events, (GDestroyNotify) g_free);

  mapped_events = manette_map_hat_event (mapping, 0, -1);
  g_assert_cmpint (g_slist_length (mapped_events), ==, 2);

  mapped_event = mapped_events->data;
  g_assert_cmpint (mapped_event->type, ==, MANETTE_MAPPING_DESTINATION_TYPE_BUTTON);
  g_assert_cmpint (mapped_event->button.button, ==, MANETTE_BUTTON_DPAD_LEFT);
  g_assert_true (mapped_event->button.pressed);

  mapped_event = mapped_events->next->data;
  g_assert_cmpint (mapped_event->type, ==, MANETTE_MAPPING_DESTINATION_TYPE_BUTTON);
  g_assert_cmpint (mapped_event->button.button, ==, MANETTE_BUTTON_DPAD_RIGHT);
  g_assert_false (mapped_event->button.pressed);

  g_slist_free_full (mapped_events, (GDestroyNotify) g_free);

  mapped_events = manette_map_hat_event (mapping, 0, 1);
  g_assert_cmpint (g_slist_length (mapped_events), ==, 2);

  mapped_event = mapped_events->data;
  g_assert_cmpint (mapped_event->type, ==, MANETTE_MAPPING_DESTINATION_TYPE_BUTTON);
  g_assert_cmpint (mapped_event->button.button, ==, MANETTE_BUTTON_DPAD_LEFT);
  g_assert_false (mapped_event->button.pressed);

  mapped_event = mapped_events->next->data;
  g_assert_cmpint (mapped_event->type, ==, MANETTE_MAPPING_DESTINATION_TYPE_BUTTON);
  g_assert_cmpint (mapped_event->button.button, ==, MANETTE_BUTTON_DPAD_RIGHT);
  g_assert_true (mapped_event->button.pressed);

  g_slist_free_full (mapped_events, (GDestroyNotify) g_free);
}

static void
test_axis_dpad_mapping (void)
{
  g_autoptr (ManetteMapping) mapping = NULL;
  GSList *mapped_events;
  ManetteMappedEvent *mapped_event;
  GError *error = NULL;

  mapping = manette_mapping_new (MAPPING_AXIS_DPAD, &error);
  g_assert_no_error (error);
  g_assert_nonnull (mapping);
  g_assert_true (MANETTE_IS_MAPPING (mapping));

  mapped_events = manette_map_absolute_event (mapping, 0, 0.0);
  g_assert_cmpint (g_slist_length (mapped_events), ==, 2);

  mapped_event = mapped_events->data;
  g_assert_cmpint (mapped_event->type, ==, MANETTE_MAPPING_DESTINATION_TYPE_BUTTON);
  g_assert_cmpint (mapped_event->button.button, ==, MANETTE_BUTTON_DPAD_LEFT);
  g_assert_false (mapped_event->button.pressed);

  mapped_event = mapped_events->next->data;
  g_assert_cmpint (mapped_event->type, ==, MANETTE_MAPPING_DESTINATION_TYPE_BUTTON);
  g_assert_cmpint (mapped_event->button.button, ==, MANETTE_BUTTON_DPAD_RIGHT);
  g_assert_false (mapped_event->button.pressed);

  g_slist_free_full (mapped_events, (GDestroyNotify) g_free);

  mapped_events = manette_map_absolute_event (mapping, 0, -1.0);
  g_assert_cmpint (g_slist_length (mapped_events), ==, 2);

  mapped_event = mapped_events->data;
  g_assert_cmpint (mapped_event->type, ==, MANETTE_MAPPING_DESTINATION_TYPE_BUTTON);
  g_assert_cmpint (mapped_event->button.button, ==, MANETTE_BUTTON_DPAD_LEFT);
  g_assert_true (mapped_event->button.pressed);

  mapped_event = mapped_events->next->data;
  g_assert_cmpint (mapped_event->type, ==, MANETTE_MAPPING_DESTINATION_TYPE_BUTTON);
  g_assert_cmpuint (mapped_event->button.button, ==, MANETTE_BUTTON_DPAD_RIGHT);
  g_assert_false (mapped_event->button.pressed);

  g_slist_free_full (mapped_events, (GDestroyNotify) g_free);

  mapped_events = manette_map_absolute_event (mapping, 0, 1.0);
  g_assert_cmpint (g_slist_length (mapped_events), ==, 2);

  mapped_event = mapped_events->data;
  g_assert_cmpint (mapped_event->type, ==, MANETTE_MAPPING_DESTINATION_TYPE_BUTTON);
  g_assert_cmpuint (mapped_event->button.button, ==, MANETTE_BUTTON_DPAD_LEFT);
  g_assert_false (mapped_event->button.pressed);

  mapped_event = mapped_events->next->data;
  g_assert_cmpint (mapped_event->type, ==, MANETTE_MAPPING_DESTINATION_TYPE_BUTTON);
  g_assert_cmpuint (mapped_event->button.button, ==, MANETTE_BUTTON_DPAD_RIGHT);
  g_assert_true (mapped_event->button.pressed);

  g_slist_free_full (mapped_events, (GDestroyNotify) g_free);

  mapped_events = manette_map_absolute_event (mapping, 1, 0.0);
  g_assert_cmpint (g_slist_length (mapped_events), ==, 2);

  mapped_event = mapped_events->data;
  g_assert_cmpint (mapped_event->type, ==, MANETTE_MAPPING_DESTINATION_TYPE_BUTTON);
  g_assert_cmpint (mapped_event->button.button, ==, MANETTE_BUTTON_DPAD_UP);
  g_assert_false (mapped_event->button.pressed);

  mapped_event = mapped_events->next->data;
  g_assert_cmpint (mapped_event->type, ==, MANETTE_MAPPING_DESTINATION_TYPE_BUTTON);
  g_assert_cmpint (mapped_event->button.button, ==, MANETTE_BUTTON_DPAD_DOWN);
  g_assert_false (mapped_event->button.pressed);

  g_slist_free_full (mapped_events, (GDestroyNotify) g_free);

  mapped_events = manette_map_absolute_event (mapping, 1, -1.0);
  g_assert_cmpint (g_slist_length (mapped_events), ==, 2);

  mapped_event = mapped_events->data;
  g_assert_cmpint (mapped_event->type, ==, MANETTE_MAPPING_DESTINATION_TYPE_BUTTON);
  g_assert_cmpint (mapped_event->button.button, ==, MANETTE_BUTTON_DPAD_UP);
  g_assert_true (mapped_event->button.pressed);

  mapped_event = mapped_events->next->data;
  g_assert_cmpint (mapped_event->type, ==, MANETTE_MAPPING_DESTINATION_TYPE_BUTTON);
  g_assert_cmpuint (mapped_event->button.button, ==, MANETTE_BUTTON_DPAD_DOWN);
  g_assert_false (mapped_event->button.pressed);

  g_slist_free_full (mapped_events, (GDestroyNotify) g_free);

  mapped_events = manette_map_absolute_event (mapping, 1, 1.0);
  g_assert_cmpint (g_slist_length (mapped_events), ==, 2);

  mapped_event = mapped_events->data;
  g_assert_cmpint (mapped_event->type, ==, MANETTE_MAPPING_DESTINATION_TYPE_BUTTON);
  g_assert_cmpuint (mapped_event->button.button, ==, MANETTE_BUTTON_DPAD_UP);
  g_assert_false (mapped_event->button.pressed);

  mapped_event = mapped_events->next->data;
  g_assert_cmpint (mapped_event->type, ==, MANETTE_MAPPING_DESTINATION_TYPE_BUTTON);
  g_assert_cmpuint (mapped_event->button.button, ==, MANETTE_BUTTON_DPAD_DOWN);
  g_assert_true (mapped_event->button.pressed);

  g_slist_free_full (mapped_events, (GDestroyNotify) g_free);
}

static void
test_axis_trigger_mapping (void)
{
  g_autoptr (ManetteMapping) mapping = NULL;
  GSList *mapped_events;
  ManetteMappedEvent *mapped_event;
  GError *error = NULL;

  mapping = manette_mapping_new (MAPPING_AXIS_TRIGGER, &error);
  g_assert_no_error (error);
  g_assert_nonnull (mapping);
  g_assert_true (MANETTE_IS_MAPPING (mapping));

  mapped_events = manette_map_absolute_event (mapping, 0, 1.0);
  g_assert_cmpint (g_slist_length (mapped_events), ==, 1);

  mapped_event = mapped_events->data;
  g_assert_cmpint (mapped_event->type, ==, MANETTE_MAPPING_DESTINATION_TYPE_AXIS);
  g_assert_cmpint (mapped_event->axis.axis, ==, MANETTE_AXIS_LEFT_TRIGGER);
  g_assert_cmpfloat (mapped_event->axis.value, ==, 1.0);

  g_slist_free_full (mapped_events, (GDestroyNotify) g_free);

  mapped_events = manette_map_absolute_event (mapping, 0, -1.0);
  g_assert_cmpint (g_slist_length (mapped_events), ==, 1);

  mapped_event = mapped_events->data;
  g_assert_cmpint (mapped_event->type, ==, MANETTE_MAPPING_DESTINATION_TYPE_AXIS);
  g_assert_cmpint (mapped_event->axis.axis, ==, MANETTE_AXIS_LEFT_TRIGGER);
  g_assert_cmpfloat (mapped_event->axis.value, ==, 0.0);

  g_slist_free_full (mapped_events, (GDestroyNotify) g_free);

  mapped_events = manette_map_absolute_event (mapping, 1, 1.0);
  g_assert_cmpint (g_slist_length (mapped_events), ==, 1);

  mapped_event = mapped_events->data;
  g_assert_cmpint (mapped_event->type, ==, MANETTE_MAPPING_DESTINATION_TYPE_AXIS);
  g_assert_cmpint (mapped_event->button.button, ==, MANETTE_AXIS_RIGHT_TRIGGER);
  g_assert_cmpfloat (mapped_event->axis.value, ==, 1.0);

  g_slist_free_full (mapped_events, (GDestroyNotify) g_free);

  mapped_events = manette_map_absolute_event (mapping, 1, -1.0);
  g_assert_cmpint (g_slist_length (mapped_events), ==, 1);

  mapped_event = mapped_events->data;
  g_assert_cmpint (mapped_event->type, ==, MANETTE_MAPPING_DESTINATION_TYPE_AXIS);
  g_assert_cmpint (mapped_event->button.button, ==, MANETTE_AXIS_RIGHT_TRIGGER);
  g_assert_cmpfloat (mapped_event->axis.value, ==, 0.0);

  g_slist_free_full (mapped_events, (GDestroyNotify) g_free);
}

int
main (int   argc,
      char *argv[])
{
  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/ManetteEventMapping/test_null", test_null);
  g_test_add_func ("/ManetteEventMapping/test_empty_mapping", test_empty_mapping);
  g_test_add_func ("/ManetteEventMapping/test_button_mapping", test_button_mapping);
  g_test_add_func ("/ManetteEventMapping/test_axis_mapping", test_axis_mapping);
  g_test_add_func ("/ManetteEventMapping/test_hat_mapping", test_hat_mapping);
  g_test_add_func ("/ManetteEventMapping/test_axis_dpad_mapping", test_axis_dpad_mapping);
  g_test_add_func ("/ManetteEventMapping/test_axis_trigger_mapping", test_axis_trigger_mapping);

  return g_test_run();
}
