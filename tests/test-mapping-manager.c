/* test-mapping-manager.c
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

#include "../src/manette-mapping-manager-private.h"

static void
test_valid (void)
{
  g_autoptr (ManetteMappingManager) mapping_manager = NULL;

  mapping_manager = manette_mapping_manager_new ();
  g_assert_nonnull (mapping_manager);
  g_assert_true (MANETTE_IS_MAPPING_MANAGER (mapping_manager));
}

int
main (int   argc,
      char *argv[])
{
  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/ManetteMappingManager/test_valid", test_valid);

  return g_test_run();
}
