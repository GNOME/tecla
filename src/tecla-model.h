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

#pragma once

#define TECLA_TYPE_MODEL (tecla_model_get_type ())
G_DECLARE_FINAL_TYPE (TeclaModel, tecla_model, TECLA, MODEL, GObject)

TeclaModel * tecla_model_new_from_xkb_keymap (struct xkb_keymap *xkb_keymap);

TeclaModel * tecla_model_new_from_layout_name (const gchar *layout);

const gchar * tecla_model_get_keycode_key (TeclaModel    *model,
					   xkb_keycode_t  keycode);

xkb_keycode_t tecla_model_get_key_keycode (TeclaModel  *model,
					   const gchar *key);

gchar * tecla_model_get_key_label (TeclaModel  *model,
				   int          level,
				   const gchar *key);

guint tecla_model_get_keyval (TeclaModel    *model,
			      int            level,
			      xkb_keycode_t  keycode);

const gchar * tecla_model_get_name (TeclaModel *model);

void tecla_model_set_group (TeclaModel *model,
			    int         group);
