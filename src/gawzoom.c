/*
 * gawzoom.c - Analog waveform viewer
 * zoom stuff
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
        


/*
 * reset the x zoom scale of all panels
 */
gint az_cmd_zoom_absolute(UserData *ud, double start, double end )
{
   double scroll_start, scroll_end;
   GawLabels *lbx = ud->xLabels;
   
   msg_dbg("start %f  end %f", start, end);
   /* set starting drawn x-value */
   if (start > end) {
      int val = start;
      start = end;
      end = val;
   }
   if (start < lbx->min_val) {
      start = lbx->min_val;
   }
   if (end > lbx->max_val) {
      end = lbx->max_val;
   }

   /*
    * Needs tp update_vals for the end of this function 
    */
   al_label_update_vals( lbx, start, end );
   /*
    * Scroll bar always goes from zero to one.
    * Perform an appropriate transform based on lin/log
    */
   if ( lbx->logAxis && lbx->logAble ) {
      scroll_start = (lbx->max_Lval > lbx->min_Lval) ?
    		  	  ( lbx->start_Lval - lbx->min_Lval ) /
    		  	  ( lbx->max_Lval - lbx->min_Lval ) :
    		  	  0.0;
      scroll_end   = (lbx->max_Lval > lbx->min_Lval) ?
    		  	  ( lbx->end_Lval  - lbx->min_Lval ) /
    		  	  ( lbx->max_Lval - lbx->min_Lval ) :
    		  	  0.0;
   } else {
      scroll_start = ( lbx->start_val - lbx->min_val ) /
	 ( lbx->max_val - lbx->min_val );
      scroll_end   = ( lbx->end_val   - lbx->min_val ) /
	 ( lbx->max_val - lbx->min_val );
   }
   if ( ud->xadj == NULL || lbx->wh == 0 ) {
       return 0;
   }
   gdouble page_size;

   page_size = fabs( scroll_end - scroll_start );
   gtk_adjustment_set_page_size (ud->xadj, page_size);
   msg_dbg("scroll_start %f  scroll_end %f", scroll_start, scroll_end);

   gtk_adjustment_set_page_increment(ud->xadj, page_size / 2);
   gtk_adjustment_set_step_increment(ud->xadj, page_size / 100);
   gtk_adjustment_set_value(ud->xadj, scroll_start);
   gtk_adjustment_set_lower(ud->xadj, 0.0);
   gtk_adjustment_set_upper(ud->xadj, 1.0);
   
   gtk_adjustment_value_changed (ud->xadj);

   return 0;
}


void az_zoom_in_gaction (GSimpleAction *action, GVariant *param, gpointer user_data )
{
   UserData *ud = (UserData *) user_data;
   double start;
   double end;
   GawLabels *lbx = ud->xLabels;
   
   if ( lbx->logAxis && lbx->logAble ) {
      start = pow( 10, lbx->start_Lval + ( lbx->end_Lval - lbx->start_Lval ) / 4 );
      end   = pow( 10, lbx->end_Lval   - ( lbx->end_Lval - lbx->start_Lval ) / 4 );
   } else {
      start = lbx->start_val + ( lbx->end_val - lbx->start_val) / 4;
      end   = lbx->end_val   - ( lbx->end_val - lbx->start_val) / 4;
   }

   az_cmd_zoom_absolute(ud, start, end );
}

void
az_pop_zoom_in_gaction (GSimpleAction *action, GVariant *param, gpointer user_data )
{
   WavePanel *wp = (WavePanel *) user_data;
   az_zoom_in_gaction (action, param, wp->ud);
}

void
az_zoom_out_gaction (GSimpleAction *action, GVariant *param, gpointer user_data )
{
   UserData *ud = (UserData *) user_data;
   double start;
   double end;
   GawLabels *lbx = ud->xLabels;
   
   if ( lbx->logAxis && lbx->logAble ) {
      start = pow( 10, lbx->start_Lval - ( lbx->end_Lval - lbx->start_Lval ) / 2 );
      end   = pow( 10, lbx->end_Lval   + ( lbx->end_Lval - lbx->start_Lval ) / 2 );
   } else {
      start = lbx->start_val - ( lbx->end_val - lbx->start_val) / 2;
      end   = lbx->end_val   + ( lbx->end_val - lbx->start_val) / 2;
   }

   az_cmd_zoom_absolute(ud, start, end );
}

