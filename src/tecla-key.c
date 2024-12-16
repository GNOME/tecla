/* Copyright (C) 2023 Red Hat, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <http://www.gnu.org/licenses/>.
 *
 * Author: Carlos Garnacho <carlosg@gnome.org>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "tecla-key.h"

#include <math.h>

struct _TeclaKey
{
	GtkWidget parent_class;
	gchar *name;
	gchar *label;
};

enum
{
	PROP_0,
	PROP_NAME,
	PROP_LABEL,
	N_PROPS,
};

static GParamSpec *props[N_PROPS] = { 0, };

enum
{
	ACTIVATED,
	N_SIGNALS,
};

static guint signals[N_SIGNALS] = { 0, };

G_DEFINE_TYPE (TeclaKey, tecla_key, GTK_TYPE_WIDGET)

static void
tecla_key_get_property (GObject    *object,
			guint       prop_id,
			GValue     *value,
			GParamSpec *pspec)
{
	TeclaKey *key = TECLA_KEY (object);

	switch (prop_id) {
	case PROP_NAME:
		g_value_set_string (value, key->name);
		break;
	case PROP_LABEL:
		g_value_set_string (value, key->label);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
	}
}

static void
tecla_key_set_property (GObject      *object,
			guint         prop_id,
			const GValue *value,
			GParamSpec   *pspec)
{
	TeclaKey *key = TECLA_KEY (object);

	switch (prop_id) {
	case PROP_NAME:
		key->name = g_value_dup_string (value);
		break;
	case PROP_LABEL:
		tecla_key_set_label (TECLA_KEY (object),
				     g_value_get_string (value));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
	}
}

static void
tecla_key_finalize (GObject *object)
{
	TeclaKey *key = TECLA_KEY (object);

	g_free (key->name);
	g_free (key->label);

	G_OBJECT_CLASS (tecla_key_parent_class)->finalize (object);
}

static void
tecla_key_snapshot (GtkWidget *widget,
		    GtkSnapshot *snapshot)
{
	TeclaKey *key = TECLA_KEY (widget);
	PangoLayout *layout;
	PangoRectangle rect;
	GdkRGBA color;
	int width, height, x, y;
	float scale;

	layout = gtk_widget_create_pango_layout (widget, key->label);
	gtk_widget_get_color (widget, &color);

	width = gtk_widget_get_width (widget);
	height = gtk_widget_get_height (widget);
	pango_layout_get_pixel_extents (layout, NULL, &rect);
	scale = MIN ((float) height / rect.height * 0.75, 3);

	/* Snap scale to 1/4ths of logical pixels */
	scale = roundf (scale * 4.0) / 4.0;

	/* Ensure pixel exactness when placing the layout
	 * centered and scaled on the widget, instead
	 * of translate/scale/translate.
	 */
	x = (width / 2) - ((rect.width / 2) * scale);
	y = (height / 2) - ((rect.height / 2) * scale);

	gtk_snapshot_translate (snapshot, &GRAPHENE_POINT_INIT (x, y));
	gtk_snapshot_scale (snapshot, scale, scale);

	gtk_snapshot_append_layout (snapshot,
				    layout,
				    &color);
}

static void
tecla_key_class_init (TeclaKeyClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);
	GtkCssProvider *css_provider;

	object_class->set_property = tecla_key_set_property;
	object_class->get_property = tecla_key_get_property;
	object_class->finalize = tecla_key_finalize;

	widget_class->snapshot = tecla_key_snapshot;

	signals[ACTIVATED] =
		g_signal_new ("activated",
			      G_OBJECT_CLASS_TYPE (object_class),
			      G_SIGNAL_RUN_LAST,
			      0,
			      NULL, NULL, NULL,
			      G_TYPE_NONE, 0);

	props[PROP_NAME] =
		g_param_spec_string ("name",
				     "name",
				     "name",
				     NULL,
				     G_PARAM_READWRITE |
				     G_PARAM_CONSTRUCT_ONLY |
				     G_PARAM_STATIC_STRINGS);
	props[PROP_LABEL] =
		g_param_spec_string ("label",
				     "label",
				     "label",
				     NULL,
				     G_PARAM_READWRITE |
				     G_PARAM_STATIC_STRINGS);

	g_object_class_install_properties (object_class, N_PROPS, props);

	css_provider = gtk_css_provider_new ();
	gtk_css_provider_load_from_resource (css_provider,
					     "/org/gnome/tecla/tecla-key.css");
	gtk_style_context_add_provider_for_display (gdk_display_get_default (),
						    GTK_STYLE_PROVIDER (css_provider),
						    GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

	gtk_widget_class_set_css_name (widget_class, "button");
}

static void
click_release_cb (GtkGestureClick *gesture,
		  int              n_press,
		  double           x,
		  double           y,
		  TeclaKey        *key)
{
	g_signal_emit (key, signals[ACTIVATED], 0);
}

static void
tecla_key_init (TeclaKey *key)
{
	GtkGesture *gesture;

	gesture = gtk_gesture_click_new ();
	g_signal_connect (gesture, "released",
			  G_CALLBACK (click_release_cb), key);

	gtk_widget_add_controller (GTK_WIDGET (key),
				   GTK_EVENT_CONTROLLER (gesture));
	gtk_widget_add_css_class (GTK_WIDGET (key), "opaque");
}

GtkWidget *
tecla_key_new (const gchar *name)
{
	return g_object_new (TECLA_TYPE_KEY,
			     "name", name,
			     NULL);
}

void
tecla_key_set_label (TeclaKey    *key,
		     const gchar *label)
{
	if (g_strcmp0 (label, key->label) == 0)
		return;

	g_free (key->label);
	key->label = g_strdup (label);
	gtk_widget_queue_draw (GTK_WIDGET (key));

        g_object_notify (G_OBJECT (key), "label");
}

const gchar *
tecla_key_get_name (TeclaKey *key)
{
	return key->name;
}
