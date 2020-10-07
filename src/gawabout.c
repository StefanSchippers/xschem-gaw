/*
 * gawabout.c -
 * This file is part of gaw Gtk Analog Wave viewer
 *
 * include LICENSE
 *
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
// #include <stdlib.h>
#include <string.h>

#include <gtk/gtk.h>
// #include <gdk/gdk.h>

#include <msglog.h>
 
#ifdef TRACE_MEM
#include <tracemem.h>
#endif
        

void
gaw_show_about_dialog (GtkWidget *parent)
{
  const gchar *authors[] = {
    "AUTHORS:",
    "Steve Tell <steve@telltronics.org>",
    "Hervé Quillévéré <www.rvq.fr>",
    "",
    "CONTRIBUTORS:",
    "Franck Paugnat franck.paugnat at imag.fr",
    NULL
  };

  const gchar *documenters[] = {
    "Steve Tell <steve@telltronics.org>",
    "Hervé Quillévéré <www.rvq.fr>",
    NULL
  };

  const gchar *translator_credits = "translator-credits";
  const gchar *copyright = "Copyright \xc2\xa9 2007 2014 Hervé Quillévéré";
  const gchar *comments = _("Gtk3 Analog Wave viewer");
  
  const gchar *license =
    "This program is free software; you can redistribute it and/or "
    "modify it under the terms of the GNU General Public License as "
    "published by the Free Software Foundation; either version 2 of "
    "the License, or (at your option) any later version.\n"
    "\n"
    "This program is distributed in the hope that it will be useful, "
    "but WITHOUT ANY WARRANTY; without even the implied warranty of "
    "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU "
    "General Public License for more details.\n"
    "\n"
    "You should have received a copy of the GNU General Public License "
    "along with this program; if not, write to the Free Software "
    "Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA "
    "02111-1307, USA.\n";

  g_return_if_fail (GTK_IS_WIDGET (parent));
  
  gtk_show_about_dialog ( GTK_WINDOW (parent),
  			 "name", "Gtk Analog Wave viewer",
  			 "version", VERSION,
  			 "copyright", copyright,
  			 "comments", comments,
  			 "authors", authors,
  			 "documenters", documenters,
  			 "translator-credits", translator_credits,
  			 "logo-icon-name", "accessories-dictionary",
  			 "license", license,
  			 "wrap-license", TRUE,
			 "screen", gtk_widget_get_screen (parent),
  			 NULL);
}
