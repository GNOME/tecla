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

#include "tecla-model.h"

#pragma once

#define TECLA_TYPE_VIEW (tecla_view_get_type ())
G_DECLARE_FINAL_TYPE (TeclaView, tecla_view,
		      TECLA, VIEW,
		      GtkWidget)

GtkWidget * tecla_view_new (void);

void tecla_view_set_model (TeclaView  *view,
			   TeclaModel *model);

int tecla_view_get_current_level (TeclaView *view);

void tecla_view_set_current_level (TeclaView *view,
				   int        level);

int tecla_view_get_num_levels (TeclaView *view);
