/*
 * gawgrip.c - resizing grip
 * 
 * include LICENSE
 */
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gtk/gtk.h>
#include <string.h>

#include <gaw.h>
 
#ifdef TRACE_MEM
#include <tracemem.h>
#endif
        

int ag_grip_panel_width (UserData *ud, char *text )
{
   int width;

   width = ud->winWidth - 2 * WIN_BORDER_SIZE - 2 - ud->up->lmtableWidth;
   if ( ud->xLabels->changed & CV_SBSHOW ){
      width -= ud->sbSize;
   }
   sprintf (text, _("Panel size w %d h %d"), width, ud->panelHeight);
   return width;
}

/*
 * Resize Grip callback
 */
static gboolean
ag_grip_button_press_cb (GtkWidget *da, GdkEventButton *event, gpointer data )
{
   UserData *ud = (UserData *) data;
   char text[128];
   
   msg_dbg( "button %d state %d, x_root = %d, x = %d,  y = %d",
           event->button, event->state, (int) event->x_root, (int) event->x, (int) event->y );

   if (event->button == 1) {
      ud->mouseState = M_CURSOR_DRAG;
      ud->gripdelta = (int) event->x_root;
      
      ag_grip_panel_width ( ud, text ); 
      gtk_statusbar_push (GTK_STATUSBAR(ud->statusbar), 1, text);
   }
   return TRUE;
}

static gboolean
ag_grip_button_release_cb (GtkWidget *da, GdkEventButton *event, gpointer data )
{
   UserData *ud = (UserData *) data;
   
   msg_dbg( "button %d state %d, x = %d, y = %d",
           event->button, event->state, (int) event->x, (int) event->y );

   gtk_statusbar_pop (GTK_STATUSBAR(ud->statusbar), 1);

   ud->mouseState = M_NONE;
   gdk_window_set_cursor(gtk_widget_get_window (da), NULL);

   return TRUE;
}

/*
 * drawing area mouse motion handler
 */
static gboolean
ag_grip_motion_cb (GtkWidget *widget, GdkEventMotion *event, gpointer data )
{
   UserData *ud = (UserData *) data;
   UserPrefs *up = ud->up;
   
   char text[128];
   msg_dbg( "state %d, x_root = %d, x = %d", ud->mouseState, (int) event->x_root, (int) event->x );

   if (ud->mouseState == M_CURSOR_DRAG ) {
      int dx = (int) event->x_root - ud->gripdelta;
      ud->gripdelta = (int) event->x_root ;
      up->lmtableWidth += dx ;
      int width = ag_grip_panel_width ( ud, text ); 

      if ( width >= up->minPanelWidth ) {
	 g_list_foreach(ud->panelList, (GFunc) pa_panel_lmscroll_win_set_size_request, NULL);

	gtk_label_set_text ( GTK_LABEL(ud->statusLabel), text);
      }
   }
   return TRUE;
}

static gboolean
ag_grip_crossing_cb (GtkWidget *widget, GdkEventCrossing *event, gpointer data )
{
//   msg_dbg(" called %d", event->type );
   if (event->type == GDK_ENTER_NOTIFY ) {
      GdkCursor *cursor = gdk_cursor_new(GDK_SB_H_DOUBLE_ARROW);
      gdk_window_set_cursor(gtk_widget_get_window (widget), cursor);
      g_object_unref(cursor);
   }
   return TRUE;
}


static gboolean
ag_grip_draw_cb (GtkWidget *da, cairo_t *cr, gpointer data )
{
   int w = gtk_widget_get_allocated_width(da);
   int h = gtk_widget_get_allocated_height(da);

   msg_dbg(" called w %d, h %d",  w, h );
   GtkStyleContext *styleCtxt = gtk_widget_get_style_context (da);
   gtk_style_context_add_class ( styleCtxt, GTK_STYLE_CLASS_GRIP); 
   gtk_render_handle (styleCtxt, cr, 0.0, 0.0, w, h );

   return TRUE;
}

GtkWidget *
ag_grip_new( UserData *ud )
{
   GtkWidget *da;

   /* drawing area for drawing sizing */
   da = gtk_drawing_area_new ();
   gtk_widget_set_name( da, "gawgrip");
   gtk_widget_add_events (da, GDK_EXPOSURE_MASK   |
			  GDK_BUTTON_PRESS_MASK   |
			  GDK_BUTTON_RELEASE_MASK |
			  GDK_ENTER_NOTIFY_MASK   |
			  GDK_BUTTON1_MOTION_MASK );
   g_signal_connect (da, "draw", 
		     G_CALLBACK (ag_grip_draw_cb), (gpointer) ud);
   g_signal_connect (da, "button-press-event",
		     G_CALLBACK (ag_grip_button_press_cb), (gpointer) ud);
   g_signal_connect( da, "motion-notify-event",
		     G_CALLBACK ( ag_grip_motion_cb), (gpointer) ud);
   g_signal_connect( da, "button-release-event",
		     G_CALLBACK ( ag_grip_button_release_cb),  (gpointer) ud);
   g_signal_connect( da, "enter-notify-event",
		     G_CALLBACK ( ag_grip_crossing_cb), (gpointer) ud);
   gtk_widget_show (da);
   gtk_widget_set_size_request (GTK_WIDGET(da), 8, 8);

   return da;
}
