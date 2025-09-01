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

#include "config.h"
#include "tecla-application.h"

#include "tecla-key.h"
#include "tecla-keymap-observer.h"
#include "tecla-model.h"
#include "tecla-view.h"

#include <glib/gi18n.h>
#include <stdlib.h>

#ifdef GDK_WINDOWING_WAYLAND
#include <gdk/wayland/gdkwayland.h>
#endif

typedef struct
{
	int level;
	guint keyval;
} KeyInfo;

typedef struct
{
	GtkWindow *window;
	TeclaView *view;
	TeclaModel *model;
	gulong remove_handler_id;
} TeclaInstance;

struct _TeclaApplication
{
	GtkApplication parent_instance;
	TeclaKeymapObserver *observer;
	TeclaInstance main;
	GList *instances; /* TeclaInstance* */
	gchar *layout;
	gchar *parent_handle;
};

static GtkPopover *current_popover = NULL;

G_DEFINE_TYPE (TeclaApplication, tecla_application, GTK_TYPE_APPLICATION)

static int
tecla_application_command_line (GApplication            *app,
				GApplicationCommandLine *cl)
{
	TeclaApplication *tecla_app = TECLA_APPLICATION (app);
	GVariantDict *options;
	g_autofree GStrv argv = NULL;
	int argc;

	options = g_application_command_line_get_options_dict (cl);
	argv = g_application_command_line_get_arguments (cl, &argc);

	if (argc > 1) {
		g_set_str (&tecla_app->layout, argv[1]);
		g_set_str (&tecla_app->parent_handle, NULL);
		g_variant_dict_lookup (options, "parent-handle", "s", &tecla_app->parent_handle);
	}

	g_application_activate (app);

	return EXIT_SUCCESS;
}

const GOptionEntry all_options[] = {
	{ "parent-handle", 0, 0, G_OPTION_ARG_STRING, NULL, N_("Attach to a parent window"), N_("Window handle") },
	{ "version", 0, 0, G_OPTION_ARG_NONE, NULL, N_("Display version number"), NULL },
	{ NULL, 0, 0, 0, NULL, NULL, NULL } /* end the list */
};

static int
tecla_application_handle_local_options (GApplication *app,
					GVariantDict *options)
{
	if (g_variant_dict_contains (options, "version")) {
		g_print ("%s %s\n", PACKAGE, VERSION);

		return 0;
	}

	return -1;
}

static void
level_clicked_cb (GtkButton *button,
		  TeclaView *view)
{
	int level;

	level = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (button), "level"));
	tecla_view_set_current_level (view, level);
}

static void
view_level_notify_cb (TeclaView  *view,
		      GParamSpec *pspec,
		      GtkButton  *button)
{
	int level, toggle_level;

	level = tecla_view_get_current_level (view);
	toggle_level = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (button), "level"));

	if (level == toggle_level)
		gtk_widget_set_state_flags (GTK_WIDGET (button), GTK_STATE_FLAG_CHECKED, FALSE);
	else
		gtk_widget_unset_state_flags (GTK_WIDGET (button), GTK_STATE_FLAG_CHECKED);
}

static void
num_levels_notify_cb (TeclaView  *view,
		      GParamSpec *pspec,
		      GtkBox     *levels)
{
	int num_levels, i;
	GtkWidget *child;

	while ((child = gtk_widget_get_first_child (GTK_WIDGET (levels))) != NULL)
		gtk_box_remove (levels, child);

	num_levels = tecla_view_get_num_levels (view);

	for (i = 0; i < num_levels; i++) {
		GtkWidget *button;
		g_autofree char *label = g_strdup_printf ("%d", i + 1);

		button = gtk_button_new_with_label (label);
		gtk_widget_add_css_class (button, "toggle");
		gtk_widget_add_css_class (button, "pill");
		gtk_widget_set_focusable (button, FALSE);
		gtk_box_append (levels, button);
		g_object_set_data (G_OBJECT (button), "level",
				   GINT_TO_POINTER (i));

		g_signal_connect (button, "clicked",
				  G_CALLBACK (level_clicked_cb), view);
		g_signal_connect_object (view, "notify::level",
					 G_CALLBACK (view_level_notify_cb), button, 0);
	}
}

static void
update_title (GtkWindow  *window,
	      TeclaModel *model)
{
	g_autofree gchar *title = NULL;

	title = g_strdup_printf ("%s ‐ %s", _("Keyboard Layout"),
				 tecla_model_get_name (model));
	gtk_window_set_title (GTK_WINDOW (window), title);
}

static GtkWindow *
create_window (TeclaApplication  *app,
	       TeclaView        **view_out)
{
	g_autoptr (GtkBuilder) builder = NULL;
	TeclaView *view;
	GtkWindow *window;
	GtkBox *levels;

	g_type_ensure (TECLA_TYPE_VIEW);

	builder = gtk_builder_new ();
	gtk_builder_add_from_resource (builder,
				       "/org/gnome/tecla/tecla-window.ui",
				       NULL);

	window = GTK_WINDOW (gtk_builder_get_object (builder, "window"));
	view = TECLA_VIEW (gtk_builder_get_object (builder, "view"));
	levels = GTK_BOX (gtk_builder_get_object (builder, "levels"));
	gtk_application_add_window (GTK_APPLICATION (app), window);

	g_signal_connect (view, "notify::num-levels",
			  G_CALLBACK (num_levels_notify_cb), levels);

	if (view_out)
		*view_out = view;

	return window;
}

