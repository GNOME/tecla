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

#include "tecla-application.h"

#include "tecla-model.h"
#include "tecla-view.h"

#include <stdlib.h>

struct _TeclaApplication
{
	GtkApplication parent_instance;
	GtkWindow *main_window;
	TeclaView *main_view;
	TeclaModel *main_model;
	gchar *layout;
};

G_DEFINE_TYPE (TeclaApplication, tecla_application, GTK_TYPE_APPLICATION)

static int
tecla_application_command_line (GApplication            *app,
				GApplicationCommandLine *cl)
{
	TeclaApplication *tecla_app = TECLA_APPLICATION (app);
	g_autofree GStrv argv = NULL;
	int argc;

	argv = g_application_command_line_get_arguments (cl, &argc);

	if (argc > 1) {
		g_free (tecla_app->layout);
		tecla_app->layout = g_strdup (argv[1]);
	}

	g_application_activate (app);

	return EXIT_SUCCESS;
}

static GtkWindow *
create_window (TeclaApplication  *app,
	       TeclaView        **view)
{
	g_autoptr (GtkBuilder) builder = NULL;
	GtkWindow *window;

	g_type_ensure (TECLA_TYPE_VIEW);

	builder = gtk_builder_new ();
	gtk_builder_add_from_resource (builder,
				       "/org/gnome/tecla/tecla-window.ui",
				       NULL);

	window = GTK_WINDOW (gtk_builder_get_object (builder, "window"));
	gtk_application_add_window (GTK_APPLICATION (app), window);
	//gtk_window_set_title (GTK_WINDOW (window), "Keyboard Layout");

	if (view)
		*view = TECLA_VIEW (gtk_builder_get_object (builder, "view"));

	return window;
}

static void
level_notify_cb (TeclaView  *view,
		 GParamSpec *pspec,
		 TeclaModel *model)
{
	int level;

	level = tecla_view_get_current_level (view);
	tecla_model_set_level (model, level);
}

static void
connect_model (TeclaView *view,
	       TeclaModel *model)
{
	tecla_view_set_model (view, model);
	g_signal_connect_object (view, "notify::level",
				 G_CALLBACK (level_notify_cb),
				 model, 0);
}

static void
tecla_application_activate (GApplication *app)
{
	TeclaApplication *tecla_app = TECLA_APPLICATION (app);
	g_autoptr (TeclaModel) model = NULL;

	if (tecla_app->layout) {
		GtkWindow *window;
		TeclaView *view;

		window = create_window (tecla_app, &view);
		model = tecla_model_new_from_layout_name (tecla_app->layout);
		connect_model (view, model);
		g_clear_pointer (&tecla_app->layout, g_free);

		gtk_window_present (window);
	} else {
		if (!tecla_app->main_window) {
			tecla_app->main_window =
				create_window (tecla_app, &tecla_app->main_view);
			model = tecla_model_new_from_layout_name ("us");
			connect_model (tecla_app->main_view, model);
		}

		gtk_window_present (tecla_app->main_window);
	}
}

static void
tecla_application_class_init (TeclaApplicationClass *klass)
{
	GApplicationClass *application_class = G_APPLICATION_CLASS (klass);

	application_class->command_line = tecla_application_command_line;
	application_class->activate = tecla_application_activate;
}

static void
tecla_application_init (TeclaApplication *app)
{
}

GApplication *
tecla_application_new (void)
{
	return g_object_new (TECLA_TYPE_APPLICATION,
			     "application-id", "org.gnome.Tecla",
			     "flags", G_APPLICATION_HANDLES_COMMAND_LINE,
			     NULL);
}