void
az_pop_zoom_out_gaction (GSimpleAction *action, GVariant *param, gpointer user_data )
{
   WavePanel *wp = (WavePanel *) user_data;
   az_zoom_out_gaction (action, param, wp->ud);
}

void
az_zoom_cursor0_centered_gaction (GSimpleAction *action, GVariant *param, gpointer user_data )
{
   UserData *ud = (UserData *) user_data;
   double start;
   double end;
   GawLabels *lbx = ud->xLabels;
   AWCursor *csp = ud->cursors[0];
   
   msg_dbg("cursors[0] shown %d 0x%x", csp->shown, (unsigned long) csp );
   if ( csp->shown ) {
      double xval = csp->xval;
      double Lxval = 0;
      
      if ( lbx->logAxis && lbx->logAble ) {
	 Lxval = log10(xval);
	 start = pow( 10, lbx->start_Lval + (( Lxval  - lbx->start_Lval ) / 4) );
	 end   = pow( 10, lbx->end_Lval   + (( Lxval  - lbx->end_Lval   ) / 4) );
      } else {
	 start =  lbx->start_val + ( xval - lbx->start_val ) / 4;
	 end   =  lbx->end_val   + ( xval - lbx->end_val   ) / 4;
      }
      msg_dbg("xval %f Lxval %f start %f end %f", xval, Lxval, start, end);
      
      az_cmd_zoom_absolute(ud, start, end );
      
   } else {
       msg_info(_( "\n\nThis function will zoom centered on cursor 0\n"
		 "But you need first set the cursor 0 by left click in DrawingArea\n") );
   }
}

void az_zoom_cursors_gaction (GSimpleAction *action, GVariant *param, gpointer user_data )
{
   UserData *ud = (UserData *) user_data;
   double start;
   double end;
   
   if ( ud->cursors[0]->shown && ud->cursors[1]->shown ) {
      if( ud->cursors[0]->xval <  ud->cursors[1]->xval) {
        start = ud->cursors[0]->xval;
        end = ud->cursors[1]->xval;
      } else {
        start = ud->cursors[1]->xval;
        end = ud->cursors[0]->xval;
      }
      az_cmd_zoom_absolute(ud, start, end );
//      cu_clear_cursors(ud);
   } else {
       msg_info(_("\n\nThis function will zoom between the 2 cursors\n"
		 "But you need first set the cursor 0 by left click in DrawingArea\n"
		 "and the cursor 1 by middle click in DrawingArea\n") );
   }
}

void
az_pop_zoom_cursors_gaction (GSimpleAction *action, GVariant *param, gpointer user_data )
{
   WavePanel *wp = (WavePanel *) user_data;
   az_zoom_cursors_gaction (action, param, wp->ud);
}


void az_zoom_x_full_gaction (GSimpleAction *action, GVariant *param, gpointer user_data )
{
   UserData *ud = (UserData *) user_data;
   double start;
   double end;
   GawLabels *lbx = ud->xLabels;
   
   start = lbx->min_val;
   end = lbx->max_val;
 
   if (lbx->start_val == lbx->min_val && lbx->end_val == lbx->max_val) {
      msg_info (_("\n\nNo Zoom X to cancel\n"));
   }

   az_cmd_zoom_absolute(ud, start, end );
}

void az_pop_zoom_x_full_gaction (GSimpleAction *action, GVariant *param, gpointer user_data )
{
   WavePanel *wp = (WavePanel *) user_data;
   az_zoom_x_full_gaction (action, param, wp->ud );
}

void az_pop_zoom_y_full_gaction (GSimpleAction *action, GVariant *param, gpointer user_data )
{
   WavePanel *wp = (WavePanel *) user_data;
   if ( ! wp ) {
      return;
   }
   GawLabels *lby = wp->yLabels;

   wp->man_yzoom = 0;
   pa_panel_set_yvals( wp, lby->min_val, lby->max_val);
   pa_panel_full_redraw(wp);
}

