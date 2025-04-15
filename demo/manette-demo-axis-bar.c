/* manette-demo-axis-bar.c
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

#include "manette-demo-axis-bar.h"

#include <adwaita.h>

struct _ManetteDemoAxisBar
{
  GtkWidget parent_instance;

  double value;
  gboolean is_trigger;
};

G_DEFINE_FINAL_TYPE (ManetteDemoAxisBar, manette_demo_axis_bar, GTK_TYPE_WIDGET)

enum {
  PROP_0,
  PROP_VALUE,
  PROP_IS_TRIGGER,
  LAST_PROP,
};

static GParamSpec *props[LAST_PROP];

static void
manette_demo_axis_bar_snapshot (GtkWidget   *widget,
                                GtkSnapshot *snapshot)
{
  ManetteDemoAxisBar *self = MANETTE_DEMO_AXIS_BAR (widget);
  int width = gtk_widget_get_width (widget);
  int height = gtk_widget_get_height (widget);
  gboolean is_rtl = gtk_widget_get_direction (GTK_WIDGET (self)) == GTK_TEXT_DIR_RTL;
  AdwStyleManager *manager;
  AdwAccentColor accent_color;
  GdkRGBA rgba = {};
  double bar_position, bar_width;

  manager = adw_style_manager_get_default ();
  accent_color = adw_style_manager_get_accent_color (manager);
  adw_accent_color_to_rgba (accent_color, &rgba);

  if (self->is_trigger) {
    if (is_rtl)
      bar_position = 1.0 - self->value;
    else
      bar_position = 0.0;

    bar_width = self->value;
  } else {
    if (is_rtl)
      bar_position = (MIN (-self->value, 0.0) + 1) * 0.5;
    else
      bar_position = (MIN (self->value, 0.0) + 1) * 0.5;

    bar_width = ABS (self->value) * 0.5;
  }

  gtk_snapshot_append_color (snapshot, &rgba,
                             &GRAPHENE_RECT_INIT (bar_position * width, 0,
                                                  bar_width * width, height));
}

static void
manette_demo_axis_bar_get_property (GObject    *object,
                                    guint       prop_id,
                                    GValue     *value,
                                    GParamSpec *pspec)
{
  ManetteDemoAxisBar *self = MANETTE_DEMO_AXIS_BAR (object);

  switch (prop_id) {
  case PROP_VALUE:
    g_value_set_double (value, self->value);
    break;
  case PROP_IS_TRIGGER:
    g_value_set_boolean (value, self->is_trigger);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
  }
}

static void
manette_demo_axis_bar_set_property (GObject      *object,
                                    guint         prop_id,
                                    const GValue *value,
                                    GParamSpec   *pspec)
{
  ManetteDemoAxisBar *self = MANETTE_DEMO_AXIS_BAR (object);

  switch (prop_id) {
  case PROP_VALUE:
    manette_demo_axis_bar_set_value (self, g_value_get_double (value));
    break;
  case PROP_IS_TRIGGER:
    self->is_trigger = g_value_get_boolean (value);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
  }
}

static void
manette_demo_axis_bar_class_init (ManetteDemoAxisBarClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->get_property = manette_demo_axis_bar_get_property;
  object_class->set_property = manette_demo_axis_bar_set_property;
  widget_class->snapshot = manette_demo_axis_bar_snapshot;

  props[PROP_VALUE] =
    g_param_spec_double ("value", NULL, NULL,
                         -G_MAXDOUBLE, G_MAXDOUBLE, 0.0,
                         G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_EXPLICIT_NOTIFY);

  props[PROP_IS_TRIGGER] =
    g_param_spec_boolean ("is-trigger", NULL, NULL,
                          FALSE,
                          G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (object_class, LAST_PROP, props);

  gtk_widget_class_set_css_name (widget_class, "axis-bar");
}

static void
manette_demo_axis_bar_init (ManetteDemoAxisBar *self)
{
  AdwStyleManager *manager = adw_style_manager_get_default ();

  gtk_widget_set_overflow (GTK_WIDGET (self), GTK_OVERFLOW_HIDDEN);

  g_signal_connect_object (manager, "notify::accent-color",
                           G_CALLBACK (gtk_widget_queue_draw), self,
                           G_CONNECT_SWAPPED);
}

GtkWidget *
manette_demo_axis_bar_new (gboolean is_trigger)
{
  is_trigger = !!is_trigger;

  return g_object_new (MANETTE_DEMO_TYPE_AXIS_BAR, "is-trigger", is_trigger, NULL);
}

void
manette_demo_axis_bar_set_value (ManetteDemoAxisBar *self,
                                 double              value)
{
  g_return_if_fail (MANETTE_DEMO_IS_AXIS_BAR (self));

  self->value = value;

  gtk_widget_queue_draw (GTK_WIDGET (self));

  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_VALUE]);
}
