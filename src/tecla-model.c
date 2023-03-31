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

#include "tecla-model.h"

struct _TeclaModel
{
	GObject parent_instance;
	struct xkb_keymap *xkb_keymap;
};

G_DEFINE_TYPE (TeclaModel, tecla_model, G_TYPE_OBJECT)

static void
tecla_model_class_init (TeclaModelClass *klass)
{
}

static void
tecla_model_init (TeclaModel *model)
{
}

static gchar *
get_key_label (xkb_keysym_t key)
{
	const gchar *label = NULL;
	gchar buf[5];
	gunichar uc;

	switch (key) {
	case GDK_KEY_Mode_switch:
	case GDK_KEY_ISO_Level3_Shift:
		label = "";
		break;

	case GDK_KEY_BackSpace:
		label = "";
		break;

	case GDK_KEY_space:
		label = "";
		break;

	case GDK_KEY_dead_grave:
		label = "ˋ";
		break;

	case GDK_KEY_dead_abovecomma:
		label = "̓";
		break;

	case GDK_KEY_dead_abovereversedcomma:
		label = "̔";
		break;

	case GDK_KEY_dead_acute:
		label = "ˊ";
		break;

	case GDK_KEY_dead_circumflex:
		label = "ˆ";
		break;

	case GDK_KEY_dead_tilde:
		label = "~";
		break;

	case GDK_KEY_dead_macron:
		label = "ˉ";
		break;

	case GDK_KEY_dead_breve:
		label = "˘";
		break;

	case GDK_KEY_dead_abovedot:
		label = "˙";
		break;

	case GDK_KEY_dead_diaeresis:
		label = "¨";
		break;

	case GDK_KEY_dead_abovering:
		label = "˚";
		break;

	case GDK_KEY_dead_doubleacute:
		label = "˝";
		break;

	case GDK_KEY_dead_caron:
		label = "ˇ";
		break;

	case GDK_KEY_dead_cedilla:
		label = "¸";
		break;

	case GDK_KEY_dead_ogonek:
		label = "˛";
		break;

	case GDK_KEY_dead_belowdot:
		label = " ̣";
		break;

	case GDK_KEY_dead_hook:
		label = "̉";
		break;

	case GDK_KEY_dead_horn:
		label = "̛";
		break;

	case GDK_KEY_dead_stroke:
		label = "̵";
		break;

	case GDK_KEY_horizconnector:
		label = "";
		break;

	case GDK_KEY_Multi_key:
		label = "";
		break;

	case GDK_KEY_Return:
		label = "";
		break;

	case GDK_KEY_Shift_L:
	case GDK_KEY_Shift_R:
		label = "";
		break;

	case GDK_KEY_Caps_Lock:
		label = "";
		break;

	case GDK_KEY_Tab:
	case GDK_KEY_ISO_Left_Tab:
		label = "";
		break;

	case GDK_KEY_Alt_L:
	case GDK_KEY_Alt_R:
		label = "";
		break;

	case GDK_KEY_Super_L:
	case GDK_KEY_Super_R:
		label = "";
		break;

	case GDK_KEY_Control_L:
	case GDK_KEY_Control_R:
		label = "";
		break;

	case GDK_KEY_Meta_L:
	case GDK_KEY_Meta_R:
		label = "";
		break;

	case GDK_KEY_Menu:
		label = "";
		break;

	case GDK_KEY_VoidSymbol:
		label = "";
		break;

	case GDK_KEY_nobreakspace:
		label = "";
		break;

	default:
		uc = gdk_keyval_to_unicode (key);

		if (uc != 0 && g_unichar_isgraph (uc)) {
			buf[g_unichar_to_utf8 (uc, buf)] = '\0';
			return g_strdup (buf);
		} else {
			const gchar *name = gdk_keyval_name (key);

			if (name) {
				g_autofree gchar *fixed_name = NULL;
				gchar *p;

				fixed_name = g_strdup (name);

				/* Replace underscores with spaces */
				for (p = fixed_name; *p; p++)
					if (*p == '_')
						*p = ' ';
				/* Get rid of scary ISO_ prefix */
				if (g_strstr_len (fixed_name, -1, "ISO "))
					return g_strdup (fixed_name + 4);
				else
					return g_strdup (fixed_name);
			} else {
				return g_strdup ("");
			}
		}

		break;
	}

	return g_strdup (label);
}

TeclaModel *
tecla_model_new_from_xkb_keymap (struct xkb_keymap *xkb_keymap)
{
	TeclaModel *model;

	model = g_object_new (TECLA_TYPE_MODEL, NULL);
	model->xkb_keymap = xkb_keymap_ref (xkb_keymap);

	return model;
}

static struct xkb_context *
create_xkb_context (void)
{
  struct xkb_context *ctx;
  char xdg[PATH_MAX] = {0};
  const char *env;

  /*
   * We can only append search paths in libxkbcommon, so we start with an
   * empty set, then add the XDG dir, then add the default search paths.
   */
  ctx = xkb_context_new (XKB_CONTEXT_NO_DEFAULT_INCLUDES);

  if ((env = g_getenv ("XDG_CONFIG_HOME")))
    {
      g_snprintf (xdg, sizeof xdg, "%s/xkb", env);
    }
  else if ((env = g_getenv ("HOME")))
    {
      g_snprintf (xdg, sizeof xdg, "%s/.config/xkb", env);
    }

  if (env)
    xkb_context_include_path_append (ctx, xdg);

  xkb_context_include_path_append_default (ctx);

  return ctx;
}

TeclaModel *
tecla_model_new_from_layout_name (const gchar *layout)
{
	TeclaModel *model;
	struct xkb_context *xkb_context;
	struct xkb_keymap *xkb_keymap;
	struct xkb_rule_names names = {
		.rules = "evdev",
		.model = "pc105",
		.layout = layout,
	};

	xkb_context = create_xkb_context ();
	xkb_keymap = xkb_keymap_new_from_names (xkb_context, &names, 0);
	xkb_context_unref (xkb_context);

	model = tecla_model_new_from_xkb_keymap (xkb_keymap);
	xkb_keymap_unref (xkb_keymap);

	return model;
}

const gchar *
tecla_model_get_keycode_key (TeclaModel    *model,
			     xkb_keycode_t  keycode)
{
	return xkb_keymap_key_get_name (model->xkb_keymap, keycode);
}

xkb_keycode_t
tecla_model_get_key_keycode (TeclaModel  *model,
			     const gchar *key)
{
	return xkb_keymap_key_by_name (model->xkb_keymap, key);
}

gchar *
tecla_model_get_key_label (TeclaModel  *model,
			   const gchar *key)
{
	xkb_keycode_t keycode;
	const xkb_keysym_t *syms;
	int n_syms;

	keycode = xkb_keymap_key_by_name (model->xkb_keymap, key);

	n_syms = xkb_keymap_key_get_syms_by_level (model->xkb_keymap,
						   keycode,
						   0,
						   0,
						   &syms);
	if (n_syms == 0)
		return NULL;

	return get_key_label (syms[0]);
}
