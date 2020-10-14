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
#include <linux/input-event-codes.h>

#define CASE_THEN_STRING(x) case x: return #x;

const char *
get_absolute_name (guint16 axis)
{
  switch (axis) {
  CASE_THEN_STRING (ABS_X)
  CASE_THEN_STRING (ABS_Y)
  CASE_THEN_STRING (ABS_RX)
  CASE_THEN_STRING (ABS_RY)
  default:
    return NULL;
  }
}

const char *
get_button_name (guint16 button)
{
  switch (button) {
  CASE_THEN_STRING (BTN_A)
  CASE_THEN_STRING (BTN_B)
  CASE_THEN_STRING (BTN_C)
  CASE_THEN_STRING (BTN_X)
  CASE_THEN_STRING (BTN_Y)
  CASE_THEN_STRING (BTN_Z)
  CASE_THEN_STRING (BTN_TL)
  CASE_THEN_STRING (BTN_TR)
  CASE_THEN_STRING (BTN_TL2)
  CASE_THEN_STRING (BTN_TR2)
  CASE_THEN_STRING (BTN_SELECT)
  CASE_THEN_STRING (BTN_START)
  CASE_THEN_STRING (BTN_MODE)
  CASE_THEN_STRING (BTN_THUMBL)
  CASE_THEN_STRING (BTN_THUMBR)
  CASE_THEN_STRING (BTN_DPAD_UP)
  CASE_THEN_STRING (BTN_DPAD_DOWN)
  CASE_THEN_STRING (BTN_DPAD_LEFT)
  CASE_THEN_STRING (BTN_DPAD_RIGHT)
  default:
    return NULL;
  }
}

const char *
get_hat_name (guint16 axis)
{
  switch (axis) {
  CASE_THEN_STRING (ABS_HAT0X)
  CASE_THEN_STRING (ABS_HAT0Y)
  CASE_THEN_STRING (ABS_HAT1X)
  CASE_THEN_STRING (ABS_HAT1Y)
  CASE_THEN_STRING (ABS_HAT2X)
  CASE_THEN_STRING (ABS_HAT2Y)
  CASE_THEN_STRING (ABS_HAT3X)
  CASE_THEN_STRING (ABS_HAT3Y)
  default:
    return NULL;
  }
}

static void
device_disconnected_cb (ManetteDevice *emitter,
                        gpointer       user_data)
{
  g_printf ("%s: disconnected\n", manette_device_get_name (emitter));
  g_object_unref (emitter);
}

static void
absolute_axis_event_cb (ManetteDevice *emitter,
                        ManetteEvent  *event,
                        gpointer       user_data)
{
  ManetteDevice *device;
  const gchar *device_name;
  const gchar *absolute_axis_name;
  guint16 absolute_axis;
  gdouble value;

  if (!manette_event_get_absolute (event, &absolute_axis, &value))
    return;

  device = manette_event_get_device (event);
  device_name = manette_device_get_name (device);
  absolute_axis_name = get_absolute_name (absolute_axis);

  if (absolute_axis_name != NULL)
    g_printf ("%s: Absolute axis %s moved to %lf\n", device_name, absolute_axis_name, value);
  else
    g_printf ("%s: Unknown absolute axis %u moved to %lf\n", device_name, absolute_axis, value);
}

static void
button_press_event_cb (ManetteDevice *emitter,
                       ManetteEvent  *event,
                       gpointer       user_data)
{
  ManetteDevice *device;
  const gchar *device_name;
  const gchar *button_name;
  guint16 button;

  if (!manette_event_get_button (event, &button))
    return;

  device = manette_event_get_device (event);
  device_name = manette_device_get_name (device);
  button_name = get_button_name (button);

  if (button_name != NULL)
    g_printf ("%s: Button %s pressed\n", device_name, button_name);
  else
    g_printf ("%s: Unknown button %u pressed\n", device_name, button);
}

static void
button_release_event_cb (ManetteDevice *emitter,
                         ManetteEvent  *event,
                         gpointer       user_data)
{
  ManetteDevice *device;
  const gchar *device_name;
  const gchar *button_name;
  guint16 button;

  if (!manette_event_get_button (event, &button))
    return;

  device = manette_event_get_device (event);
  device_name = manette_device_get_name (device);
  button_name = get_button_name (button);

  if (button_name != NULL)
    g_printf ("%s: Button %s released\n", device_name, button_name);
  else
    g_printf ("%s: Unknown button %u released\n", device_name, button);
}

static void
hat_axis_event_cb (ManetteDevice *emitter,
                   ManetteEvent  *event,
                   gpointer       user_data)
{
  ManetteDevice *device;
  const gchar *device_name;
  const gchar *hat_axis_name;
  guint16 hat_axis;
  gint8 value;

  if (!manette_event_get_hat (event, &hat_axis, &value))
    return;

  device = manette_event_get_device (event);
  device_name = manette_device_get_name (device);
  hat_axis_name = get_hat_name (hat_axis);

  if (hat_axis_name != NULL)
    g_printf ("%s: Hat axis %s moved to %d\n", device_name, hat_axis_name, value);
  else
    g_printf ("%s: Unknown hat axis %u moved to %d\n", device_name, hat_axis, value);
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
                           "absolute-axis-event",
                           (GCallback) absolute_axis_event_cb,
                           NULL,
                           0);
  g_signal_connect_object (G_OBJECT (device),
                           "button-press-event",
                           (GCallback) button_press_event_cb,
                           NULL,
                           0);
  g_signal_connect_object (G_OBJECT (device),
                           "button-release-event",
                           (GCallback) button_release_event_cb,
                           NULL,
                           0);
  g_signal_connect_object (G_OBJECT (device),
                           "hat-axis-event",
                           (GCallback) hat_axis_event_cb,
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
  g_autoptr (ManetteMonitorIter) iter = NULL;
  ManetteDevice *device;

  monitor = manette_monitor_new ();
  g_signal_connect_object (G_OBJECT (monitor),
                           "device-connected",
                           (GCallback) device_connected_cb,
                           NULL,
                           0);

  iter = manette_monitor_iterate (monitor);
  while (manette_monitor_iter_next (iter, &device))
    listen_to_device (device);

  main_loop = g_main_loop_new (NULL, FALSE);
  g_main_loop_run (main_loop);

  return 0;
}
