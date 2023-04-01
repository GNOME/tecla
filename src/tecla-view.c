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

#include <gtk/gtk.h>
#include <xkbcommon/xkbcommon.h>

#include "tecla-view.h"

#include "pc105.h"
#include "tecla-key.h"

struct _TeclaView
{
	GtkWidget parent_instance;
	GtkWidget *grid;
	GHashTable *keys_by_name;
	TeclaModel *model;
	guint model_changed_id;

	GList *level2_keys;
	GList *level3_keys;
};

G_DEFINE_TYPE (TeclaView, tecla_view, GTK_TYPE_WIDGET)

enum
{
	PROP_0,
	PROP_MODEL,
	N_PROPS,
};

static GParamSpec *props[N_PROPS];

enum
{
	KEY_ACTIVATED,
	N_SIGNALS,
};

static guint signals[N_SIGNALS] = { 0, };

static void
tecla_view_set_property (GObject      *object,
			 guint         prop_id,
			 const GValue *value,
			 GParamSpec   *pspec)
{
	TeclaView *view = TECLA_VIEW (object);

	switch (prop_id) {
	case PROP_MODEL:
		tecla_view_set_model (view, g_value_get_object (value));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}

static void
tecla_view_get_property (GObject    *object,
			 guint       prop_id,
			 GValue     *value,
			 GParamSpec *pspec)
{
	TeclaView *view = TECLA_VIEW (object);

	switch (prop_id) {
	case PROP_MODEL:
		g_value_set_object (value, view->model);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}

static void
tecla_view_finalize (GObject *object)
{
	TeclaView *view = TECLA_VIEW (object);

	g_hash_table_unref (view->keys_by_name);
	g_clear_list (&view->level2_keys, NULL);
	g_clear_list (&view->level3_keys, NULL);
	gtk_widget_unparent (gtk_widget_get_first_child (GTK_WIDGET (view)));

	G_OBJECT_CLASS (tecla_view_parent_class)->finalize (object);
}

static void
bind_state (GtkWidget     *w1,
	    GtkStateFlags  old_flags,
	    GtkWidget     *w2)
{
	GtkStateFlags flags;

	flags = gtk_widget_get_state_flags (w1);

	if (flags != gtk_widget_get_state_flags (w2))
		gtk_widget_set_state_flags (w2, flags, TRUE);
}

static void
pair_state (GtkWidget *widget,
	    GtkWidget *other_widget)
{
	g_signal_connect (widget, "state-flags-changed",
			  G_CALLBACK (bind_state), other_widget);
	g_signal_connect (other_widget, "state-flags-changed",
			  G_CALLBACK (bind_state), widget);
}

static void
key_activated_cb (TeclaKey  *key,
		  TeclaView *view)
{
	const gchar *name;

	name = tecla_key_get_name (key);
	g_signal_emit (view, signals[KEY_ACTIVATED], 0, name);
}

static void
construct_grid (TeclaView *view)
{
	gulong i, j;
	int anchor = 0;

	for (i = 0; i < G_N_ELEMENTS (pc105_layout.rows); i++) {
		for (j = 0; j < G_N_ELEMENTS (pc105_layout.rows[i].keys); j++) {
			TeclaLayoutKey *key;
			GtkWidget *button, *prev;
			double width, height;
			int left, top;

			key = &pc105_layout.rows[i].keys[j];
			if (!key->name)
				break;

			left = anchor;
			top = key->height >= 0 ? i : i + key->height + 1;
			width = MAX (key->width, 1) * 4;
			height = MAX (fabs (key->height), 1);

			button = tecla_key_new (key->name);
			g_signal_connect (button, "activated",
					  G_CALLBACK (key_activated_cb), view);

			gtk_widget_add_css_class (button, "tecla-key");
			gtk_grid_attach (GTK_GRID (view->grid), button,
					 left, top,
					 (int) width,
					 (int) height);

			anchor += (int) width;

			prev = g_hash_table_lookup (view->keys_by_name,
						    key->name);

			if (prev) {
				pair_state (prev, button);
			} else {
				g_hash_table_insert (view->keys_by_name,
						     (gpointer) tecla_key_get_name (TECLA_KEY (button)),
						     button);
			}
		}

		anchor = 0;
	}

	gtk_widget_set_layout_manager (GTK_WIDGET (view), gtk_bin_layout_new ());
}

static void
tecla_view_constructed (GObject *object)
{
	TeclaView *view = TECLA_VIEW (object);

	G_OBJECT_CLASS (tecla_view_parent_class)->constructed (object);

	construct_grid (view);
}

static void
tecla_view_class_init (TeclaViewClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

	object_class->set_property = tecla_view_set_property;
	object_class->get_property = tecla_view_get_property;
	object_class->finalize = tecla_view_finalize;
	object_class->constructed = tecla_view_constructed;

	signals[KEY_ACTIVATED] =
		g_signal_new ("key-activated",
			      G_OBJECT_CLASS_TYPE (object_class),
			      G_SIGNAL_RUN_LAST,
			      0, NULL, NULL, NULL,
			      G_TYPE_NONE, 1, G_TYPE_STRING);

	props[PROP_MODEL] =
		g_param_spec_object ("model",
				     "Model",
				     "Model",
				     TECLA_TYPE_MODEL,
				     G_PARAM_READWRITE |
				     G_PARAM_CONSTRUCT_ONLY);

	g_object_class_install_properties (object_class, N_PROPS, props);

	gtk_widget_class_set_template_from_resource (widget_class, "/org/gnome/tecla/tecla-view.ui");
	gtk_widget_class_bind_template_child (widget_class, TeclaView, grid);
}

static void
key_pressed_cb (GtkEventControllerKey *controller,
		guint                  keyval,
		guint                  keycode,
		GdkModifierType        modifiers,
		TeclaView             *view)
{
	const gchar *name;
	GtkWidget *key;

	name = tecla_model_get_keycode_key (view->model, keycode);
	key = g_hash_table_lookup (view->keys_by_name, name);

	if (key)
		gtk_widget_set_state_flags (key, GTK_STATE_FLAG_ACTIVE, FALSE);

	g_signal_emit (view, signals[KEY_ACTIVATED], 0, name);
}

static void
key_released_cb (GtkEventControllerKey *controller,
		 guint                  keyval,
		 guint                  keycode,
		 GdkModifierType        modifiers,
		 TeclaView             *view)
{
	const gchar *name;
	GtkWidget *key;

	name = tecla_model_get_keycode_key (view->model, keycode);
	key = g_hash_table_lookup (view->keys_by_name, name);

	if (key)
		gtk_widget_unset_state_flags (key, GTK_STATE_FLAG_ACTIVE);
}

static void
tecla_view_init (TeclaView *view)
{
	GtkEventController *controller;

	gtk_widget_init_template (GTK_WIDGET (view));
	view->keys_by_name = g_hash_table_new (g_str_hash, g_str_equal);

	controller = gtk_event_controller_key_new ();
	g_signal_connect (controller, "key-pressed",
			  G_CALLBACK (key_pressed_cb), view);
	g_signal_connect (controller, "key-released",
			  G_CALLBACK (key_released_cb), view);
	gtk_widget_add_controller (GTK_WIDGET (view), controller);

	gtk_widget_set_focusable (GTK_WIDGET (view), TRUE);
}

static void
update_from_model_foreach (const gchar *name,
			   TeclaKey    *key,
			   TeclaView   *view)
{
	xkb_keycode_t keycode;
	gchar *action;
	guint keyval;

	action = tecla_model_get_key_label (view->model, name);
	tecla_key_set_label (key, action);
	g_free (action);

	keycode = tecla_model_get_key_keycode (view->model, name);
	keyval = tecla_model_get_keyval (view->model, 0, keycode);

	if ((keyval == GDK_KEY_Shift_L || keyval == GDK_KEY_Shift_R) &&
	    !g_list_find_custom (view->level2_keys, name, (GCompareFunc) g_strcmp0))
		view->level2_keys = g_list_prepend (view->level2_keys, (gpointer) name);

	if (keyval == GDK_KEY_ISO_Level3_Shift &&
	    !g_list_find_custom (view->level3_keys, name, (GCompareFunc) g_strcmp0))
		view->level3_keys = g_list_prepend (view->level3_keys, (gpointer) name);
}

static void
update_view (TeclaView *view)
{
	g_hash_table_foreach (view->keys_by_name,
			      (GHFunc) update_from_model_foreach,
			      view);
}

GtkWidget *
tecla_view_new (void)
{
	return g_object_new (TECLA_TYPE_VIEW, NULL);
}

static void
model_changed_cb (TeclaModel *model,
		  TeclaView  *view)
{
	update_view (view);
}

void
tecla_view_set_model (TeclaView  *view,
		      TeclaModel *model)
{
	if (view->model == model)
		return;

	if (view->model_changed_id) {
		g_signal_handler_disconnect (view->model, view->model_changed_id);
		view->model_changed_id = 0;
	}

	if (view->model) {
		g_clear_list (&view->level2_keys, NULL);
		g_clear_list (&view->level3_keys, NULL);
	}

	g_set_object (&view->model, model);

	if (view->model) {
		view->model_changed_id =
			g_signal_connect (view->model, "changed",
					  G_CALLBACK (model_changed_cb), view);
	}

	update_view (view);
}