static void
name_notify_cb (TeclaModel *model,
		GParamSpec *pspec,
		GtkWindow  *window)
{
	update_title (window, model);
}

static gboolean
unparent_popover (GtkWidget *popover)
{
	gtk_widget_unparent (popover);
	return G_SOURCE_REMOVE;
}

static void
popover_closed_cb (GtkPopover *popover,
		   TeclaView  *view)
{
	GtkWidget *parent;

	if (current_popover == popover)
		current_popover = NULL;

	parent = gtk_widget_get_parent (GTK_WIDGET (popover));
	gtk_widget_unset_state_flags (parent, GTK_STATE_FLAG_ACTIVE);

	g_idle_add ((GSourceFunc) unparent_popover, popover);
}

static GtkPopover *
create_popover (TeclaView    *view,
		TeclaModel   *model,
		GtkWidget    *widget,
		const gchar  *name,
		gchar       **a11y_description)
{
	int n_levels, i;
	xkb_keycode_t keycode;
	GtkPopover *popover;
	GtkWidget *box;
	g_autoptr (GArray) key_info = NULL;
	g_autoptr (GString) a11y_data = NULL;

	keycode = tecla_model_get_key_keycode (model, name);
	n_levels = tecla_view_get_num_levels (view);

	key_info = g_array_new (FALSE, TRUE, sizeof (KeyInfo));

	a11y_data = g_string_new (_("Key description:"));

	for (i = 0; i < n_levels; i++) {
		KeyInfo info;

		info.level = i;
		info.keyval = tecla_model_get_keyval (model,
						      info.level,
						      keycode);
		if (info.keyval == 0)
			continue;

		g_array_append_val (key_info, info);
	}

	if (key_info->len < 2)
		return NULL;

	box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
	gtk_widget_set_margin_start (box, 12);
	gtk_widget_set_margin_end (box, 12);
	gtk_widget_set_margin_top (box, 12);
	gtk_widget_set_margin_bottom (box, 12);

	for (i = 0; i < (int) key_info->len; i++) {
		GtkWidget *hbox, *level, *etching, *desc;
		KeyInfo *info;
		g_autofree gchar *str;
		g_autofree gchar *level_a11y = NULL;

		info = &g_array_index (key_info, KeyInfo, i);

		hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);

		str = g_strdup_printf ("%d", info->level + 1);
		level = gtk_label_new (str);
		gtk_widget_add_css_class (level, "heading");
		gtk_box_append (GTK_BOX (hbox), level);

		str = tecla_model_get_key_label (model, info->level, name);
		etching = tecla_key_new (NULL);
		tecla_key_set_label (TECLA_KEY (etching), str);
		gtk_widget_add_css_class (etching, "tecla-key");
		gtk_widget_set_sensitive (etching, FALSE);
		gtk_box_append (GTK_BOX (hbox), etching);

		desc = gtk_label_new (gdk_keyval_name (info->keyval));
		gtk_box_append (GTK_BOX (hbox), desc);

		gtk_box_append (GTK_BOX (box), hbox);

		g_string_append_printf (a11y_data, _(" Level %d, character %s."),
					info->level + 1,
					gdk_keyval_name (info->keyval));

	}

	popover = GTK_POPOVER (gtk_popover_new ());
	gtk_popover_set_child (popover, box);

	gtk_popover_set_autohide (popover, FALSE);
	gtk_popover_set_position (popover, GTK_POS_TOP);
	g_signal_connect_after (popover, "closed",
			  G_CALLBACK (popover_closed_cb), view);

	*a11y_description = g_strdup (a11y_data->str);
	return popover;
}

static void
key_activated_cb (TeclaView   *view,
		  const gchar *name,
		  GtkWidget   *widget,
		  TeclaModel  *model)
{
	GtkPopover *popover;
	g_autofree gchar *a11y_description = NULL;

	if (current_popover) {
		if (gtk_widget_get_parent (GTK_WIDGET (current_popover)) == widget) {
			gtk_popover_popdown (current_popover);
			return;
		}

		gtk_popover_popdown (current_popover);
	}

	if (!widget)
		return;

	popover = create_popover (view, model, widget, name, &a11y_description);

	if (popover) {
		gtk_widget_set_parent (GTK_WIDGET (popover), widget);
		gtk_widget_set_state_flags (widget, GTK_STATE_FLAG_ACTIVE, FALSE);
		gtk_popover_popup (popover);
		current_popover = popover;
		gtk_accessible_announce (GTK_ACCESSIBLE (view),
					a11y_description,
					GTK_ACCESSIBLE_ANNOUNCEMENT_PRIORITY_MEDIUM);
	}
}

