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

#include <linux/input-event-codes.h>
#include "../src/manette-event-mapping-private.h"
#include "../src/manette-event-private.h"

#define MAPPING_EMPTY "00000000000000000000000000000000,empty,"
#define MAPPING_BUTTON "00000000000000000000000000000000,button,a:b0,b:b1,x:b2,y:b3,"
#define MAPPING_AXIS "00000000000000000000000000000000,axis,leftx:a0,lefty:a1,-rightx:-a2,+rightx:+a2,-righty:+a3~,+righty:-a3~,"
#define MAPPING_HAT "00000000000000000000000000000000,hat,dpleft:h0.8,dpright:h0.2,dpup:h0.1,dpdown:h0.4,"
#define MAPPING_AXIS_DPAD "00000000000000000000000000000000,button,dpleft:-a0,dpright:+a0,dpup:-a1,dpdown:+a1,"
#define MAPPING_AXIS_TRIGGER "00000000000000000000000000000000,trigger,lefttrigger:a0,righttrigger:a1,"

#define DUMMY_TIMESTAMP 0x76543210

static void
cmp_event_any_except_type (ManetteEvent *event1,
                           ManetteEvent *event2)
{
  g_assert_cmpint (event1->any.time, ==, event2->any.time);
  g_assert (event1->any.device == event2->any.device);
  g_assert_cmpuint (event1->any.hardware_type, ==, event2->any.hardware_type);
  g_assert_cmpuint (event1->any.hardware_code, ==, event2->any.hardware_code);
  g_assert_cmpint (event1->any.hardware_value, ==, event2->any.hardware_value);
  g_assert_cmpuint (event1->any.hardware_index, ==, event2->any.hardware_index);
}

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
  ManetteEvent event = {};
  GSList *mapped_events;
  GError *error = NULL;

  mapping = manette_mapping_new (MAPPING_EMPTY, &error);
  g_assert_no_error (error);
  g_assert_nonnull (mapping);
  g_assert_true (MANETTE_IS_MAPPING (mapping));

  event.any.type = MANETTE_EVENT_BUTTON_PRESS;
  event.any.time = DUMMY_TIMESTAMP;
  event.any.hardware_type = EV_KEY;
  event.any.hardware_code = BTN_SOUTH;
  event.any.hardware_value = G_MAXINT;
  event.any.hardware_index = 0;
  event.button.button = 0;

  mapped_events = manette_map_event (mapping, &event);
  g_assert_cmpint (g_slist_length (mapped_events), ==, 0);
}

static void
test_button_mapping (void)
{
  g_autoptr (ManetteMapping) mapping = NULL;
  ManetteEvent event = {};
  GSList *mapped_events;
  ManetteEvent *mapped_event;
  GError *error = NULL;

  mapping = manette_mapping_new (MAPPING_BUTTON, &error);
  g_assert_no_error (error);
  g_assert_nonnull (mapping);
  g_assert_true (MANETTE_IS_MAPPING (mapping));

  event.any.type = MANETTE_EVENT_BUTTON_PRESS;
  event.any.time = DUMMY_TIMESTAMP;
  event.any.hardware_type = EV_KEY;
  event.any.hardware_code = BTN_SOUTH;
  event.any.hardware_value = G_MAXINT;
  event.any.hardware_index = 0;

  mapped_events = manette_map_event (mapping, &event);
  g_assert_cmpint (g_slist_length (mapped_events), ==, 1);

  mapped_event = mapped_events->data;
  g_assert_cmpint (mapped_event->any.type, ==, MANETTE_EVENT_BUTTON_PRESS);
  cmp_event_any_except_type (mapped_event, &event);
  g_assert_cmpuint (mapped_event->button.button, ==, BTN_SOUTH);

  g_slist_free_full (mapped_events, (GDestroyNotify) manette_event_free);
}