void az_zoom_y_full_gaction (GSimpleAction *action, GVariant *param, gpointer user_data )
{
   UserData *ud = (UserData *) user_data;
   WavePanel *wp = ud->selected_panel;

   if ( ! wp) {
      msg_info (aw_panel_not_selected_msg);
      ap_all_redraw(ud);
      return;
   }
   az_pop_zoom_y_full_gaction (action, param, wp);
}

void az_zoom_x_gaction (GSimpleAction *action, GVariant *param, gpointer user_data )
{
   UserData *ud = (UserData *) user_data;
   ud->srange->type = SR_X;
   ud->mouseState = M_SELRANGE_ARMED;

   /* set gdk cursor in all panels */
   g_list_foreach(ud->panelList, (GFunc) pa_panel_drawing_set_gdk_cursor,
                     GINT_TO_POINTER (GDK_RIGHT_SIDE) );
}

void az_pop_zoom_x_gaction (GSimpleAction *action, GVariant *param, gpointer user_data )
{
   WavePanel *wp = (WavePanel *) user_data;
   az_zoom_x_gaction (action, param, wp->ud);
}

void
az_zoom_y_gaction (GSimpleAction *action, GVariant *param, gpointer user_data )
{
   UserData *ud = (UserData *) user_data;
   ud->srange->type = SR_Y;
   ud->mouseState = M_SELRANGE_ARMED;
   /* set gdk cursor in all panels */
   g_list_foreach(ud->panelList, (GFunc) pa_panel_drawing_set_gdk_cursor,
                     GINT_TO_POINTER (GDK_TOP_SIDE) );
}

void
az_pop_zoom_y_gaction (GSimpleAction *action, GVariant *param, gpointer user_data )
{
   WavePanel *wp = (WavePanel *) user_data;
   az_zoom_y_gaction (action, param, wp->ud);
}

void
az_zoom_xy_area_gaction (GSimpleAction *action, GVariant *param, gpointer user_data )
{
   UserData *ud = (UserData *) user_data;
   ud->srange->type = SR_XY;
   ud->mouseState = M_SELRANGE_ARMED;
   /* set gdk cursor in all panels */
   g_list_foreach(ud->panelList, (GFunc) pa_panel_drawing_set_gdk_cursor,
                     GINT_TO_POINTER (GDK_TOP_LEFT_CORNER) );
}

void az_pop_zoom_xy_area_gaction (GSimpleAction *action, GVariant *param, gpointer user_data )
{
   WavePanel *wp = (WavePanel *) user_data;
   az_zoom_xy_area_gaction (action, param, wp->ud);
}

typedef struct _ToggledData {
   WavePanel *wp;
   GtkWidget *entry_ys;
   GtkWidget *entry_ye;
} ToggledData;


static void
az_button_toggled_cb (GtkWidget *widget, ToggledData *pdata)
{
   pdata->wp->man_yzoom = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget));
   if ( pdata->entry_ys ){
      gtk_widget_set_sensitive (pdata->entry_ys, pdata->wp->man_yzoom);
   }
   if ( pdata->entry_ye ){
      gtk_widget_set_sensitive (pdata->entry_ye, pdata->wp->man_yzoom);
   }
}

static void
az_entry_changed_cb (GtkWidget *widget, gpointer pdata)
{
   const gchar *text;
   double *val = ( double *) pdata;

   text = gtk_entry_get_text (GTK_ENTRY (widget));
   *val = str2val((char *) text);
}

