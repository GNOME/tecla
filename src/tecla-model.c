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

#include "tecla-util.h"

struct _TeclaModel
{
	GObject parent_instance;
	struct xkb_keymap *xkb_keymap;
	int group;
};

enum
{
	PROP_0,
	PROP_NAME,
	N_PROPS
};

static GParamSpec *props[N_PROPS] = { 0, };

enum
{
	CHANGED,
	N_SIGNALS,
};

static guint signals[N_SIGNALS] = { 0, };

G_DEFINE_TYPE (TeclaModel, tecla_model, G_TYPE_OBJECT)

static void
tecla_model_get_property (GObject    *object,
			  guint       prop_id,
			  GValue     *value,
			  GParamSpec *pspec)
{
	TeclaModel *model = TECLA_MODEL (object);

	switch (prop_id) {
	case PROP_NAME:
		g_value_set_string (value, tecla_model_get_name (model));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}

static void
tecla_model_class_init (TeclaModelClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	object_class->get_property = tecla_model_get_property;

	signals[CHANGED] =
		g_signal_new ("changed",
			      G_OBJECT_CLASS_TYPE (object_class),
			      G_SIGNAL_RUN_LAST,
			      0, NULL, NULL, NULL,
			      G_TYPE_NONE, 0);

	props[PROP_NAME] =
		g_param_spec_string ("name",
				     "Name",
				     "Name",
				     NULL,
				     G_PARAM_READABLE);

	g_object_class_install_properties (object_class, N_PROPS, props);
}

static void
tecla_model_init (TeclaModel *model)
{
}

static struct {
        gunichar ch;
        const char *nick;
} notable_chars[] = {
        { 0x00a, "⍽"      }, /* NO-BREAK SPACE */
        { 0x00ad, "SHY"   }, /* SOFT HYPHEN */
        { 0x034f, "CGJ"   }, /* COMBINING GRAPHEME JOINER */
        { 0x061c, "ALM"   }, /* ARABIC LETTER MARK */
        { 0x200b, "ZWS"   }, /* ZERO WIDTH SPACE */
        { 0x200c, "ZWNJ"  }, /* ZERO WIDTH NON-JOINER */
        { 0x200d, "ZWJ"   }, /* ZERO WIDTH JOINER */
        { 0x200e, "LRM"   }, /* LEFT-TO-RIGHT MARK */
        { 0x200f, "RLM"   }, /* RIGHT-TO-LEFT MARK */
        { 0x2028, "LS"    }, /* LINE SEPARATOR */
        { 0x2029, "PS"    }, /* PARAGRAPH SEPARATOR */
        { 0x202a, "LRE"   }, /* LEFT-TO-RIGHT EMBEDDING */
        { 0x202b, "RLE"   }, /* RIGHT-TO-LEFT EMBEDDING */
        { 0x202c, "PDF"   }, /* POP DIRECTIONAL FORMATTING */
        { 0x202d, "LRO"   }, /* LEFT-TO-RIGHT OVERRIDE */
        { 0x202e, "RLO"   }, /* RIGHT-TO-LEFT OVERRIDE */
        { 0x202f, "⍽"     }, /* NARROW NO-BREAK SPACE */
        { 0x2060, "WJ"    }, /* WORD JOINER */
        { 0x2061, "FA"    }, /* FUNCTION APPLICATION */
        { 0x2062, "IT"    }, /* INVISIBLE TIMES */
        { 0x2063, "IS"    }, /* INVISIBLE SEPARATOR */
        { 0x2066, "LRI"   }, /* LEFT-TO-RIGHT ISOLATE */
        { 0x2067, "RLI"   }, /* RIGHT-TO-LEFT ISOLATE */
        { 0x2068, "FSI"   }, /* FIRST STRONG ISOLATE */
        { 0x2069, "PDI"   }, /* POP DIRECTIONAL ISOLATE */
        { 0xfeff, "ZWNBS" }, /* ZERO WIDTH NO-BREAK SPACE */
};

static const gchar *
get_unicode_nick (gunichar ch)
{
        for (gsize i = 0; i < G_N_ELEMENTS (notable_chars); i++) {
                if (ch < notable_chars[i].ch)
                        return NULL;

                if (ch == notable_chars[i].ch)
                        return notable_chars[i].nick;
        }

        return NULL;
}

static gchar *
get_key_label (xkb_keysym_t key)
{
	const gchar *label = NULL;
	gchar buf[5];
	gunichar uc;

	switch (key) {
	case GDK_KEY_Mode_switch:
		label = "";
		break;

	case GDK_KEY_ISO_Level3_Shift:
		label = "⎇";
		break;

	case GDK_KEY_ISO_Level5_Shift:
	case GDK_KEY_ISO_Level5_Latch:
	case GDK_KEY_ISO_Level5_Lock:
		label = "⎇5";
		break;

	case GDK_KEY_Delete:
		label = "⌦";
		break;

	case GDK_KEY_BackSpace:
		label = "⌫";
		break;

	case GDK_KEY_space:
		label = "";
		break;

	case GDK_KEY_dead_grave:
		label = "◌̀";
		break;

	case GDK_KEY_dead_abovecomma:
		label = "̓◌̓";
		break;

	case GDK_KEY_dead_abovereversedcomma:
		label = "̔◌̔";
		break;

	case GDK_KEY_dead_acute:
		label = "◌́";
		break;

	case GDK_KEY_dead_circumflex:
		label = "◌̂";
		break;

	case GDK_KEY_dead_tilde:
		label = "◌̃";
		break;

	case GDK_KEY_dead_macron:
		label = "◌̄";
		break;

	case GDK_KEY_dead_breve:
		label = "◌̆";
		break;

	case GDK_KEY_dead_abovedot:
		label = "◌̇";
		break;

	case GDK_KEY_dead_diaeresis:
		label = "◌̈";
		break;

	case GDK_KEY_dead_abovering:
		label = "◌̊";
		break;

	case GDK_KEY_dead_doubleacute:
		label = "◌̋";
		break;

	case GDK_KEY_dead_caron:
		label = "◌̌";
		break;

	case GDK_KEY_dead_cedilla:
		label = "◌̧";
		break;

	case GDK_KEY_dead_ogonek:
		label = "◌̨";
		break;

	case GDK_KEY_dead_belowdot:
		label = "◌̣";
		break;

	case GDK_KEY_dead_hook:
		label = "◌̉";
		break;

	case GDK_KEY_dead_horn:
		label = "◌̛";
		break;

	case GDK_KEY_dead_stroke:
		label = "◌̵ ";
		break;

	case GDK_KEY_dead_hamza:
		label = "ء";
		break;

	case GDK_KEY_horizconnector:
		label = "";
		break;

	case GDK_KEY_dead_belowcomma:
		label = "◌̦";
		break;

	case GDK_KEY_dead_iota:
		label = "◌ͅ";
		break;

	case GDK_KEY_dead_doublegrave:
		label = "◌̏";
		break;

	case GDK_KEY_dead_belowring:
		label = "◌̥";
		break;

	case GDK_KEY_dead_belowmacron:
		label = "◌̱";
		break;

	case GDK_KEY_dead_belowcircumflex:
		label = "◌̭";
		break;

	case GDK_KEY_dead_belowtilde:
		label = "◌̰";
		break;

	case GDK_KEY_dead_belowbreve:
		label = "◌̮";
		break;

	case GDK_KEY_dead_belowdiaeresis:
		label = "◌̤";
		break;

	case GDK_KEY_dead_lowline:
		label = "◌̲";
		break;

	case GDK_KEY_dead_aboveverticalline:
		label = "◌̍ ";
		break;

	case GDK_KEY_dead_belowverticalline:
		label = "◌̩";
		break;

	case GDK_KEY_dead_longsolidusoverlay:
		label = "◌̸ ";
		break;

	case GDK_KEY_dead_voiced_sound:
		label = "◌゙";
		break;

	case GDK_KEY_dead_a:
		label = "◌ͣ";
		break;

	case GDK_KEY_dead_e:
		label = "◌ͤ";
		break;

	case GDK_KEY_dead_i:
		label = "◌ͥ";
		break;

	case GDK_KEY_dead_o:
		label = "◌ͦ";
		break;

	case GDK_KEY_dead_u:
		label = "◌ͧ";
		break;

	case GDK_KEY_dead_small_schwa:
		label = "◌ᷪ";
		break;

	case GDK_KEY_dead_greek:
		label = "a→α";
		break;

	case GDK_KEY_dead_currency:
		label = "e→€";
		break;

	case GDK_KEY_Multi_key:
		label = "⎄";
		break;

	case GDK_KEY_ISO_Enter:
	case GDK_KEY_Return:
		label = "⏎";
		break;

	case GDK_KEY_Shift_L:
	case GDK_KEY_Shift_R:
		label = "⬆";
		break;

	case GDK_KEY_Caps_Lock:
		label = "";
		break;

	case GDK_KEY_Tab:
		label = "⭾";
		break;

	case GDK_KEY_ISO_Left_Tab:
		label = "⭰";
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

	case GDK_KEY_Left:
		label = "⇠";
		break;

	case GDK_KEY_Right:
		label = "⇢";
		break;

	case GDK_KEY_Up:
		label = "⇡";
		break;

	case GDK_KEY_Down:
		label = "⇣";
		break;

	case GDK_KEY_Escape:
		label = "Esc";
		break;

	case GDK_KEY_Undo:
		label = "↶";
		break;

	case GDK_KEY_Redo:
		label = "↷";
		break;

	default:
		uc = gdk_keyval_to_unicode (key);

		if (uc != 0 && g_unichar_isgraph (uc)) {
			buf[g_unichar_to_utf8 (uc, buf)] = '\0';
			return g_strdup (buf);
		} else {
                        const gchar *nick = get_unicode_nick (uc);
			const gchar *name = gdk_keyval_name (key);

                        if (nick) {
                                label = nick;
                        }
			else if (name) {
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

TeclaModel *
tecla_model_new_from_layout_name (const gchar *name)
{
	TeclaModel *model = NULL;
	struct xkb_context *xkb_context;
	struct xkb_keymap *xkb_keymap;
	g_autofree gchar *layout = NULL;
	const gchar *variant = NULL, *sep;
	struct xkb_rule_names rule_names = {
		.rules = "evdev",
		.model = "pc105",
	};

	sep = strchr (name, '+');
	if (!sep)
		sep = strchr (name, ' ');
	if (!sep)
		sep = strchr (name, '\t');

	if (sep) {
		variant = sep + 1;
		layout = g_strndup (name, sep - name);
	} else {
		layout = g_strdup (name);
	}

	rule_names.layout = layout;
	rule_names.variant = variant;

	xkb_context = tecla_util_create_xkb_context ();
	xkb_keymap = xkb_keymap_new_from_names (xkb_context, &rule_names, 0);
	xkb_context_unref (xkb_context);

	if (xkb_keymap) {
		model = tecla_model_new_from_xkb_keymap (xkb_keymap);
		xkb_keymap_unref (xkb_keymap);
	}

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
			   int          level,
			   const gchar *key)
{
	xkb_keycode_t keycode;
	guint keysym;

	keycode = xkb_keymap_key_by_name (model->xkb_keymap, key);
	keysym = tecla_model_get_keyval (model, level, keycode);

	if (keysym == 0)
		return NULL;

	return get_key_label (keysym);
}

guint
tecla_model_get_keyval (TeclaModel    *model,
			int            level,
			xkb_keycode_t  keycode)
{
	const xkb_keysym_t *syms;
	int n_syms;

	n_syms = xkb_keymap_key_get_syms_by_level (model->xkb_keymap,
						   keycode,
						   model->group,
						   level,
						   &syms);
	if (n_syms == 0)
		return 0;

	return syms[0];
}

const gchar *
tecla_model_get_name (TeclaModel *model)
{
	return xkb_keymap_layout_get_name (model->xkb_keymap, model->group);
}

void
tecla_model_set_group (TeclaModel *model,
		       int         group)
{
	model->group = group;
	g_object_notify (G_OBJECT (model), "name");
	g_signal_emit (model, signals[CHANGED], 0);
}
