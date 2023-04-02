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

#include "tecla-keymap-observer.h"

#include <gdk/gdk.h>

#ifdef GDK_WINDOWING_WAYLAND
#include <gdk/wayland/gdkwayland.h>
#include <wayland-client.h>
#endif

#include "tecla-util.h"

struct _TeclaKeymapObserver
{
	GObject parent_instance;
#ifdef GDK_WINDOWING_WAYLAND
	uint32_t seat_id;
	struct wl_registry *wl_registry;
	struct wl_seat *wl_seat;
	struct wl_keyboard *wl_keyboard;
#endif

	struct xkb_keymap *xkb_keymap;
	uint32_t group;
};

enum
{
	PROP_0,
	PROP_KEYMAP,
	PROP_GROUP,
	N_PROPS,
};

static GParamSpec *props[N_PROPS] = { 0, };

G_DEFINE_TYPE (TeclaKeymapObserver, tecla_keymap_observer, G_TYPE_OBJECT)

#ifdef GDK_WINDOWING_WAYLAND

static void
dummy (void)
{
}

static void
keyboard_keymap (void               *data,
		 struct wl_keyboard *wl_keyboard,
		 uint32_t            format,
		 int32_t             fd,
		 uint32_t            size)
{
	TeclaKeymapObserver *observer = data;
	g_autoptr (GMappedFile) mapped_file = NULL;
	struct xkb_context *xkb_context;

	mapped_file = g_mapped_file_new_from_fd (fd, FALSE, NULL);
	if (!mapped_file)
		return;

	if (observer->xkb_keymap)
		xkb_keymap_unref (observer->xkb_keymap);

	xkb_context = tecla_util_create_xkb_context ();
	observer->xkb_keymap =
		xkb_keymap_new_from_string (xkb_context,
					    g_mapped_file_get_contents (mapped_file),
					    format,
					    XKB_KEYMAP_COMPILE_NO_FLAGS);
	xkb_context_unref (xkb_context);
	close (fd);

	g_object_notify (G_OBJECT (observer), "keymap");
}

static void
keyboard_modifiers (void               *data,
		    struct wl_keyboard *wl_keyboard,
		    uint32_t            serial,
		    uint32_t            mods_depressed,
		    uint32_t            mods_latched,
		    uint32_t            mods_locked,
		    uint32_t            group)
{
	TeclaKeymapObserver *observer = data;

	if (observer->group == group)
		return;

	observer->group = group;
	g_object_notify (G_OBJECT (observer), "group");
}

static struct wl_keyboard_listener keyboard_listener = {
	keyboard_keymap,
	(void (*) (void*, struct wl_keyboard*, uint32_t, struct wl_surface*, struct wl_array*)) dummy,
	(void (*) (void*, struct wl_keyboard*, uint32_t, struct wl_surface*)) dummy,
	(void (*) (void*, struct wl_keyboard*, uint32_t, uint32_t, uint32_t, uint32_t)) dummy,
	keyboard_modifiers,
	(void (*)(void *, struct wl_keyboard *, int32_t,  int32_t)) dummy,
};

static void
seat_capabilities (void           *data,
		   struct wl_seat *wl_seat,
		   uint32_t        capabilities)
{
	TeclaKeymapObserver *observer = data;

	if (!observer->wl_keyboard &&
	    (capabilities & WL_SEAT_CAPABILITY_KEYBOARD) != 0) {
		observer->wl_keyboard = wl_seat_get_keyboard (wl_seat);
		wl_keyboard_add_listener (observer->wl_keyboard, &keyboard_listener, observer);
	} else if (observer->wl_keyboard &&
		   (capabilities & WL_SEAT_CAPABILITY_KEYBOARD) == 0) {
		g_clear_pointer (&observer->wl_keyboard, wl_keyboard_destroy);
	}
}

static struct wl_seat_listener seat_listener = {
	seat_capabilities,
	(void (*)(void *, struct wl_seat *, const char *)) dummy,
};

static void
registry_global (void               *data,
		 struct wl_registry *wl_registry,
		 uint32_t            name,
		 const char         *interface,
		 uint32_t            version)
{
	TeclaKeymapObserver *observer = data;

	if (strcmp (interface, "wl_seat") != 0)
		return;

	observer->wl_seat =
		wl_registry_bind (wl_registry,
				  name, &wl_seat_interface,
				  1);
	wl_seat_add_listener (observer->wl_seat, &seat_listener, observer);
}

static void
registry_global_remove (void               *data,
			struct wl_registry *wl_registry,
			uint32_t            name)
{
	TeclaKeymapObserver *observer = data;

	if (name != observer->seat_id)
		return;

	g_clear_pointer (&observer->wl_keyboard, wl_keyboard_destroy);
	g_clear_pointer (&observer->wl_seat, wl_seat_destroy);
}

static struct wl_registry_listener registry_listener = {
	registry_global,
	registry_global_remove,
};
#endif

static void
tecla_keymap_observer_finalize (GObject *object)
{
	TeclaKeymapObserver *observer = TECLA_KEYMAP_OBSERVER (object);

#ifdef GDK_WINDOWING_WAYLAND
	g_clear_pointer (&observer->wl_keyboard, wl_keyboard_destroy);
	g_clear_pointer (&observer->wl_seat, wl_seat_destroy);
	g_clear_pointer (&observer->wl_registry, wl_registry_destroy);
#endif

	g_clear_pointer (&observer->xkb_keymap, xkb_keymap_unref);

	G_OBJECT_CLASS (tecla_keymap_observer_parent_class)->finalize (object);
}

static void
tecla_keymap_observer_get_property (GObject    *object,
				    guint       prop_id,
				    GValue     *value,
				    GParamSpec *pspec)
{
	TeclaKeymapObserver *observer = TECLA_KEYMAP_OBSERVER (object);

	switch (prop_id) {
	case PROP_KEYMAP:
		g_value_set_pointer (value, observer->xkb_keymap);
		break;
	case PROP_GROUP:
		g_value_set_int (value, observer->group);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}

static void
tecla_keymap_observer_class_init (TeclaKeymapObserverClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	object_class->finalize = tecla_keymap_observer_finalize;
	object_class->get_property = tecla_keymap_observer_get_property;

	props[PROP_KEYMAP] =
		g_param_spec_pointer ("keymap",
				      "Keymap",
				      "Keymap",
				      G_PARAM_READABLE);
	props[PROP_GROUP] =
		g_param_spec_int ("group",
				  "Group",
				  "Group",
				  0, G_MAXINT, 0,
				  G_PARAM_READABLE);

	g_object_class_install_properties (object_class, N_PROPS, props);
}

static void
tecla_keymap_observer_init (TeclaKeymapObserver *observer)
{
	GdkDisplay *display;

	display = gdk_display_get_default ();

#ifdef GDK_WINDOWING_WAYLAND
	if (GDK_IS_WAYLAND_DISPLAY (display)) {
		struct wl_display *wl_display;
		struct wl_registry *wl_registry;

		wl_display = gdk_wayland_display_get_wl_display (display);
		wl_registry = wl_display_get_registry (wl_display);
		wl_registry_add_listener (wl_registry,
					  &registry_listener,
					  observer);
	}
#endif
}

TeclaKeymapObserver *
tecla_keymap_observer_new (void)
{
	return g_object_new (TECLA_TYPE_KEYMAP_OBSERVER, NULL);
}

struct xkb_keymap *
tecla_keymap_observer_get_keymap (TeclaKeymapObserver *observer)
{
	return observer->xkb_keymap;
}

int
tecla_keymap_observer_get_group (TeclaKeymapObserver *observer)
{
	return observer->group;
}
