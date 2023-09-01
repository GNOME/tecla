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

#include "tecla-util.h"

#include <gtk/gtk.h>

struct xkb_context *
tecla_util_create_xkb_context (void)
{
  struct xkb_context *ctx;
  char xdg[1024] = {0};
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