static void
test_axis_mapping (void)
{
  g_autoptr (ManetteMapping) mapping = NULL;
  ManetteEvent event = {};
  GSList *mapped_events;
  ManetteEvent *mapped_event;
  GError *error = NULL;

  mapping = manette_mapping_new (MAPPING_AXIS, &error);
  g_assert_no_error (error);
  g_assert_nonnull (mapping);
  g_assert_true (MANETTE_IS_MAPPING (mapping));

  event.any.type = MANETTE_EVENT_ABSOLUTE;
  event.any.time = DUMMY_TIMESTAMP;
  event.any.hardware_type = EV_ABS;
  event.any.hardware_code = ABS_X;

  mapped_events = manette_map_event (mapping, &event);
  g_assert_cmpint (g_slist_length (mapped_events), ==, 1);

  mapped_event = mapped_events->data;
  g_assert_cmpint (mapped_event->any.type, ==, MANETTE_EVENT_ABSOLUTE);
  cmp_event_any_except_type (mapped_event, &event);
  g_assert_cmpuint (mapped_event->absolute.axis, ==, ABS_X);
  g_assert_cmpfloat (mapped_event->absolute.value, ==, 0);

  g_slist_free_full (mapped_events, (GDestroyNotify) manette_event_free);

  event.any.type = MANETTE_EVENT_ABSOLUTE;
  event.any.time = DUMMY_TIMESTAMP;
  event.any.hardware_type = EV_ABS;
  event.any.hardware_code = ABS_Y;
  event.any.hardware_index = 1;

  mapped_events = manette_map_event (mapping, &event);
  g_assert_cmpint (g_slist_length (mapped_events), ==, 1);

  mapped_event = mapped_events->data;
  g_assert_cmpint (mapped_event->any.type, ==, MANETTE_EVENT_ABSOLUTE);
  cmp_event_any_except_type (mapped_event, &event);
  g_assert_cmpuint (mapped_event->absolute.axis, ==, ABS_Y);
  g_assert_cmpfloat (mapped_event->absolute.value, ==, 0);

  g_slist_free_full (mapped_events, (GDestroyNotify) manette_event_free);
}

static void
test_hat_mapping (void)
{
  g_autoptr (ManetteMapping) mapping = NULL;
  ManetteEvent event = {};
  GSList *mapped_events;
  ManetteEvent *mapped_event;
  GError *error = NULL;

  mapping = manette_mapping_new (MAPPING_HAT, &error);
  g_assert_no_error (error);
  g_assert_nonnull (mapping);
  g_assert_true (MANETTE_IS_MAPPING (mapping));

  event.any.type = MANETTE_EVENT_HAT;
  event.any.time = DUMMY_TIMESTAMP;
  event.any.hardware_type = EV_ABS;
  event.any.hardware_code = ABS_HAT0X;

  mapped_events = manette_map_event (mapping, &event);
  g_assert_cmpint (g_slist_length (mapped_events), ==, 2);

  mapped_event = mapped_events->data;
  g_assert_cmpint (mapped_event->any.type, ==, MANETTE_EVENT_BUTTON_RELEASE);
  cmp_event_any_except_type (mapped_event, &event);
  g_assert_cmpuint (mapped_event->button.button, ==, BTN_DPAD_LEFT);

  mapped_event = mapped_events->next->data;
  g_assert_cmpint (mapped_event->any.type, ==, MANETTE_EVENT_BUTTON_RELEASE);
  cmp_event_any_except_type (mapped_event, &event);
  g_assert_cmpuint (mapped_event->button.button, ==, BTN_DPAD_RIGHT);

  g_slist_free_full (mapped_events, (GDestroyNotify) manette_event_free);

  event.hat.value = -1;

  mapped_events = manette_map_event (mapping, &event);
  g_assert_cmpint (g_slist_length (mapped_events), ==, 1);

  mapped_event = mapped_events->data;
  g_assert_cmpint (mapped_event->any.type, ==, MANETTE_EVENT_BUTTON_PRESS);
  cmp_event_any_except_type (mapped_event, &event);
  g_assert_cmpuint (mapped_event->button.button, ==, BTN_DPAD_LEFT);

  g_slist_free_full (mapped_events, (GDestroyNotify) manette_event_free);

  event.hat.value = 1;

  mapped_events = manette_map_event (mapping, &event);
  g_assert_cmpint (g_slist_length (mapped_events), ==, 1);

  mapped_event = mapped_events->data;
  g_assert_cmpint (mapped_event->any.type, ==, MANETTE_EVENT_BUTTON_PRESS);
  cmp_event_any_except_type (mapped_event, &event);
  g_assert_cmpuint (mapped_event->button.button, ==, BTN_DPAD_RIGHT);

  g_slist_free_full (mapped_events, (GDestroyNotify) manette_event_free);
}

