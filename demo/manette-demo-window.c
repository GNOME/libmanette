/* manette-demo-window.c
 *
 * Copyright (C) 2025 Alice Mikhaylenko <alicem@gnome.org>
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

#include "manette-demo-window.h"

#include <glib/gi18n.h>
#include <libmanette.h>

#include "manette-demo-axis-bar.h"

// TODO have this in API
#define N_AXES (MANETTE_AXIS_RIGHT_TRIGGER + 1)
#define N_BUTTONS (MANETTE_BUTTON_TOUCHPAD + 1)

struct _ManetteDemoWindow
{
  AdwApplicationWindow parent_instance;

  GtkStack *empty_stack;
  AdwNavigationSplitView *split_view;
  GtkListBox *devices_list;
  AdwNavigationPage *device_page;
  AdwActionRow *name_row;
  AdwActionRow *guid_row;
  AdwActionRow *device_type_row;
  GtkWidget *rumble_group;
  GtkAdjustment *strong_magnitude_adj;
  GtkAdjustment *weak_magnitude_adj;
  GtkAdjustment *duration_adj;
  GtkWidget *buttons_group;
  GtkFlowBox *buttons_flowbox;
  GtkWidget *axes_group;
  GtkFlowBox *axes_flowbox;

  ManetteMonitor *monitor;
  ManetteDevice *selected_device;

  GtkWidget *button_cells[N_BUTTONS];
  GtkWidget *button_indicators[N_BUTTONS];
  GtkWidget *axis_cells[N_AXES];
  ManetteDemoAxisBar *axis_bars[N_AXES];
  GtkLabel *axis_labels[N_AXES];
};

G_DEFINE_FINAL_TYPE (ManetteDemoWindow, manette_demo_window, ADW_TYPE_APPLICATION_WINDOW)

static char *
get_device_type_name (ManetteDeviceType device_type)
{
  switch (device_type) {
  case MANETTE_DEVICE_GENERIC:
    return g_strdup (_("Generic"));
  case MANETTE_DEVICE_STEAM_DECK:
    return g_strdup (_("Steam Deck"));
  default:
    g_assert_not_reached ();
  }
}

static char *
get_button_name (ManetteButton button)
{
  switch (button) {
  case MANETTE_BUTTON_DPAD_UP:
    return g_strdup (_("D-Pad ↑"));
  case MANETTE_BUTTON_DPAD_DOWN:
    return g_strdup (_("D-Pad ↓"));
  case MANETTE_BUTTON_DPAD_LEFT:
    return g_strdup (_("D-Pad ←"));
  case MANETTE_BUTTON_DPAD_RIGHT:
    return g_strdup (_("D-Pad →"));
  case MANETTE_BUTTON_NORTH:
    return g_strdup (_("North"));
  case MANETTE_BUTTON_SOUTH:
    return g_strdup (_("South"));
  case MANETTE_BUTTON_WEST:
    return g_strdup (_("West"));
  case MANETTE_BUTTON_EAST:
    return g_strdup (_("East"));
  case MANETTE_BUTTON_SELECT:
    return g_strdup (_("Select"));
  case MANETTE_BUTTON_START:
    return g_strdup (_("Start"));
  case MANETTE_BUTTON_MODE:
    return g_strdup (_("Mode"));
  case MANETTE_BUTTON_LEFT_SHOULDER:
    return g_strdup (_("Left Shoulder"));
  case MANETTE_BUTTON_RIGHT_SHOULDER:
    return g_strdup (_("Right Shoulder"));
  case MANETTE_BUTTON_LEFT_STICK:
    return g_strdup (_("Left Stick"));
  case MANETTE_BUTTON_RIGHT_STICK:
    return g_strdup (_("Right Stick"));
  case MANETTE_BUTTON_LEFT_PADDLE1:
    return g_strdup (_("Left Paddle 1"));
  case MANETTE_BUTTON_LEFT_PADDLE2:
    return g_strdup (_("Left Paddle 2"));
  case MANETTE_BUTTON_RIGHT_PADDLE1:
    return g_strdup (_("Right Paddle 1"));
  case MANETTE_BUTTON_RIGHT_PADDLE2:
    return g_strdup (_("Right Paddle 2"));
  case MANETTE_BUTTON_MISC1:
    return g_strdup (_("Misc 1"));
  case MANETTE_BUTTON_MISC2:
    return g_strdup (_("Misc 2"));
  case MANETTE_BUTTON_MISC3:
    return g_strdup (_("Misc 3"));
  case MANETTE_BUTTON_MISC4:
    return g_strdup (_("Misc 4"));
  case MANETTE_BUTTON_MISC5:
    return g_strdup (_("Misc 5"));
  case MANETTE_BUTTON_MISC6:
    return g_strdup (_("Misc 6"));
  case MANETTE_BUTTON_TOUCHPAD:
    return g_strdup (_("Touchpad"));
  default:
    g_assert_not_reached ();
  }
}

static char *
get_axis_name (ManetteAxis axis)
{
  switch (axis) {
  case MANETTE_AXIS_LEFT_X:
    return g_strdup (_("Left X"));
  case MANETTE_AXIS_LEFT_Y:
    return g_strdup (_("Left Y"));
  case MANETTE_AXIS_RIGHT_X:
    return g_strdup (_("Right X"));
  case MANETTE_AXIS_RIGHT_Y:
    return g_strdup (_("Right Y"));
  case MANETTE_AXIS_LEFT_TRIGGER:
    return g_strdup (_("Left Trigger"));
  case MANETTE_AXIS_RIGHT_TRIGGER:
    return g_strdup (_("Right Trigger"));
  default:
    g_assert_not_reached ();
  }
}

static void
narrow_breakpoint_unapply_cb (ManetteDemoWindow *self)
{
  GtkListBoxRow *row;
  int index = 0;

  while ((row = gtk_list_box_get_row_at_index (self->devices_list, index++)) != NULL) {
    ManetteDevice *device = g_object_get_data (G_OBJECT (row), "-manette-device");

    if (device == self->selected_device) {
      gtk_list_box_set_selection_mode (self->devices_list, GTK_SELECTION_SINGLE);
      gtk_list_box_select_row (self->devices_list, row);
      break;
    }
  }
}

static void
button_pressed_cb (ManetteDemoWindow *self,
                   ManetteButton      button,
                   ManetteDevice     *device)
{
  g_assert (device == self->selected_device);

  gtk_widget_set_state_flags (self->button_indicators[button],
                              GTK_STATE_FLAG_CHECKED,
                              FALSE);
}

static void
button_released_cb (ManetteDemoWindow *self,
                    ManetteButton      button,
                    ManetteDevice     *device)
{
  g_assert (device == self->selected_device);

  gtk_widget_unset_state_flags (self->button_indicators[button],
                                GTK_STATE_FLAG_CHECKED);
}

static void
axis_changed_cb (ManetteDemoWindow *self,
                 ManetteAxis        axis,
                 double             value,
                 ManetteDevice     *device)
{
  g_autofree char *value_str = NULL;

  g_assert (device == self->selected_device);

  value_str = g_strdup_printf ("%.5lf", value);

  manette_demo_axis_bar_set_value (self->axis_bars[axis], value);
  gtk_label_set_label (self->axis_labels[axis], value_str);
}

static void
select_device (ManetteDemoWindow *self,
               ManetteDevice     *device)
{
  if (self->selected_device) {
    g_signal_handlers_disconnect_by_func (self->selected_device,
                                          button_pressed_cb, self);
    g_signal_handlers_disconnect_by_func (self->selected_device,
                                          button_released_cb, self);
    g_signal_handlers_disconnect_by_func (self->selected_device,
                                          axis_changed_cb, self);
  }

  self->selected_device = device;

  if (self->selected_device) {
    ManetteDeviceType device_type;
    g_autofree char *device_type_name = NULL;
    g_autofree char *stripped_name = NULL;
    const char *name;
    ManetteButton button;
    ManetteAxis axis;

    g_signal_connect_swapped (self->selected_device, "button-pressed",
                              G_CALLBACK (button_pressed_cb), self);
    g_signal_connect_swapped (self->selected_device, "button-released",
                              G_CALLBACK (button_released_cb), self);
    g_signal_connect_swapped (self->selected_device, "absolute-axis-changed",
                              G_CALLBACK (axis_changed_cb), self);

    device_type = manette_device_get_device_type (device);
    device_type_name = get_device_type_name (device_type);
    name = manette_device_get_name (device);
    stripped_name = g_strstrip (g_strdup (name));

    adw_navigation_page_set_title (self->device_page, stripped_name);

    adw_action_row_set_subtitle (self->name_row, name);
    adw_action_row_set_subtitle (self->guid_row, manette_device_get_guid (device));
    adw_action_row_set_subtitle (self->device_type_row, device_type_name);
    gtk_widget_set_visible (self->rumble_group, manette_device_has_rumble (device));

    gtk_widget_set_visible (self->buttons_group, FALSE);
    gtk_widget_set_visible (self->axes_group, FALSE);

    for (button = 0; button < N_BUTTONS; button++) {
      gboolean has_button = manette_device_has_button (self->selected_device, button);

      gtk_widget_set_visible (self->button_cells[button], has_button);

      if (has_button)
        gtk_widget_set_visible (self->buttons_group, TRUE);

      button_released_cb (self, button, self->selected_device);
    }

    for (axis = 0; axis < N_AXES; axis++) {
      gboolean has_axis = manette_device_has_axis (self->selected_device, axis);

      gtk_widget_set_visible (self->axis_cells[axis], has_axis);

      if (has_axis)
        gtk_widget_set_visible (self->axes_group, TRUE);

      axis_changed_cb (self, axis, 0, self->selected_device);
    }

    gtk_stack_set_visible_child_name (self->empty_stack, "content");
  } else {
    gtk_stack_set_visible_child_name (self->empty_stack, "empty");
  }
//  gtk_widget_set_visible (self->axes_group, FALSE);
}

static void
row_activated_cb (ManetteDemoWindow *self,
                  GtkListBoxRow     *row)
{
  ManetteDevice *device = g_object_get_data (G_OBJECT (row), "-manette-device");

  gtk_list_box_select_row (self->devices_list, row);

  select_device (self, device);

  adw_navigation_split_view_set_show_content (self->split_view, TRUE);
}

static GtkWidget *
create_device_row (ManetteDevice *device)
{
  g_autofree char *name = g_strstrip (g_strdup (manette_device_get_name (device)));
  GtkWidget *box, *icon, *label, *row;

  icon = gtk_image_new_from_icon_name ("input-gaming-symbolic");

  label = gtk_label_new (name);
  gtk_label_set_xalign (GTK_LABEL (label), 0);
  gtk_label_set_ellipsize (GTK_LABEL (label), PANGO_ELLIPSIZE_END);
  gtk_widget_set_hexpand (label, TRUE);

  box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 9);
  gtk_widget_set_margin_top (box, 12);
  gtk_widget_set_margin_bottom (box, 12);
  gtk_widget_set_margin_start (box, 3);
  gtk_widget_set_margin_end (box, 3);
  gtk_box_append (GTK_BOX (box), icon);
  gtk_box_append (GTK_BOX (box), label);

  row = gtk_list_box_row_new ();
  gtk_list_box_row_set_child (GTK_LIST_BOX_ROW (row), box);
  g_object_set_data (G_OBJECT (row), "-manette-device", device);

  return row;
}

static void
device_disconnected_cb (ManetteDemoWindow *self,
                        ManetteDevice     *device)
{
  GtkListBoxRow *row;
  int index = 0;

  while ((row = gtk_list_box_get_row_at_index (self->devices_list, index++)) != NULL) {
    ManetteDevice *d = g_object_get_data (G_OBJECT (row), "-manette-device");

    if (d == device) {
      gtk_list_box_remove (self->devices_list, GTK_WIDGET (row));
      index--;
      break;
    }
  }

  row = gtk_list_box_get_row_at_index (self->devices_list, MAX (index - 1, 0));

  if (row) {
    ManetteDevice *new_device = g_object_get_data (G_OBJECT (row), "-manette-device");
    gtk_list_box_select_row (self->devices_list, row);
    select_device (self, new_device);
  } else {
    select_device (self, NULL);
  }
}

static void
device_connected_cb (ManetteDemoWindow *self,
                     ManetteDevice     *device)
{
  gboolean was_empty = gtk_list_box_get_row_at_index (self->devices_list, 0) == NULL;

  gtk_list_box_append (self->devices_list, create_device_row (device));

  g_signal_connect_swapped (device, "disconnected",
                            G_CALLBACK (device_disconnected_cb), self);

  if (was_empty) {
    GtkListBoxRow *row = gtk_list_box_get_row_at_index (self->devices_list, 0);
    gtk_list_box_select_row (self->devices_list, row);
    select_device (self, device);
  }
}

static void
rumble_activated_cb (ManetteDemoWindow *self)
{
  double strong, weak;
  guint16 duration;

  g_assert (self->selected_device);

  strong = gtk_adjustment_get_value (self->strong_magnitude_adj);
  weak = gtk_adjustment_get_value (self->weak_magnitude_adj);
  duration = (guint16) gtk_adjustment_get_value (self->duration_adj);

  manette_device_rumble (self->selected_device, strong, weak, duration);
}

static void
manette_demo_window_dispose (GObject *object)
{
  ManetteDemoWindow *self = MANETTE_DEMO_WINDOW (object);

  g_clear_object (&self->monitor);

  G_OBJECT_CLASS (manette_demo_window_parent_class)->dispose (object);
}

static void
manette_demo_window_class_init (ManetteDemoWindowClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->dispose = manette_demo_window_dispose;

  gtk_widget_class_set_template_from_resource (widget_class,
                                               "/org/gnome/Manette1/Demo/manette-demo-window.ui");

  gtk_widget_class_bind_template_child (widget_class, ManetteDemoWindow, empty_stack);
  gtk_widget_class_bind_template_child (widget_class, ManetteDemoWindow, split_view);
  gtk_widget_class_bind_template_child (widget_class, ManetteDemoWindow, devices_list);
  gtk_widget_class_bind_template_child (widget_class, ManetteDemoWindow, device_page);
  gtk_widget_class_bind_template_child (widget_class, ManetteDemoWindow, name_row);
  gtk_widget_class_bind_template_child (widget_class, ManetteDemoWindow, guid_row);
  gtk_widget_class_bind_template_child (widget_class, ManetteDemoWindow, device_type_row);
  gtk_widget_class_bind_template_child (widget_class, ManetteDemoWindow, rumble_group);
  gtk_widget_class_bind_template_child (widget_class, ManetteDemoWindow, strong_magnitude_adj);
  gtk_widget_class_bind_template_child (widget_class, ManetteDemoWindow, weak_magnitude_adj);
  gtk_widget_class_bind_template_child (widget_class, ManetteDemoWindow, duration_adj);
  gtk_widget_class_bind_template_child (widget_class, ManetteDemoWindow, buttons_group);
  gtk_widget_class_bind_template_child (widget_class, ManetteDemoWindow, buttons_flowbox);
  gtk_widget_class_bind_template_child (widget_class, ManetteDemoWindow, axes_group);
  gtk_widget_class_bind_template_child (widget_class, ManetteDemoWindow, axes_flowbox);

  gtk_widget_class_bind_template_callback (widget_class, narrow_breakpoint_unapply_cb);
  gtk_widget_class_bind_template_callback (widget_class, row_activated_cb);
  gtk_widget_class_bind_template_callback (widget_class, rumble_activated_cb);
}

static void
manette_demo_window_init (ManetteDemoWindow *self)
{
  g_autofree ManetteDevice **devices = NULL;
  gsize n_devices, i;
  ManetteButton button;
  ManetteAxis axis;

  gtk_widget_init_template (GTK_WIDGET (self));

  for (button = 0; button < N_BUTTONS; button++) {
    GtkWidget *child, *box, *label, *indicator;
    g_autofree char *button_name = get_button_name (button);

    child = gtk_flow_box_child_new ();
    gtk_widget_set_focusable (child, FALSE);

    box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
    gtk_widget_set_margin_top (box, 12);
    gtk_widget_set_margin_bottom (box, 12);
    gtk_widget_set_margin_start (box, 12);
    gtk_widget_set_margin_end (box, 12);
    gtk_flow_box_child_set_child (GTK_FLOW_BOX_CHILD (child), box);

    label = gtk_label_new (button_name);
    gtk_widget_set_hexpand (label, TRUE);
    gtk_label_set_xalign (GTK_LABEL (label), 0);
    gtk_label_set_ellipsize (GTK_LABEL (label), PANGO_ELLIPSIZE_END);
    gtk_box_append (GTK_BOX (box), label);

    indicator = adw_bin_new ();
    gtk_widget_add_css_class (indicator, "indicator");
    gtk_box_append (GTK_BOX (box), indicator);

    self->button_cells[button] = child;
    self->button_indicators[button] = indicator;

    gtk_flow_box_append (self->buttons_flowbox, child);
  }

  for (axis = 0; axis < N_AXES; axis++) {
    GtkWidget *child, *box, *label, *bar, *value;
    g_autofree char *axis_name = get_axis_name (axis);

    child = gtk_flow_box_child_new ();
    gtk_widget_set_focusable (child, FALSE);

    box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
    gtk_widget_set_margin_top (box, 12);
    gtk_widget_set_margin_bottom (box, 6);
    gtk_widget_set_margin_start (box, 12);
    gtk_widget_set_margin_end (box, 12);
    gtk_flow_box_child_set_child (GTK_FLOW_BOX_CHILD (child), box);

    label = gtk_label_new (axis_name);
    gtk_label_set_xalign (GTK_LABEL (label), 0);
    gtk_label_set_ellipsize (GTK_LABEL (label), PANGO_ELLIPSIZE_END);
    gtk_box_append (GTK_BOX (box), label);

    bar = manette_demo_axis_bar_new (axis == MANETTE_AXIS_LEFT_TRIGGER ||
                                     axis == MANETTE_AXIS_RIGHT_TRIGGER);
    gtk_box_append (GTK_BOX (box), bar);

    value = gtk_label_new (NULL);
    gtk_widget_add_css_class (value, "caption");
    gtk_widget_add_css_class (value, "numeric");
    gtk_label_set_ellipsize (GTK_LABEL (label), PANGO_ELLIPSIZE_END);
    gtk_box_append (GTK_BOX (box), value);

    self->axis_cells[axis] = child;
    self->axis_bars[axis] = MANETTE_DEMO_AXIS_BAR (bar);
    self->axis_labels[axis] = GTK_LABEL (value);

    gtk_flow_box_append (self->axes_flowbox, child);
  }

  self->monitor = manette_monitor_new ();

  devices = manette_monitor_list_devices (self->monitor, &n_devices);
  for (i = 0; i < n_devices; i++)
    device_connected_cb (self, devices[i]);

  g_signal_connect_object (self->monitor, "device-connected",
                           (GCallback) device_connected_cb, self,
                           G_CONNECT_SWAPPED);
}

GtkWindow *
manette_demo_window_new (GtkApplication *app)
{
  g_return_val_if_fail (GTK_IS_APPLICATION (app), NULL);

  return g_object_new (MANETTE_DEMO_TYPE_WINDOW, "application", app, NULL);
}
