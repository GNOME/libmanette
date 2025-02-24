/* manette-test.c
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

#include <glib/gprintf.h>
#include <libmanette.h>

char *
get_absolute_name (ManetteAxis axis)
{
  GEnumClass *enum_class;
  GEnumValue *value;

  if (axis < 0 || axis > MANETTE_AXIS_RIGHT_Y)
    return NULL;

  enum_class = g_type_class_ref (MANETTE_TYPE_AXIS);
  value = g_enum_get_value (enum_class, axis);

  return g_strdup (value->value_nick);
}

char *
get_button_name (ManetteButton button)
{
  GEnumClass *enum_class;
  GEnumValue *value;

  if (button < 0 || button > MANETTE_BUTTON_TOUCHPAD)
    return NULL;

  enum_class = g_type_class_ref (MANETTE_TYPE_BUTTON);
  value = g_enum_get_value (enum_class, button);

  return g_strdup (value->value_nick);
}

static void
device_disconnected_cb (ManetteDevice *emitter,
                        gpointer       user_data)
{
  g_printf ("%s: disconnected\n", manette_device_get_name (emitter));
  g_object_unref (emitter);
}

static void
absolute_axis_changed_cb (ManetteDevice *emitter,
                          ManetteAxis    axis,
                          double         value,
                          gpointer       user_data)
{
  g_autofree char *axis_name = get_absolute_name (axis);
  const char *device_name = manette_device_get_name (emitter);

  if (axis_name != NULL)
    g_printf ("%s: Absolute axis %s moved to %lf\n", device_name, axis_name, value);
  else
    g_printf ("%s: Unknown absolute axis %u moved to %lf\n", device_name, axis, value);
}

static void
button_pressed_cb (ManetteDevice *emitter,
                   guint          button,
                   gpointer       user_data)
{
  g_autofree char *button_name = get_button_name (button);
  const char *device_name = manette_device_get_name (emitter);

  if (button_name != NULL)
    g_printf ("%s: Button %s pressed\n", device_name, button_name);
  else
    g_printf ("%s: Unknown button %u pressed\n", device_name, button);
}

static void
button_released_cb (ManetteDevice *emitter,
                    guint          button,
                    gpointer       user_data)
{
  const char *device_name;
  const char *button_name;

  device_name = manette_device_get_name (emitter);
  button_name = get_button_name (button);

  if (button_name != NULL)
    g_printf ("%s: Button %s released\n", device_name, button_name);
  else
    g_printf ("%s: Unknown button %u released\n", device_name, button);
}

#define PHASES 4

typedef struct {
  ManetteDevice *device;
  guint8 phase;
} RumbleData;

typedef struct {
  guint16 strong_magnitude;
  guint16 weak_magnitude;
  guint16 duration_ms;
  guint16 wait_ms;
} RumblePhase;

gboolean
rumble (RumbleData *data)
{
  static const RumblePhase phases[] = {
    { G_MAXUINT16/16, G_MAXUINT16/2, 200, 300 },
    { G_MAXUINT16/16, G_MAXUINT16/2, 200, 1300 },
    { G_MAXUINT16/8, G_MAXUINT16/16, 200, 300 },
    { G_MAXUINT16/8, G_MAXUINT16/16, 200, 1800 },
  };

  if (!MANETTE_IS_DEVICE (data->device))
    return FALSE;

  manette_device_rumble (data->device,
                         phases[data->phase].strong_magnitude,
                         phases[data->phase].weak_magnitude,
                         phases[data->phase].duration_ms);
  g_timeout_add (phases[data->phase].wait_ms, (GSourceFunc) rumble, data);
  data->phase = (data->phase + 1) % PHASES;

  return FALSE;
}

static void
listen_to_device (ManetteDevice *device)
{
  RumbleData *rumble_data;

  g_printf ("%s: connected\n", manette_device_get_name (device));
  g_signal_connect_object (G_OBJECT (device),
                           "disconnected",
                           (GCallback) device_disconnected_cb,
                           NULL,
                           0);
  g_signal_connect_object (G_OBJECT (device),
                           "absolute-axis-changed",
                           (GCallback) absolute_axis_changed_cb,
                           NULL,
                           0);
  g_signal_connect_object (G_OBJECT (device),
                           "button-pressed",
                           (GCallback) button_pressed_cb,
                           NULL,
                           0);
  g_signal_connect_object (G_OBJECT (device),
                           "button-released",
                           (GCallback) button_released_cb,
                           NULL,
                           0);
  if (manette_device_has_rumble (device)) {
    rumble_data = g_new0 (RumbleData, 1);
    rumble_data->device = g_object_ref (device);
    rumble (rumble_data);
  }
}

static void
device_connected_cb (ManetteMonitor *emitter,
                     ManetteDevice  *device,
                     gpointer        user_data)
{
  listen_to_device (device);
}

int
main (int    argc,
      char **argv)
{
  g_autoptr (GMainLoop) main_loop = NULL;
  g_autoptr (ManetteMonitor) monitor = NULL;
  g_autofree ManetteDevice **devices = NULL;
  gsize n_devices, i;

  monitor = manette_monitor_new ();
  g_signal_connect_object (G_OBJECT (monitor),
                           "device-connected",
                           (GCallback) device_connected_cb,
                           NULL,
                           0);

  devices = manette_monitor_list_devices (monitor, &n_devices);
  for (i = 0; i < n_devices; i++)
    listen_to_device (devices[i]);

  main_loop = g_main_loop_new (NULL, FALSE);
  g_main_loop_run (main_loop);

  return 0;
}