void
az_pop_zoom_dialog_gaction (GSimpleAction *action, GVariant *param, gpointer user_data )
{
   WavePanel *wp = (WavePanel *) user_data;
   if ( ! wp ) {
      return ;
   }
   UserData *ud = wp->ud;
   GtkWidget *dialog;
   GtkWidget *vbox;
   gint response;
   static ToggledData *toggledData  = NULL;
   GtkWidget *frame;
   GtkWidget *x_table;
   GtkWidget *y_table;
   GtkWidget *label;
   GtkWidget *entry;
   GtkWidget *button;
   gchar *str;
   GawLabels *lbx = ud->xLabels;
   GawLabels *lby = wp->yLabels;

   if (! toggledData) {
      toggledData = g_new0 (ToggledData, 1);
   }
   toggledData->wp = wp;

   dialog = gtk_dialog_new_with_buttons (_("Gaw axis settings"),
					 GTK_WINDOW (ud->window),
					 GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                                         _("_Cancel"),
                                         GTK_RESPONSE_REJECT,
                                	 _("_OK"),
                                         GTK_RESPONSE_ACCEPT,
				 NULL);
   vbox = gtk_dialog_get_content_area (GTK_DIALOG (dialog));
   gtk_container_set_border_width (GTK_CONTAINER (vbox), 8);
   
   /* frame Global X Axis */
   frame = gtk_frame_new ( _("Global X Axis"));
   gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, TRUE, 0);
   x_table = gtk_grid_new ();
   gtk_grid_set_column_spacing( GTK_GRID(x_table), 5);
   gtk_grid_set_row_spacing( GTK_GRID (x_table), 2);
   gtk_container_add (GTK_CONTAINER (frame), x_table);
      
   /* frame Panel Y Axis */
   frame = gtk_frame_new (_("Panel Y Axis"));
   gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, TRUE, 0);
   y_table = gtk_grid_new ();
   gtk_grid_set_column_spacing( GTK_GRID(y_table), 5);
   gtk_grid_set_row_spacing( GTK_GRID (y_table), 2);
   gtk_container_add (GTK_CONTAINER (frame), y_table);

   str = _("min");
   label = gtk_label_new( str );
   gtk_grid_attach(GTK_GRID(x_table),  label, 
                /* left,    top,  width,   height    */
                      1,      0,      1,        1  );   
   label = gtk_label_new( str );
   gtk_grid_attach(GTK_GRID(y_table), label, 
                /* left,    top,  width,   height    */
                      1,      0,      1,        1  );

   str = _("max");
   label = gtk_label_new( str );
   gtk_grid_attach(GTK_GRID(x_table),  label, 
                /* left,    top,  width,   height    */
                      2,      0,      1,        1  );   
   label = gtk_label_new( str );
   gtk_grid_attach(GTK_GRID(y_table),  label, 
                /* left,    top,  width,   height    */
                      2,      0,      1,        1  );   
   
   str = _("Current");
   label = gtk_label_new( str );
   gtk_grid_attach(GTK_GRID(x_table),  label, 
                /* left,    top,  width,   height    */
                      0,      2,      1,        1  );   
   label = gtk_label_new( str );
   gtk_grid_attach(GTK_GRID(y_table),  label, 
                /* left,    top,  width,   height    */
                      0,      2,      1,        1  );   
   
   str = _("New:");
   label = gtk_label_new( str );
   gtk_grid_attach(GTK_GRID(x_table),  label, 
                /* left,    top,  width,   height    */
                      0,      3,      1,        1  );   
   label = gtk_label_new( str );
   gtk_grid_attach(GTK_GRID(y_table),  label, 
                /* left,    top,  width,   height    */
                      0,      3,      1,        1  );   
   
   /* X values */
   str = val2str(lbx->min_val, ud->up->scientific);
   label = gtk_label_new( str );
   gtk_grid_attach(GTK_GRID(x_table),  label, 
                /* left,    top,  width,   height    */
                      1,      1,      1,        1  );   
   str = val2str(lbx->max_val,  ud->up->scientific);
   label = gtk_label_new( str );
   gtk_grid_attach(GTK_GRID(x_table),  label, 
                /* left,    top,  width,   height    */
                      2,      1,      1,        1  );   

   str = val2str(lbx->start_val,  ud->up->scientific);
   label = gtk_label_new( str );
   gtk_grid_attach(GTK_GRID(x_table),  label, 
                /* left,    top,  width,   height    */
                      1,      2,      1,        1  );   
   str = val2str(lbx->end_val,  ud->up->scientific);
   label = gtk_label_new( str );
   gtk_grid_attach(GTK_GRID(x_table),  label, 
                /* left,    top,  width,   height    */
                      2,      2,      1,        1  );   
   
   /* Y values */
   str = val2str(lby->min_val,  ud->up->scientific);
   label = gtk_label_new( str );
   gtk_grid_attach(GTK_GRID(y_table),  label, 
                /* left,    top,  width,   height    */
                      1,      1,      1,        1  );   
   str = val2str(lby->max_val,  ud->up->scientific);
   label = gtk_label_new( str );
   gtk_grid_attach(GTK_GRID(y_table),  label, 
                /* left,    top,  width,   height    */
                      2,      1,      1,        1  );   
   
   str = val2str(lby->start_val,  ud->up->scientific);
   label = gtk_label_new( str );
   gtk_grid_attach(GTK_GRID(y_table),  label, 
                /* left,    top,  width,   height    */
                      1,      2,      1,        1  );   
   str = val2str(lby->end_val,  ud->up->scientific);
   label = gtk_label_new( str );
   gtk_grid_attach(GTK_GRID(y_table),  label, 
                /* left,    top,  width,   height    */
                      2,      2,      1,        1  );   
   
   /* new X values in entry */
   str = val2str(lbx->start_val,  ud->up->scientific);
   entry = gtk_entry_new() ;
   gtk_entry_set_text (GTK_ENTRY (entry), str);
   g_signal_connect (entry, "changed",
		     G_CALLBACK (az_entry_changed_cb),
		     ( gpointer) &lbx->start_val);
   gtk_grid_attach(GTK_GRID(x_table),  entry, 
                /* left,    top,  width,   height    */
                      1,      3,      1,        1  );   
   str = val2str(lbx->end_val, ud->up->scientific);
   entry = gtk_entry_new() ;
   gtk_entry_set_text (GTK_ENTRY (entry), str);
   g_signal_connect (entry, "changed",
		     G_CALLBACK (az_entry_changed_cb),
		     ( gpointer) &lbx->end_val);
   gtk_grid_attach(GTK_GRID(x_table),  entry, 
                /* left,    top,  width,   height    */
                      2,      3,      1,        1  );   
   
   /* new Y values in entry */
   str = val2str(lby->start_val, ud->up->scientific);
   entry = gtk_entry_new() ;
   toggledData->entry_ys = entry;
   gtk_entry_set_text (GTK_ENTRY (entry), str);
   g_signal_connect (entry, "changed",
		     G_CALLBACK (az_entry_changed_cb),
		     ( gpointer) &lby->start_val);
   gtk_grid_attach(GTK_GRID(y_table),  entry, 
                /* left,    top,  width,   height    */
                      1,      3,      1,        1  );   
   gtk_widget_set_sensitive (entry, wp->man_yzoom);
   str = val2str(lby->end_val, ud->up->scientific);
   entry = gtk_entry_new() ;
   toggledData->entry_ye = entry;
   gtk_entry_set_text (GTK_ENTRY (entry), str);
   g_signal_connect (entry, "changed",
		     G_CALLBACK (az_entry_changed_cb),
		     ( gpointer) &lby->end_val);
   gtk_grid_attach(GTK_GRID(y_table),  entry, 
                /* left,    top,  width,   height    */
                      2,      3,      1,        1  );   
   gtk_widget_set_sensitive (entry, wp->man_yzoom);

   /* Full Scale button */
   str = _("Zoom to");
   button = gtk_toggle_button_new_with_label (str) ;
   gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(button), wp->man_yzoom);
   g_signal_connect (button, "toggled",
		     G_CALLBACK (az_button_toggled_cb),
		     ( gpointer) toggledData );
   gtk_grid_attach(GTK_GRID(y_table), button, 
                /* left,    top,  width,   height    */
                      0,      0,      1,        1  );   
   
   gtk_widget_show_all (vbox);
   response = gtk_dialog_run (GTK_DIALOG (dialog));

  if (response == GTK_RESPONSE_ACCEPT) {
     msg_dbg(_("dialog OK"));
     az_cmd_zoom_absolute(ud, lbx->start_val, lbx->end_val );
     ap_all_redraw(ud);
  } else {
     pa_panel_set_yvals( wp, lby->min_val, lby->max_val);
     wp->man_yzoom = 0;
  }

  gtk_widget_destroy (dialog);
}

void
az_zoom_dialog_gaction (GSimpleAction *action, GVariant *param, gpointer user_data )
{
   UserData *ud = (UserData *) user_data;
   if ( ud->selected_panel == NULL) {
      msg_info (aw_panel_not_selected_msg);
      return ;
   }
   WavePanel *wp = ud->selected_panel ;

   az_pop_zoom_dialog_gaction ( action, param, wp);
}