static void
connect_model (GtkWindow  *window,
	       TeclaView  *view,
	       TeclaModel *model)
{
	tecla_view_set_model (view, model);
	g_signal_connect_object (model, "notify::name",
				 G_CALLBACK (name_notify_cb),
				 window, 0);

	g_signal_connect_object (view, "key-activated",
				 G_CALLBACK (key_activated_cb),
				 model, 0);
}

static void
observer_keymap_notify_cb (TeclaKeymapObserver *observer,
			   GParamSpec          *pspec,
			   TeclaApplication    *app)
{
	g_autoptr (TeclaModel) model = NULL;
	struct xkb_keymap *xkb_keymap;

	xkb_keymap = tecla_keymap_observer_get_keymap (observer);
	model = tecla_model_new_from_xkb_keymap (xkb_keymap);
	connect_model (app->main.window,
		       app->main.view, model);
	update_title (app->main.window, model);

	g_set_object (&app->main.model, model);
}

static void
observer_keymap_group_cb (TeclaKeymapObserver *observer,
			  GParamSpec          *pspec,
			  TeclaApplication    *app)
{
	int group;

	group = tecla_keymap_observer_get_group (observer);
	if (app->main.model)
		tecla_model_set_group (app->main.model, group);
}

void
window_removed_cb (TeclaApplication *tecla_app,
                   GtkWindow        *window,
                   gpointer          user_data)
{
	TeclaInstance *instance = user_data;

	tecla_app->instances =
		g_list_remove (tecla_app->instances, instance);

	g_signal_handler_disconnect (tecla_app, instance->remove_handler_id);

	g_clear_object (&instance->model);
	g_free (instance);
}

void
main_window_removed_cb (TeclaApplication *tecla_app,
                        GtkWindow        *window,
                        gpointer          user_data)
{
	if (tecla_app->main.window != window)
		return;

	g_clear_object (&tecla_app->observer);
	g_clear_object (&tecla_app->main.model);
	tecla_app->main.view = NULL;
	tecla_app->main.window = NULL;
}

static void
tecla_application_activate (GApplication *app)
{
	TeclaApplication *tecla_app = TECLA_APPLICATION (app);
	g_autofree char *layout = g_steal_pointer (&tecla_app->layout);
	g_autofree char *parent_handle = g_steal_pointer (&tecla_app->parent_handle);

	if (!layout) {
		if (!tecla_app->main.window) {
			tecla_app->main.window =
				create_window (tecla_app, &tecla_app->main.view);
			g_signal_connect (tecla_app, "window-removed",
					  G_CALLBACK (main_window_removed_cb),
					  NULL);
		}

		if (!tecla_app->observer) {
			tecla_app->observer = tecla_keymap_observer_new ();
			g_signal_connect (tecla_app->observer, "notify::keymap",
					  G_CALLBACK (observer_keymap_notify_cb), app);
			g_signal_connect (tecla_app->observer, "notify::group",
					  G_CALLBACK (observer_keymap_group_cb), app);
		}

		gtk_window_present (tecla_app->main.window);
	} else {
		TeclaInstance *instance = g_new0 (TeclaInstance, 1);

		instance->window = create_window (tecla_app, &instance->view);

		instance->model =
			tecla_model_new_from_layout_name (layout);

		if (instance->model) {
			connect_model (instance->window,
				       instance->view,
				       instance->model);
			update_title (instance->window, instance->model);
		}

#ifdef GDK_WINDOWING_WAYLAND
		if (parent_handle &&
		    GDK_IS_WAYLAND_DISPLAY (gtk_widget_get_display (GTK_WIDGET (instance->window)))) {
			GdkSurface *surface;

			gtk_widget_realize (GTK_WIDGET (instance->window));
			surface = gtk_native_get_surface (GTK_NATIVE (instance->window));
			gdk_wayland_toplevel_set_transient_for_exported (GDK_TOPLEVEL (surface),
									 parent_handle);
		}
#endif

		instance->remove_handler_id =
			g_signal_connect (tecla_app, "window-removed",
					  G_CALLBACK (window_removed_cb),
					  instance);

		tecla_app->instances =
			g_list_prepend (tecla_app->instances, instance);

		gtk_window_present (instance->window);
	}
}

static void
tecla_application_class_init (TeclaApplicationClass *klass)
{
	GApplicationClass *application_class = G_APPLICATION_CLASS (klass);

	application_class->command_line = tecla_application_command_line;
	application_class->activate = tecla_application_activate;
	application_class->handle_local_options = tecla_application_handle_local_options;
}

static void
tecla_application_init (TeclaApplication *app)
{
	gtk_window_set_default_icon_name ("org.gnome.Tecla");
	g_application_add_main_option_entries (G_APPLICATION (app), all_options);
}

GApplication *
tecla_application_new (void)
{
	return g_object_new (TECLA_TYPE_APPLICATION,
			     "application-id", "org.gnome.Tecla",
			     "flags", G_APPLICATION_HANDLES_COMMAND_LINE,
			     NULL);
}