static void
test_axis_dpad_mapping (void)
{
  g_autoptr (ManetteMapping) mapping = NULL;
  ManetteEvent event = {};
  GSList *mapped_events;
  ManetteEvent *mapped_event;
  GError *error = NULL;

  mapping = manette_mapping_new (MAPPING_AXIS_DPAD, &error);
  g_assert_no_error (error);
  g_assert_nonnull (mapping);
  g_assert_true (MANETTE_IS_MAPPING (mapping));

  event.any.type = MANETTE_EVENT_ABSOLUTE;
  event.any.time = DUMMY_TIMESTAMP;
  event.any.hardware_type = EV_ABS;
  event.any.hardware_code = ABS_X;
  event.any.hardware_index = 0;
  event.absolute.axis = ABS_RX;
  event.absolute.value = 0.0;

  mapped_events = manette_map_event (mapping, &event);
  g_assert_cmpint (g_slist_length (mapped_events), ==, 2);

  mapped_event = mapped_events->data;
  g_assert_cmpint (mapped_event->any.type, ==, MANETTE_EVENT_BUTTON_RELEASE);
  cmp_event_any_except_type (mapped_event, &event);
  g_assert_cmpuint (mapped_event->button.button, ==, BTN_DPAD_LEFT);

  mapped_event = mapped_events->next->data;
  g_assert_cmpint (mapped_event->any.type, ==, MANETTE_EVENT_BUTTON_RELEASE);
  cmp_event_any_except_type (mapped_event, &event);
  g_assert_cmpuint (mapped_event->button.button, ==, BTN_DPAD_RIGHT);

  g_slist_free_full (mapped_events, (GDestroyNotify) manette_event_free);

  event.any.type = MANETTE_EVENT_ABSOLUTE;
  event.any.time = DUMMY_TIMESTAMP;
  event.any.hardware_type = EV_ABS;
  event.any.hardware_code = ABS_X;
  event.any.hardware_index = 0;
  event.absolute.axis = ABS_RX;
  event.absolute.value = -1.0;

  mapped_events = manette_map_event (mapping, &event);
  g_assert_cmpint (g_slist_length (mapped_events), ==, 1);

  mapped_event = mapped_events->data;
  g_assert_cmpint (mapped_event->any.type, ==, MANETTE_EVENT_BUTTON_PRESS);
  cmp_event_any_except_type (mapped_event, &event);
  g_assert_cmpuint (mapped_event->button.button, ==, BTN_DPAD_LEFT);

  g_slist_free_full (mapped_events, (GDestroyNotify) manette_event_free);

  event.any.type = MANETTE_EVENT_ABSOLUTE;
  event.any.time = DUMMY_TIMESTAMP;
  event.any.hardware_type = EV_ABS;
  event.any.hardware_code = ABS_X;
  event.any.hardware_index = 0;
  event.absolute.axis = ABS_RX;
  event.absolute.value = 1.0;

  mapped_events = manette_map_event (mapping, &event);
  g_assert_cmpint (g_slist_length (mapped_events), ==, 1);

  mapped_event = mapped_events->data;
  g_assert_cmpint (mapped_event->any.type, ==, MANETTE_EVENT_BUTTON_PRESS);
  cmp_event_any_except_type (mapped_event, &event);
  g_assert_cmpuint (mapped_event->button.button, ==, BTN_DPAD_RIGHT);

  g_slist_free_full (mapped_events, (GDestroyNotify) manette_event_free);

  event.any.type = MANETTE_EVENT_ABSOLUTE;
  event.any.time = DUMMY_TIMESTAMP;
  event.any.hardware_type = EV_ABS;
  event.any.hardware_code = ABS_Y;
  event.any.hardware_index = 1;
  event.absolute.axis = ABS_RY;
  event.absolute.value = 0.0;

  mapped_events = manette_map_event (mapping, &event);
  g_assert_cmpint (g_slist_length (mapped_events), ==, 2);

  mapped_event = mapped_events->data;
  g_assert_cmpint (mapped_event->any.type, ==, MANETTE_EVENT_BUTTON_RELEASE);
  cmp_event_any_except_type (mapped_event, &event);
  g_assert_cmpuint (mapped_event->button.button, ==, BTN_DPAD_UP);

  mapped_event = mapped_events->next->data;
  g_assert_cmpint (mapped_event->any.type, ==, MANETTE_EVENT_BUTTON_RELEASE);
  cmp_event_any_except_type (mapped_event, &event);
  g_assert_cmpuint (mapped_event->button.button, ==, BTN_DPAD_DOWN);

  g_slist_free_full (mapped_events, (GDestroyNotify) manette_event_free);

  event.any.type = MANETTE_EVENT_ABSOLUTE;
  event.any.time = DUMMY_TIMESTAMP;
  event.any.hardware_type = EV_ABS;
  event.any.hardware_code = ABS_Y;
  event.any.hardware_index = 1;
  event.absolute.axis = ABS_RY;
  event.absolute.value = -1.0;

  mapped_events = manette_map_event (mapping, &event);
  g_assert_cmpint (g_slist_length (mapped_events), ==, 1);

  mapped_event = mapped_events->data;
  g_assert_cmpint (mapped_event->any.type, ==, MANETTE_EVENT_BUTTON_PRESS);
  cmp_event_any_except_type (mapped_event, &event);
  g_assert_cmpuint (mapped_event->button.button, ==, BTN_DPAD_UP);

  g_slist_free_full (mapped_events, (GDestroyNotify) manette_event_free);

  event.any.type = MANETTE_EVENT_ABSOLUTE;
  event.any.time = DUMMY_TIMESTAMP;
  event.any.hardware_type = EV_ABS;
  event.any.hardware_code = ABS_Y;
  event.any.hardware_index = 1;
  event.absolute.axis = ABS_RY;
  event.absolute.value = 1.0;

  mapped_events = manette_map_event (mapping, &event);
  g_assert_cmpint (g_slist_length (mapped_events), ==, 1);

  mapped_event = mapped_events->data;
  g_assert_cmpint (mapped_event->any.type, ==, MANETTE_EVENT_BUTTON_PRESS);
  cmp_event_any_except_type (mapped_event, &event);
  g_assert_cmpuint (mapped_event->button.button, ==, BTN_DPAD_DOWN);

  g_slist_free_full (mapped_events, (GDestroyNotify) manette_event_free);
}

