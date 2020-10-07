/*
 * gawcursor.c - cursors in drawing area
 * 
 * include LICENSE
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gtk/gtk.h>
#include <string.h>
// #include <stdlib.h>
#include <math.h>

#include <gaw.h>
 
#ifdef TRACE_MEM
#include <tracemem.h>
#endif
        


void cu_cursor_clear(UserData *ud, int i)
{
   AWCursor *csp = ud->cursors[i];
 
   if ( csp->shown == 0 ) {
      return;
   }
   ud->cursors[2]->shown = 0;
   csp->shown = 0;
   /* undraw the cursor line */
   g_list_foreach(ud->panelList, (GFunc) pa_panel_drawing_redraw, NULL);

   ap_xmeasure_button_show(ud);
   ap_mbtn_update_all(ud);
}

void cu_clear_cursors(UserData *ud)
{
   int i;

   for ( i = 0 ; i < AW_NX_MBTN ; i++ ) {
      ud->cursors[i]->shown = 0;
   }
   ap_xmeasure_button_show(ud);
   ap_mbtn_update_all(ud);
}


void cu_update_xcursor(WavePanel *wp, AWCursor *csp, int x, int redraw)
{
   UserData  *ud = wp->ud;
   AWCursor *csp1;
   GawLabels *lbx = wp->ud->xLabels;

   csp->shown = 1;
   int zoom = csp->zoom;
   if ( csp->zoom ) {
      /* put the cursor at prev xval value */
      csp->zoom = 0;
      csp->x = al_label_val2x(lbx, csp->xval);
   } else {
      csp->xval = al_label_x2val(lbx, x);
      csp->x = x;
   }
   msg_dbg("xval %f x %d zoom %d 0x%lx", csp->xval, csp->x, zoom, (unsigned long ) csp);

   if ( redraw ) {
      /* draw cursor in each panel */
      g_list_foreach(ud->panelList, (GFunc) pa_panel_drawing_redraw, NULL);
   }
   
   csp = ud->cursors[0];
   csp1 = ud->cursors[1];
   if (  csp->shown && csp1->shown ) {
      ud->cursors[2]->shown = 1;
      ud->cursors[2]->x = csp1->x -  csp->x;
      ud->cursors[2]->xval = csp1->xval -  csp->xval;
   }

   /* show label */
   ap_xmeasure_button_draw(wp->ud);
   ap_xmeasure_button_show(wp->ud);
   /* update all measurebuttons */
   ap_mbtn_update_all(wp->ud);
}

void cu_display_xcursor(WavePanel *wp, gint button, int x, int redraw)
{
   UserData  *ud = wp->ud;
   AWCursor *csp;

//   msg_dbg("display at x %d", x);
   if ( button < 1 || button > 2 ) {
      return;
   }
   csp = ud->cursors[button - 1];
   if ( x == csp->x && csp->shown ) {
      return;
   }
   cu_update_xcursor(wp, csp, x, redraw);
}

void
cu_cursor_clear_cursor0_gaction (GSimpleAction *action, GVariant *param, gpointer user_data )
{
   UserData *ud = (UserData *) user_data;
   cu_cursor_clear(ud, 0);
}

void
cu_cursor_clear_cursor1_gaction (GSimpleAction *action, GVariant *param, gpointer user_data )
{
   UserData *ud = (UserData *) user_data;
   cu_cursor_clear(ud, 1);
}

void cu_cursor_clear_cursors_gaction (GSimpleAction *action, GVariant *param, gpointer user_data )
{
   UserData *ud = (UserData *) user_data;
   cu_clear_cursors( ud );
}

void cu_cursor_set_cursor0_gaction (GSimpleAction *action, GVariant *param, gpointer user_data )
{
   UserData *ud = (UserData *) user_data;
   WavePanel *wp = (WavePanel *) g_list_nth_data (ud->panelList, 0);
   cu_display_xcursor(wp, 1, ud->cursors[0]->x, 1);
}

void cu_cursor_set_cursor1_gaction (GSimpleAction *action, GVariant *param, gpointer user_data )
{
   UserData *ud = (UserData *) user_data;
   WavePanel *wp = (WavePanel *) g_list_nth_data (ud->panelList, 0);
   cu_display_xcursor(wp, 2, ud->cursors[1]->x, 1);
}

void cu_cursor_set_cursors_gaction (GSimpleAction *action, GVariant *param, gpointer user_data )
{
   UserData *ud = (UserData *) user_data;
   WavePanel *wp = (WavePanel *) g_list_nth_data (ud->panelList, 0);
   cu_display_xcursor(wp, 1, ud->cursors[0]->x, 0);
   cu_display_xcursor(wp, 2, ud->cursors[1]->x, 1);
}
