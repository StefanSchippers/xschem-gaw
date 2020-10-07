/*
 * gawimg.c - Save image to file
 * 
 * include LICENSE
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <string.h>

#include <gtk/gtk.h>

#include <gaw.h>

#ifdef TRACE_MEM
#include <tracemem.h>
#endif

/*
 * create a button with a label and an imamge from theme
 */

GtkWidget *gawutil_button_new_with_label( gchar *label, gchar *themed_name,
                                     GtkIconSize size)
{
   GtkWidget *button = gtk_button_new_with_label (label);

   GIcon *icon = g_themed_icon_new (themed_name);
   GtkWidget *image = gtk_image_new_from_gicon (icon, size);
   gtk_button_set_image (GTK_BUTTON(button), image );

   g_object_unref (icon);
   return button;
}
 