static void
test_axis_trigger_mapping (void)
{
  g_autoptr (ManetteMapping) mapping = NULL;
  ManetteEvent event = {};
  GSList *mapped_events;
  ManetteEvent *mapped_event;
  GError *error = NULL;

  mapping = manette_mapping_new (MAPPING_AXIS_TRIGGER, &error);
  g_assert_no_error (error);
  g_assert_nonnull (mapping);
  g_assert_true (MANETTE_IS_MAPPING (mapping));

  event.any.type = MANETTE_EVENT_ABSOLUTE;
  event.any.time = DUMMY_TIMESTAMP;
  event.any.hardware_type = EV_ABS;
  event.any.hardware_code = ABS_X;
  event.any.hardware_index = 0;
  event.absolute.axis = ABS_RX;
  event.absolute.value = 1.0;

  mapped_events = manette_map_event (mapping, &event);
  g_assert_cmpint (g_slist_length (mapped_events), ==, 1);

  mapped_event = mapped_events->data;
  g_assert_cmpint (mapped_event->any.type, ==, MANETTE_EVENT_BUTTON_PRESS);
  cmp_event_any_except_type (mapped_event, &event);
  g_assert_cmpuint (mapped_event->button.button, ==, BTN_TL2);

  g_slist_free_full (mapped_events, (GDestroyNotify) manette_event_free);

  event.any.type = MANETTE_EVENT_ABSOLUTE;
  event.any.time = DUMMY_TIMESTAMP;
  event.any.hardware_type = EV_ABS;
  event.any.hardware_code = ABS_X;
  event.any.hardware_index = 0;
  event.absolute.axis = ABS_RX;
  event.absolute.value = -1.0;

  mapped_events = manette_map_event (mapping, &event);
  g_assert_cmpint (g_slist_length (mapped_events), ==, 1);

  mapped_event = mapped_events->data;
  g_assert_cmpint (mapped_event->any.type, ==, MANETTE_EVENT_BUTTON_RELEASE);
  cmp_event_any_except_type (mapped_event, &event);
  g_assert_cmpuint (mapped_event->button.button, ==, BTN_TL2);

  g_slist_free_full (mapped_events, (GDestroyNotify) manette_event_free);

  event.any.type = MANETTE_EVENT_ABSOLUTE;
  event.any.time = DUMMY_TIMESTAMP;
  event.any.hardware_type = EV_ABS;
  event.any.hardware_code = ABS_Y;
  event.any.hardware_index = 1;
  event.absolute.axis = ABS_RY;
  event.absolute.value = 1.0;

  mapped_events = manette_map_event (mapping, &event);
  g_assert_cmpint (g_slist_length (mapped_events), ==, 1);

  mapped_event = mapped_events->data;
  g_assert_cmpint (mapped_event->any.type, ==, MANETTE_EVENT_BUTTON_PRESS);
  cmp_event_any_except_type (mapped_event, &event);
  g_assert_cmpuint (mapped_event->button.button, ==, BTN_TR2);

  g_slist_free_full (mapped_events, (GDestroyNotify) manette_event_free);

  event.any.type = MANETTE_EVENT_ABSOLUTE;
  event.any.time = DUMMY_TIMESTAMP;
  event.any.hardware_type = EV_ABS;
  event.any.hardware_code = ABS_Y;
  event.any.hardware_index = 1;
  event.absolute.axis = ABS_RY;
  event.absolute.value = -1.0;

  mapped_events = manette_map_event (mapping, &event);
  g_assert_cmpint (g_slist_length (mapped_events), ==, 1);

  mapped_event = mapped_events->data;
  g_assert_cmpint (mapped_event->any.type, ==, MANETTE_EVENT_BUTTON_RELEASE);
  cmp_event_any_except_type (mapped_event, &event);
  g_assert_cmpuint (mapped_event->button.button, ==, BTN_TR2);

  g_slist_free_full (mapped_events, (GDestroyNotify) manette_event_free);
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
