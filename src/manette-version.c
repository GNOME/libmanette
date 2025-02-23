/*
 * Copyright (C) 2025 Alice Mikhaylenko <alicem@gnome.org>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include "config.h"

#include "manette-version.h"

/**
 * manette_get_major_version:
 *
 * Returns the major version number of the libmanette library.
 *
 * For example, in libmanette version 1.2.3 this is 1.
 *
 * This function is in the library, so it represents the libmanette library your
 * code is running against. Contrast with the [const@MAJOR_VERSION] constant,
 * which represents the major version of the libmanette headers you have
 * included when compiling your code.
 *
 * Returns: the major version number of the libmanette library
 */
guint
manette_get_major_version (void)
{
  return MANETTE_MAJOR_VERSION;
}

/**
 * manette_get_minor_version:
 *
 * Returns the minor version number of the libmanette library.
 *
 * For example, in libmanette version 1.2.3 this is 2.
 *
 * This function is in the library, so it represents the libmanette library your
 * code is running against. Contrast with the [const@MAJOR_VERSION] constant,
 * which represents the minor version of the libmanette headers you have
 * included when compiling your code.
 *
 * Returns: the minor version number of the libmanette library
 */
guint
manette_get_minor_version (void)
{
  return MANETTE_MINOR_VERSION;
}

/**
 * manette_get_micro_version:
 *
 * Returns the micro version number of the libmanette library.
 *
 * For example, in libmanette version 1.2.3 this is 3.
 *
 * This function is in the library, so it represents the libmanette library your
 * code is running against. Contrast with the [const@MAJOR_VERSION] constant,
 * which represents the micro version of the libmanette headers you have
 * included when compiling your code.
 *
 * Returns: the micro version number of the libmanette library
 */
guint
manette_get_micro_version (void)
{
  return MANETTE_MICRO_VERSION;
}
