/* test-mapping.c
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

#include "../src/manette-mapping-private.h"

#define MAPPING_STEAM_CONTROLLER "03000000de280000fc11000001000000,Steam Controller,a:b0,b:b1,back:b6,dpdown:h0.4,dpleft:h0.8,dpright:h0.2,dpup:h0.1,guide:b8,leftshoulder:b4,leftstick:b9,lefttrigger:a2,leftx:a0,lefty:a1,rightshoulder:b5,rightstick:b10,righttrigger:a5,rightx:a3,righty:a4,start:b7,x:b2,y:b3,"
#define MAPPING_BUTTON "00000000000000000000000000000000,button,a:b0,b:b1,x:b2,y:b3,"
#define MAPPING_AXIS "00000000000000000000000000000000,axis,leftx:a0,lefty:a1,-rightx:-a2,+rightx:+a2,-righty:+a3~,+righty:-a3~,"
#define MAPPING_HAT "00000000000000000000000000000000,hat,dpleft:h0.8,dpright:h0.2,dpup:h0.1,dpdown:h0.4,"

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
test_empty (void)
{
  ManetteMapping *mapping;
  g_autoptr (GError) error = NULL;

  mapping = manette_mapping_new ("", &error);
  g_assert_error (error,
                  MANETTE_MAPPING_ERROR,
                  MANETTE_MAPPING_ERROR_NOT_A_MAPPING);

  g_assert_null (mapping);
}

static void
test_valid (void)
{
  g_autoptr (ManetteMapping) mapping = NULL;
  GError *error = NULL;

  mapping = manette_mapping_new (MAPPING_STEAM_CONTROLLER, &error);
  g_assert_no_error (error);
  g_assert_nonnull (mapping);
  g_assert_true (MANETTE_IS_MAPPING (mapping));
}

static void
test_button_bindings (void)
{
  g_autoptr (ManetteMapping) mapping = NULL;
  const ManetteMappingBinding * const *bindings;
  const ManetteMappingBinding *binding;
  GError *error = NULL;

  mapping = manette_mapping_new (MAPPING_BUTTON, &error);
  g_assert_no_error (error);
  g_assert_nonnull (mapping);
  g_assert_true (MANETTE_IS_MAPPING (mapping));

  bindings = manette_mapping_get_bindings (mapping,
                                           MANETTE_MAPPING_INPUT_TYPE_BUTTON,
                                           0);

  g_assert_nonnull (bindings);
  g_assert_nonnull (bindings[0]);
  g_assert_null (bindings[1]);

  binding = bindings[0];
  g_assert_cmpint (binding->source.type, ==, MANETTE_MAPPING_INPUT_TYPE_BUTTON);
  g_assert_cmpint (binding->source.index, ==, 0);
  g_assert_cmpint (binding->source.range, ==, MANETTE_MAPPING_RANGE_FULL);
  g_assert_false (binding->source.invert);
  g_assert_cmpint (binding->destination.type, ==, MANETTE_MAPPING_DESTINATION_TYPE_BUTTON);
  g_assert_cmpint (binding->destination.code, ==, MANETTE_BUTTON_SOUTH);
  g_assert_cmpint (binding->destination.range, ==, MANETTE_MAPPING_RANGE_FULL);

  bindings = manette_mapping_get_bindings (mapping,
                                           MANETTE_MAPPING_INPUT_TYPE_BUTTON,
                                           1);

  g_assert_nonnull (bindings);
  g_assert_nonnull (bindings[0]);
  g_assert_null (bindings[1]);

  binding = bindings[0];
  g_assert_cmpint (binding->source.type, ==, MANETTE_MAPPING_INPUT_TYPE_BUTTON);
  g_assert_cmpint (binding->source.index, ==, 1);
  g_assert_cmpint (binding->source.range, ==, MANETTE_MAPPING_RANGE_FULL);
  g_assert_false (binding->source.invert);
  g_assert_cmpint (binding->destination.type, ==, MANETTE_MAPPING_DESTINATION_TYPE_BUTTON);
  g_assert_cmpint (binding->destination.code, ==, MANETTE_BUTTON_EAST);
  g_assert_cmpint (binding->destination.range, ==, MANETTE_MAPPING_RANGE_FULL);

  bindings = manette_mapping_get_bindings (mapping,
                                           MANETTE_MAPPING_INPUT_TYPE_BUTTON,
                                           2);

  g_assert_nonnull (bindings);
  g_assert_nonnull (bindings[0]);
  g_assert_null (bindings[1]);

  binding = bindings[0];
  g_assert_cmpint (binding->source.type, ==, MANETTE_MAPPING_INPUT_TYPE_BUTTON);
  g_assert_cmpint (binding->source.index, ==, 2);
  g_assert_cmpint (binding->source.range, ==, MANETTE_MAPPING_RANGE_FULL);
  g_assert_false (binding->source.invert);
  g_assert_cmpint (binding->destination.type, ==, MANETTE_MAPPING_DESTINATION_TYPE_BUTTON);
  g_assert_cmpint (binding->destination.code, ==, MANETTE_BUTTON_WEST);
  g_assert_cmpint (binding->destination.range, ==, MANETTE_MAPPING_RANGE_FULL);

  bindings = manette_mapping_get_bindings (mapping,
                                           MANETTE_MAPPING_INPUT_TYPE_BUTTON,
                                           3);

  g_assert_nonnull (bindings);
  g_assert_nonnull (bindings[0]);
  g_assert_null (bindings[1]);

  binding = bindings[0];
  g_assert_cmpint (binding->source.type, ==, MANETTE_MAPPING_INPUT_TYPE_BUTTON);
  g_assert_cmpint (binding->source.index, ==, 3);
  g_assert_cmpint (binding->source.range, ==, MANETTE_MAPPING_RANGE_FULL);
  g_assert_false (binding->source.invert);
  g_assert_cmpint (binding->destination.type, ==, MANETTE_MAPPING_DESTINATION_TYPE_BUTTON);
  g_assert_cmpint (binding->destination.code, ==, MANETTE_BUTTON_NORTH);
  g_assert_cmpint (binding->destination.range, ==, MANETTE_MAPPING_RANGE_FULL);
}

static void
test_axis_bindings (void)
{
  g_autoptr (ManetteMapping) mapping = NULL;
  const ManetteMappingBinding * const *bindings;
  const ManetteMappingBinding *binding;
  GError *error = NULL;

  mapping = manette_mapping_new (MAPPING_AXIS, &error);
  g_assert_no_error (error);
  g_assert_nonnull (mapping);
  g_assert_true (MANETTE_IS_MAPPING (mapping));

  bindings = manette_mapping_get_bindings (mapping,
                                           MANETTE_MAPPING_INPUT_TYPE_AXIS,
                                           0);

  g_assert_nonnull (bindings);
  g_assert_nonnull (bindings[0]);
  g_assert_null (bindings[1]);

  binding = bindings[0];
  g_assert_cmpint (binding->source.type, ==, MANETTE_MAPPING_INPUT_TYPE_AXIS);
  g_assert_cmpint (binding->source.index, ==, 0);
  g_assert_cmpint (binding->source.range, ==, MANETTE_MAPPING_RANGE_FULL);
  g_assert_false (binding->source.invert);
  g_assert_cmpint (binding->destination.type, ==, MANETTE_MAPPING_DESTINATION_TYPE_AXIS);
  g_assert_cmpint (binding->destination.code, ==, MANETTE_AXIS_LEFT_X);
  g_assert_cmpint (binding->destination.range, ==, MANETTE_MAPPING_RANGE_FULL);

  bindings = manette_mapping_get_bindings (mapping,
                                           MANETTE_MAPPING_INPUT_TYPE_AXIS,
                                           1);

  g_assert_nonnull (bindings);
  g_assert_nonnull (bindings[0]);
  g_assert_null (bindings[1]);

  binding = bindings[0];
  g_assert_cmpint (binding->source.type, ==, MANETTE_MAPPING_INPUT_TYPE_AXIS);
  g_assert_cmpint (binding->source.index, ==, 1);
  g_assert_cmpint (binding->source.range, ==, MANETTE_MAPPING_RANGE_FULL);
  g_assert_false (binding->source.invert);
  g_assert_cmpint (binding->destination.type, ==, MANETTE_MAPPING_DESTINATION_TYPE_AXIS);
  g_assert_cmpint (binding->destination.code, ==, MANETTE_AXIS_LEFT_Y);
  g_assert_cmpint (binding->destination.range, ==, MANETTE_MAPPING_RANGE_FULL);
}

static void
test_axis_range_bindings (void)
{
  g_autoptr (ManetteMapping) mapping = NULL;
  const ManetteMappingBinding * const *bindings;
  const ManetteMappingBinding *binding;
  GError *error = NULL;

  mapping = manette_mapping_new (MAPPING_AXIS, &error);
  g_assert_no_error (error);
  g_assert_nonnull (mapping);
  g_assert_true (MANETTE_IS_MAPPING (mapping));

  bindings = manette_mapping_get_bindings (mapping,
                                           MANETTE_MAPPING_INPUT_TYPE_AXIS,
                                           2);

  g_assert_nonnull (bindings);
  g_assert_nonnull (bindings[0]);
  g_assert_nonnull (bindings[1]);
  g_assert_null (bindings[2]);

  binding = bindings[0];
  g_assert_cmpint (binding->source.type, ==, MANETTE_MAPPING_INPUT_TYPE_AXIS);
  g_assert_cmpint (binding->source.index, ==, 2);
  g_assert_cmpint (binding->source.range, ==, MANETTE_MAPPING_RANGE_NEGATIVE);
  g_assert_false (binding->source.invert);
  g_assert_cmpint (binding->destination.type, ==, MANETTE_MAPPING_DESTINATION_TYPE_AXIS);
  g_assert_cmpint (binding->destination.code, ==, MANETTE_AXIS_RIGHT_X);
  g_assert_cmpint (binding->destination.range, ==, MANETTE_MAPPING_RANGE_NEGATIVE);

  binding = bindings[1];
  g_assert_cmpint (binding->source.type, ==, MANETTE_MAPPING_INPUT_TYPE_AXIS);
  g_assert_cmpint (binding->source.index, ==, 2);
  g_assert_cmpint (binding->source.range, ==, MANETTE_MAPPING_RANGE_POSITIVE);
  g_assert_false (binding->source.invert);
  g_assert_cmpint (binding->destination.type, ==, MANETTE_MAPPING_DESTINATION_TYPE_AXIS);
  g_assert_cmpint (binding->destination.code, ==, MANETTE_AXIS_RIGHT_X);
  g_assert_cmpint (binding->destination.range, ==, MANETTE_MAPPING_RANGE_POSITIVE);

  bindings = manette_mapping_get_bindings (mapping,
                                           MANETTE_MAPPING_INPUT_TYPE_AXIS,
                                           3);

  g_assert_nonnull (bindings);
  g_assert_nonnull (bindings[0]);
  g_assert_nonnull (bindings[1]);
  g_assert_null (bindings[2]);

  binding = bindings[0];
  g_assert_cmpint (binding->source.type, ==, MANETTE_MAPPING_INPUT_TYPE_AXIS);
  g_assert_cmpint (binding->source.index, ==, 3);
  g_assert_cmpint (binding->source.range, ==, MANETTE_MAPPING_RANGE_POSITIVE);
  g_assert_true (binding->source.invert);
  g_assert_cmpint (binding->destination.type, ==, MANETTE_MAPPING_DESTINATION_TYPE_AXIS);
  g_assert_cmpint (binding->destination.code, ==, MANETTE_AXIS_RIGHT_Y);
  g_assert_cmpint (binding->destination.range, ==, MANETTE_MAPPING_RANGE_NEGATIVE);

  binding = bindings[1];
  g_assert_cmpint (binding->source.type, ==, MANETTE_MAPPING_INPUT_TYPE_AXIS);
  g_assert_cmpint (binding->source.index, ==, 3);
  g_assert_cmpint (binding->source.range, ==, MANETTE_MAPPING_RANGE_NEGATIVE);
  g_assert_true (binding->source.invert);
  g_assert_cmpint (binding->destination.type, ==, MANETTE_MAPPING_DESTINATION_TYPE_AXIS);
  g_assert_cmpint (binding->destination.code, ==, MANETTE_AXIS_RIGHT_Y);
  g_assert_cmpint (binding->destination.range, ==, MANETTE_MAPPING_RANGE_POSITIVE);
}

static void
test_hat_x_bindings (void)
{
  g_autoptr (ManetteMapping) mapping = NULL;
  const ManetteMappingBinding * const *bindings;
  const ManetteMappingBinding *binding;
  GError *error = NULL;

  mapping = manette_mapping_new (MAPPING_HAT, &error);
  g_assert_no_error (error);
  g_assert_nonnull (mapping);
  g_assert_true (MANETTE_IS_MAPPING (mapping));

  bindings = manette_mapping_get_bindings (mapping,
                                           MANETTE_MAPPING_INPUT_TYPE_HAT,
                                           0);

  g_assert_nonnull (bindings);
  g_assert_nonnull (bindings[0]);
  g_assert_nonnull (bindings[1]);
  g_assert_null (bindings[2]);

  binding = bindings[0];
  g_assert_cmpint (binding->source.type, ==, MANETTE_MAPPING_INPUT_TYPE_HAT);
  g_assert_cmpint (binding->source.index, ==, 0);
  g_assert_cmpint (binding->source.range, ==, MANETTE_MAPPING_RANGE_NEGATIVE);
  g_assert_true (binding->source.invert);
  g_assert_cmpint (binding->destination.type, ==, MANETTE_MAPPING_DESTINATION_TYPE_BUTTON);
  g_assert_cmpint (binding->destination.code, ==, MANETTE_BUTTON_DPAD_LEFT);
  g_assert_cmpint (binding->destination.range, ==, MANETTE_MAPPING_RANGE_FULL);

  binding = bindings[1];
  g_assert_cmpint (binding->source.type, ==, MANETTE_MAPPING_INPUT_TYPE_HAT);
  g_assert_cmpint (binding->source.index, ==, 0);
  g_assert_cmpint (binding->source.range, ==, MANETTE_MAPPING_RANGE_POSITIVE);
  g_assert_false (binding->source.invert);
  g_assert_cmpint (binding->destination.type, ==, MANETTE_MAPPING_DESTINATION_TYPE_BUTTON);
  g_assert_cmpint (binding->destination.code, ==, MANETTE_BUTTON_DPAD_RIGHT);
  g_assert_cmpint (binding->destination.range, ==, MANETTE_MAPPING_RANGE_FULL);
}

static void
test_hat_y_bindings (void)
{
  g_autoptr (ManetteMapping) mapping = NULL;
  const ManetteMappingBinding * const *bindings;
  const ManetteMappingBinding *binding;
  GError *error = NULL;

  mapping = manette_mapping_new (MAPPING_HAT, &error);
  g_assert_no_error (error);
  g_assert_nonnull (mapping);
  g_assert_true (MANETTE_IS_MAPPING (mapping));

  bindings = manette_mapping_get_bindings (mapping,
                                           MANETTE_MAPPING_INPUT_TYPE_HAT,
                                           1);

  g_assert_nonnull (bindings);
  g_assert_nonnull (bindings[0]);
  g_assert_nonnull (bindings[1]);
  g_assert_null (bindings[2]);

  binding = bindings[0];
  g_assert_cmpint (binding->source.type, ==, MANETTE_MAPPING_INPUT_TYPE_HAT);
  g_assert_cmpint (binding->source.index, ==, 1);
  g_assert_cmpint (binding->source.range, ==, MANETTE_MAPPING_RANGE_NEGATIVE);
  g_assert_true (binding->source.invert);
  g_assert_cmpint (binding->destination.type, ==, MANETTE_MAPPING_DESTINATION_TYPE_BUTTON);
  g_assert_cmpint (binding->destination.code, ==, MANETTE_BUTTON_DPAD_UP);
  g_assert_cmpint (binding->destination.range, ==, MANETTE_MAPPING_RANGE_FULL);

  binding = bindings[1];
  g_assert_cmpint (binding->source.type, ==, MANETTE_MAPPING_INPUT_TYPE_HAT);
  g_assert_cmpint (binding->source.index, ==, 1);
  g_assert_cmpint (binding->source.range, ==, MANETTE_MAPPING_RANGE_POSITIVE);
  g_assert_false (binding->source.invert);
  g_assert_cmpint (binding->destination.type, ==, MANETTE_MAPPING_DESTINATION_TYPE_BUTTON);
  g_assert_cmpint (binding->destination.code, ==, MANETTE_BUTTON_DPAD_DOWN);
  g_assert_cmpint (binding->destination.range, ==, MANETTE_MAPPING_RANGE_FULL);
}

static void
test_has_destination_input (void)
{
  g_autoptr (ManetteMapping) mapping = NULL;
  GError *error = NULL;

  mapping = manette_mapping_new (MAPPING_STEAM_CONTROLLER, &error);
  g_assert_no_error (error);
  g_assert_nonnull (mapping);
  g_assert_true (MANETTE_IS_MAPPING (mapping));

  g_assert_true (manette_mapping_has_destination_button (mapping, MANETTE_BUTTON_SOUTH));
  g_assert_true (manette_mapping_has_destination_button (mapping, MANETTE_BUTTON_EAST));
  g_assert_true (manette_mapping_has_destination_button (mapping, MANETTE_BUTTON_NORTH));
  g_assert_true (manette_mapping_has_destination_button (mapping, MANETTE_BUTTON_WEST));
  g_assert_true (manette_mapping_has_destination_button (mapping, MANETTE_BUTTON_LEFT_SHOULDER));
  g_assert_true (manette_mapping_has_destination_button (mapping, MANETTE_BUTTON_RIGHT_SHOULDER));
  g_assert_true (manette_mapping_has_destination_button (mapping, MANETTE_BUTTON_SELECT));
  g_assert_true (manette_mapping_has_destination_button (mapping, MANETTE_BUTTON_START));
  g_assert_true (manette_mapping_has_destination_button (mapping, MANETTE_BUTTON_MODE));
  g_assert_true (manette_mapping_has_destination_button (mapping, MANETTE_BUTTON_LEFT_STICK));
  g_assert_true (manette_mapping_has_destination_button (mapping, MANETTE_BUTTON_RIGHT_STICK));
  g_assert_true (manette_mapping_has_destination_button (mapping, MANETTE_BUTTON_DPAD_UP));
  g_assert_true (manette_mapping_has_destination_button (mapping, MANETTE_BUTTON_DPAD_DOWN));
  g_assert_true (manette_mapping_has_destination_button (mapping, MANETTE_BUTTON_DPAD_LEFT));
  g_assert_true (manette_mapping_has_destination_button (mapping, MANETTE_BUTTON_DPAD_RIGHT));

  g_assert_true (manette_mapping_has_destination_axis (mapping, MANETTE_AXIS_LEFT_X));
  g_assert_true (manette_mapping_has_destination_axis (mapping, MANETTE_AXIS_LEFT_Y));
  g_assert_true (manette_mapping_has_destination_axis (mapping, MANETTE_AXIS_RIGHT_X));
  g_assert_true (manette_mapping_has_destination_axis (mapping, MANETTE_AXIS_RIGHT_Y));
  g_assert_true (manette_mapping_has_destination_axis (mapping, MANETTE_AXIS_LEFT_TRIGGER));
  g_assert_true (manette_mapping_has_destination_axis (mapping, MANETTE_AXIS_RIGHT_TRIGGER));
}

int
main (int   argc,
      char *argv[])
{
  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/ManetteMapping/test_null", test_null);
  g_test_add_func ("/ManetteMapping/test_empty", test_empty);
  g_test_add_func ("/ManetteMapping/test_valid", test_valid);
  g_test_add_func ("/ManetteMapping/test_button_bindings", test_button_bindings);
  g_test_add_func ("/ManetteMapping/test_axis_bindings", test_axis_bindings);
  g_test_add_func ("/ManetteMapping/test_axis_range_bindings", test_axis_range_bindings);
  g_test_add_func ("/ManetteMapping/test_hat_x_bindings", test_hat_x_bindings);
  g_test_add_func ("/ManetteMapping/test_hat_y_bindings", test_hat_y_bindings);
  g_test_add_func ("/ManetteMapping/test_has_destination_input", test_has_destination_input);

  return g_test_run();
}
