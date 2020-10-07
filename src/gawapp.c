/*
 * gawapp.c - gaw application stuff
 * 
 * include LICENSE
 */
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gtk/gtk.h>
#include <string.h>
#include <math.h>

#include <gaw.h>
#include <datestrconv.h>
#include <duprintf.h>

#ifdef TRACE_MEM
#include <tracemem.h>
#endif
        

void ap_set_user_panel_size(UserData *ud)
{
   if (ud->winWidth ){
      ud->up->panelWidth  = ud->panelWidth;
      ud->up->panelHeight = ud->panelHeight;
      msg_dbg( "w %d, h %d", ud->up->panelWidth, ud->up->panelHeight );
   }
}

WavePanel *ap_panel_add_line(UserData *ud, WavePanel *owp, int relpos)
{
   int pos = pa_panel_find_pos(owp);
   
   if ( pos < 0 ) {
      pos = g_list_length( ud->panelList);
   }
   pos += relpos;
   
//   ap_set_user_panel_size(ud);
   WavePanel *wp = pa_panel_new ( ud );
   ud->panelList = g_list_insert( ud->panelList, wp, pos );
   /*  update a container to store the WavePanels */
   ap_panel_update_table(ud);
   
   aw_panel_scrolled_set_size_request(ud);
   return (WavePanel *) g_list_nth_data (ud->panelList, pos);
}

/*
 * Remove this panel
 *   if panel == NULL remove selected
 *   if no panel selected remove last one
 */
void ap_panel_remove_line( UserData *ud, WavePanel *wp)
{
   if ( ! wp ){
      if ( (wp = pa_panel_find_selected(ud)) == NULL){
	 wp = (WavePanel *) g_list_nth_data (ud->panelList, g_list_length( ud->panelList) - 1);
      }
   }

   if ( g_list_length(ud->panelList) == 1 ) {
      msg_warning(_("I don't want to remove the last panel!"));
      return;
   }
//   ap_set_user_panel_size(ud);
   ud->panelList = g_list_remove ( ud->panelList, wp );
   pa_panel_destroy(wp);
   ap_panel_update_table(ud);
   
   /* resize the window to minimum needed */
   aw_panel_scrolled_set_size_request(ud);
   ap_all_redraw(ud);
}

void ap_panel_update_table(UserData *ud )
{
   int newrow;
   int n;
   GtkWidget *table = ud->panelTable;

   if ( ! table ) {
      return;
   }
   ap_container_empty( table, 0);
   n =  g_list_length( ud->panelList);
//   gtk_table_resize ( GTK_TABLE(table), n, AW_PANELTABLE_COLS);

   msg_dbg( "panelTable rows %d", n );
   
   newrow = 0;
   GList *list = ud->panelList;
   while (list) {
      GList *next = list->next;

      WavePanel *wp = (WavePanel *) list->data;

      gtk_grid_attach(GTK_GRID(table), wp->lmtable, 
                /* left,    top,  width,   height    */
                      0, newrow,      1,        1  );

      gtk_grid_attach(GTK_GRID(table), wp->drawing, 
                /* left,    top,  width,   height    */
                      1, newrow,      1,        1  );
      newrow++;
      list = next;
   }
}

void ap_set_xvals(UserData *ud)
{
   GawLabels *lbx = ud->xLabels;
   int start_idx = 0;
   int end_idx = 0;
   

   lbx->npoints = 0;
   if ( ud->curwds ) {
      start_idx = dataset_find_row_index(ud->curwds, lbx->start_val );
      end_idx   = dataset_find_row_index(ud->curwds, lbx->end_val );
      lbx->npoints = (end_idx - start_idx) / lbx->w;
   }
   
   al_label_compute(lbx);

   if ( ud->LogX ){
      gtk_widget_set_sensitive (ud->LogX, lbx->logAble );
   }
   msg_dbg("  %d/%d, np %d,  LogX %d",
	     end_idx - start_idx, lbx->w, lbx->npoints, lbx->logAble );
}

void ap_container_empty(GtkWidget *widget, int del)
{
   if ( ! widget ) {
      return;
   }
   GList *list = gtk_container_get_children(GTK_CONTAINER(widget));
   
   while (list) {
      GList *next = list->next;
      gtk_container_remove (GTK_CONTAINER(widget), list->data);
      list = next;
    }
   if ( del ) {
      gtk_widget_destroy(widget);
   }
}


void 
ap_widget_show(GtkWidget *w,  gboolean show)
{
   if ( show) {
      if ( ! gtk_widget_get_visible(w)) {
	 gtk_widget_show(w);
      }
   } else {
      if ( gtk_widget_get_visible(w)) {
	 gtk_widget_hide(w);
      }
   }
}

/*************** X measure button  ***************/
GtkWidget * 
ap_create_toggle_button(GtkWidget *box, gchar *brc, gchar *tips )
{
   GtkWidget *button;
   
   button = gtk_toggle_button_new ();
   if ( box ) {
      gtk_box_pack_end(GTK_BOX(box), button,  FALSE, FALSE, 0);
   }
   gtk_widget_set_name(button, brc );
   gtk_widget_set_tooltip_text( GTK_WIDGET(button), tips );
   gtk_widget_show(button); /* show the button to get the color !!! */
   
   return button;
}

void ap_xmeasure_button_show(UserData *ud)
{  
   int i;
   int shown = 0;
   
   /* show the 3 X buttons  */
   for ( i = 0 ; i < AW_NX_MBTN ; i++ ) {
      ap_widget_show( ud->cursors[i]->button, ud->cursors[i]->shown);
      shown |= ud->cursors[i]->shown;
   }
   if ( ud->meas_hbox_shown !=  shown ){
      ud->meas_hbox_shown = shown;
      aw_window_size(ud);
   }
}

void ap_xmeasure_button_draw(UserData *ud)
{      
   int i;
   UserPrefs *up = ud->up;
   char buf[BUFSIZ];
   
   /* draw xval in label for the 3 X buttons  */
   for ( i = 0 ; i < AW_NX_MBTN ; i++ ) {

      if ( up->xconvert == 1 ){ /*convert time_t to string */
         time_t mytime = (time_t) ud->cursors[i]->xval;
         char *tips = app_strdup_printf( "%ld", mytime );
         if ( i == 2 ){ /* diff value */
            convert_difftime_str( mytime, up->diffdate_fmt, buf, sizeof(buf) );
         } else {
            convert_time_t_to_date( mytime, up->date_fmt, buf, sizeof(buf) );
         }
         gtk_label_set_text(GTK_LABEL(ud->cursors[i]->label), buf );
         gtk_widget_set_tooltip_text (GTK_WIDGET(ud->cursors[i]->button), tips );
         app_free( tips );
      } else {
         gtk_label_set_text(GTK_LABEL(ud->cursors[i]->label),
		    val2str(ud->cursors[i]->xval, up->scientific) );
      }
   }
}

void
ap_measure_button_clicked_cb (GtkWidget *button, gpointer user_data)
{
  GtkWidget *label = GTK_WIDGET (user_data);
  GtkClipboard *clipboard;

  /* Get the clipboard object */
  clipboard = gtk_widget_get_clipboard (button, GDK_SELECTION_CLIPBOARD);

  /* Set clipboard text */
  gtk_clipboard_set_text (clipboard, gtk_label_get_text (GTK_LABEL (label)), -1);
}

GtkWidget * 
ap_create_measure_button(GtkWidget *box, gchar *brc, gchar *tips )
{
   GtkWidget *button;
   
   button = gtk_button_new ();
   if ( box ) {
      gtk_box_pack_end(GTK_BOX(box), button,  FALSE, FALSE, 0);
   }
   gtk_widget_set_name(button, brc );
   gtk_widget_set_tooltip_text (GTK_WIDGET(button), tips );
   gtk_widget_show(button); /* show the button to get the color !!! */
   
   return button;
}

GtkWidget * 
ap_create_measure_label(GtkWidget *box, gchar *blabel,  gchar *lrc )
{
   GtkWidget *label;
   
   label = gtk_label_new (blabel);
   gtk_widget_set_name( label, lrc );
   gtk_widget_show( label);
/* do not work -   gtk_label_set_selectable ( GTK_LABEL(label), TRUE ); */
   gtk_container_add( GTK_CONTAINER(box), label );
   
   return label;
}

void ap_create_xmeasure_unit(UserData *ud)
{
   GtkWidget *hbox;
   GtkWidget *button;
   GtkWidget *label;
   gchar text[80];
   gchar tips[80];
   int i;
   
   /* hbox for 3 cursor labels */
   hbox = ud->meas_hbox;
   gtk_widget_show(hbox);
   
   if ( ! ud->cursors ) {
      ud->cursors = g_new0(AWCursor *, AW_NX_MBTN);
   }

   /* create the 3 X buttons  */
   for ( i = AW_NX_MBTN - 1 ; i >= 0 ; i-- ) {
      ud->cursors[i] = g_new0(AWCursor, 1);
      if ( i == 2 ) {
	   /* diff button */
	 sprintf(text, "cursorD");
	 sprintf(tips, _("measure: cursor1 - cursor0"));
      } else {
	 sprintf(text, "cursor%d", i);
	 sprintf(tips, _("measure: cursor%d"), i);
	 ud->cursors[i]->x = (i + 1) * ud->up->panelWidth / 3;
      }
      button = ap_create_measure_button(hbox, "measurebutton", tips );
      label = ap_create_measure_label(button, "0.0", text );
      g_signal_connect (button, "clicked",
                        G_CALLBACK (ap_measure_button_clicked_cb), label );
      ud->cursors[i]->button = button;
      ap_widget_show(button,  0);

      ud->cursors[i]->label = label;
   }
}

/*************** X labels  ***************/


void
ap_xlabel_box_show(UserData *ud)
{
  int changed = gtk_widget_get_visible(ud->xlabel_box) ^ ud->up->showXLabels;
   ap_widget_show(ud->xlabel_box, ud->up->showXLabels);
   ap_widget_show(ud->logx_box, ud->up->showXLabels);
   if ( ud->up->showXLabels ) {
      ap_widget_show(ud->win_xlabel_log, al_label_do_logAxis(ud->xLabels) );
   }
   if ( changed ) {
      aw_window_size(ud);
   }
}

/*
 * ev box on all the ligne
 */
void ap_xlabel_box_create(UserData *ud)
{
   GtkWidget *hbox;
   GtkWidget *label;
   GtkWidget *event_box;
   GtkWidget *layout;
   GtkWidget *da;
   GawLabels *lbx = ud->xLabels;

   /* event box */
   event_box = ud->xlabel_ev_box;
   g_object_ref (event_box); /* increment ref to avoid destruction */
   gtk_widget_show(event_box);
   ad_set_drag_dest(event_box, ud, NULL, DND_EVENT_BOX );
   
   /* all line hbox */
   hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
   gtk_container_set_border_width (GTK_CONTAINER (hbox), 2);
   ud->allline_box = hbox ;
   gtk_widget_show (hbox);
   gtk_container_add (GTK_CONTAINER (event_box), hbox);
 
   /* xlabels box */
   hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
   gtk_widget_show (hbox);
   ud->xlabel_box = hbox ;
   g_object_ref (hbox); /* increment ref to avoid destruction */
   gtk_box_pack_end (GTK_BOX (ud->allline_box), hbox, FALSE, TRUE, 0);
 
   /* create layout for xlabels */
   layout =  gtk_layout_new (NULL, NULL);
   lbx->label_layout = layout;
   gtk_box_pack_start (GTK_BOX (ud->xlabel_box), layout, TRUE, TRUE, 0);
   gtk_widget_show (layout);

   /* create 2 xlabels */
   al_label_add_remove(lbx, 2);
   lbx->nlabels = 2;  /* for init */

   /* logx box */
   hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
   gtk_box_pack_start (GTK_BOX (ud->allline_box), hbox, TRUE, TRUE, 0);
   ud->logx_box = hbox ;
   g_object_ref (hbox); /* increment ref to avoid destruction */

   /* drawing area for drawing sizing */
   da = ag_grip_new (ud);
   gtk_box_pack_end (GTK_BOX (hbox), da, FALSE, FALSE, 0);
   
   /* logx label */
   label = gtk_label_new("LogX");
   gtk_box_pack_end(GTK_BOX(hbox), label, FALSE, FALSE, 0);
   ud->win_xlabel_log = label;
 
   ap_xlabel_box_show(ud);
}

/*************** X horizontal scroolbar  ***************/
/*
 * main win horizontal scroolbar handler
 */
void
ap_x_scroll_handler_cb(GtkWidget *widget, gpointer data )
{
   GtkAdjustment *adj = GTK_ADJUSTMENT(widget);
   WavePanel *wp;
   UserData *ud = (UserData *) data;
   GawLabels *lbx = ud->xLabels;
   double start;
   double end ;

   gdouble value = gtk_adjustment_get_value(adj);
   gdouble page_size = gtk_adjustment_get_page_size(adj);

   if ( lbx->logAxis && lbx->logAble ) {

      start = ((lbx->min_val > DBL_EPSILON) || (lbx->min_val < -DBL_EPSILON)) ?
    		  	  lbx->min_val * pow( lbx->max_val / lbx->min_val, value ) :
    		  	  0.0;
      end = ((lbx->min_val > DBL_EPSILON) || (lbx->min_val < -DBL_EPSILON)) ?
    		  	  lbx->min_val * pow( lbx->max_val / lbx->min_val, value + page_size ) :
    		  	  0.0;
   } else {
      start = lbx->min_val + value * ( lbx->max_val - lbx->min_val );
      end =  lbx->min_val + 
	 (value + page_size ) * ( lbx->max_val - lbx->min_val ) ;
   }

   msg_dbg("start %f, end %f", start, end);
   al_label_update_vals(lbx, start, end );
   ap_set_xvals(ud);
   
   GList *list = ud->panelList;
   while (list) {
      GList *next = list->next;

      wp = (WavePanel *) list->data;       
      AWCursor *csp;

      csp = ud->cursors[0];
      if ( csp->shown ) {
	 csp->zoom = 1;
	 cu_update_xcursor(wp, csp, csp->x, 0);
      }
      csp = ud->cursors[1];
      if ( csp->shown ) {
	 csp->zoom = 1;
	 cu_update_xcursor(wp, csp, csp->x, 0);
      }
      
      if ( ud->suppress_redraw == 0) {
	 da_drawing_redraw(wp->drawing);
      }
      list = next;
   }
   al_label_draw(lbx);
}

/*
 * main win bottom right scrollbar
 */
void ap_create_win_bottom(UserData *ud)
{
   GtkAdjustment *adj;
   GtkWidget *scrollbar;
   
   /* scrollbar */
   adj = (GtkAdjustment *)
      gtk_adjustment_new(0.0, /* value */
			 0.0, /* lower */
			 1.0, /* upper */
			 1.0/100.,    /* step increment = 1% */
			 1.0/2.,      /* page increment = 50% */
			 1.0          /* page_size */
			 );
   ud->xadj = adj;
   scrollbar = gtk_scrollbar_new(GTK_ORIENTATION_HORIZONTAL,
                                  GTK_ADJUSTMENT(adj));

   g_signal_connect( adj, "value_changed", 
		     G_CALLBACK (ap_x_scroll_handler_cb), (gpointer) ud );
   gtk_widget_show(scrollbar);
   ud->xscrollbar = scrollbar;
   g_object_ref (scrollbar); /* increment ref to avoid destruction */
}


void
ap_mbtn_update(MeasureBtn *mbtn, UserData *ud )
{
   double mvalue;
   gint show;
   
   if ( mbtn->csp == ud->cursors[2] ) {
      /* Y diff button */
      double y0 = wavevar_interp_value(mbtn->var, ud->cursors[0]->xval);
      double y1 = wavevar_interp_value(mbtn->var, ud->cursors[1]->xval);
      mvalue = y1 - y0;
      show = mbtn->csp->shown & ud->up->showYDiff;
   } else {
      mvalue = wavevar_interp_value(mbtn->var, mbtn->csp->xval);
      show = mbtn->csp->shown ;
   }
   
   if (show) {
      gtk_label_set_text(GTK_LABEL(mbtn->label), val2str(mvalue, ud->up->scientific));
   }
   ap_widget_show(mbtn->button, show);
}


void
ap_mbtn_update_all(UserData *ud)
{
   msg_dbg("called");
   g_list_foreach(ud->all_measure_buttons, (GFunc) ap_mbtn_update, (gpointer) ud);
}



void ap_all_panel_redraw(UserData *ud)
{
   g_list_foreach (ud->panelList,(GFunc) pa_panel_full_redraw, NULL);
}

void ap_redraw_x(UserData *ud)
{
   ap_xlabel_box_show(ud);
   al_label_draw(ud->xLabels);
}

void ap_all_redraw(UserData *ud)
{
   GawLabels *lbx = ud->xLabels;
   
   if ( lbx ) { /* force recompute */
      al_label_update_min_max_vals(lbx, 0.0, 0.0 );
   }
   ap_all_panel_redraw(ud);
   ap_redraw_x(ud);
}


/********************************************************************/


/*
 * Add a new waveform to a WavePanel, creating a new VisibleWave.
 * If no wavepanel is specified, try to use the first "selected" wavepanel,
 * This is the only place that VisibleWave structures are created.
 * If ovw != NULL, just ref this wave.
 */

void ap_panel_add_var(WavePanel *wp, WaveVar *var, VisibleWave *ovw, GdkRGBA *usercolor)
{
   VisibleWave *vw;
   DataFile *wdata = (DataFile *) wavetable_get_datafile((WaveTable *) var->wvtable);
   UserData *ud = ( UserData *) wdata->ud;
   GdkRGBA *color = NULL;
   
   if (wp == NULL) {
      if ( ud->selected_panel  == NULL) {
	 msg_info (aw_panel_not_selected_msg);
	 return ;
      }
      wp = ud->selected_panel;
   }

   if ( ovw ) {
      color = ovw->color;
   }

   if( usercolor ) {      /* stefan */
      color = usercolor;
   } 

   vw = wave_new( var, wdata );
   
   if ( (vw->logAble == 0 && al_label_get_logAxis(wp->yLabels)) ||
	(wavevar_ivar_get_max(vw->var) <= 0 && al_label_get_logAxis(ud->xLabels)) ){
      gint response;
      char *message =
	 _("You are going to display a non Logarithm waveform\n"
	   "in a Logarithm scaled panel.\n"
	   "If you click OK, the waveform will be displayed,\n"
	   "and the scale will be reverted to linear.\n"
	   "Click Cancel to abort the operation\n");

      response = aw_dialog_show ( AW_MSG_T_SHOW_CANCEL | MSG_T_WARNING, message );
      if (response != GTK_RESPONSE_ACCEPT ) {
	 wave_destroy(vw);
	 return;
      }
   }
   wave_attach(vw, wp, color);
   
   /*
    * update max and min in wp and ud
    * Update drawingArea
    * Update y-axis labels
    */
   pa_panel_full_redraw(wp);

   return;
}


DataFile *
ap_load_wave (DataFile *wdata)
{
   UserData *ud = (UserData *) wdata->ud;
   
   int ret = datafile_load(wdata);
   if (ret < 0) {
      datafile_destroy(wdata);
      return NULL;
   }
   ud->wdata_list = g_list_prepend(ud->wdata_list, wdata);
   return wdata;
}

/*
 * remove VisibleWave - 
 * Remove from their respective panels all waveforms if
 *  the specified test succeeds.
 */

typedef struct _TestParams {
   VisibleWave *vw;
   WavePanel *wp;
   WaveVar *var;
   gpointer *data;
   gpointer *data1;
} TestParams ;

/*
 * return TRUE if visible wave is selected
 */
static gboolean
ap_wave_if_selected(TestParams *params)
{
   return gtk_toggle_button_get_active
      (GTK_TOGGLE_BUTTON(params->vw->button)) ;
}

/*
 * return TRUE if visible wave  reference this file
 */
static gboolean
ap_wave_if_wfile(TestParams *params)
{
   if ( params->vw->wdata == (DataFile *) params->data) {
      return TRUE;
   }
   return FALSE;
}

/*
 * test if the visible wave var is found in new wdata
 *    if yes update visible wave with new var pointers, return false
 *    else return TRUE and the wave will be removed
 */    
static gboolean
ap_wave_if_var_wdata(TestParams *params)
{
   VisibleWave *vw = params->vw;
   DataFile *wdata = (DataFile *) params->data;
   WaveVar *newvar;
   int j;
   DnDSrcData *dd;
   
   WaveVar *oldvar = vw->var;
   if (vw->wdata == wdata) {
      newvar = wavetable_get_var_for_name(wdata->wt, oldvar->varName, oldvar->tblno );
      if (newvar) {
	 msg_dbg( "updated variable %s to %lx", oldvar->varName, (unsigned long) wdata->wt);
	 vw->var = newvar;
	 for (j = 0 ; j < AW_NY_MBTN ; j++) {
	    vw->mbtn[j]->var = newvar;
	 }
         dd = g_object_get_data (G_OBJECT(vw->button), "DnDSrc");
         if ( dd ){
            dd->var = newvar;
         }
      } else {
	 msg_dbg( "variable %s no longer in file %s; removing from panel",
		  oldvar->varName, wdata->filename);
	 return TRUE;
      }
   }
   return FALSE;
}

/*
 * return TRUE if visible wave  reference this panel and this var
 */

static gboolean
ap_wave_if_panel_and_var(TestParams *params)
{
   if ( params->wp == (WavePanel *) params->data &&
       params->var == (WaveVar *) params->data1) {
      return TRUE;
   }
   return FALSE;
}

/*
 * remove all waves corresponding to vars with the same base name
 */
static gboolean
ap_wave_if_var_name(TestParams *params)
{
   WaveVar *curvar = params->var;
   WaveVar *var = (WaveVar *) params->data;
 
   if ( app_strcmp(curvar->varName, var->varName) == 0 ){ 
      return TRUE;
   }
   return FALSE;
}

/*
 * Remove from panels any VisibleWaves that responds TRUE
 * to the specified test.
 */

static int
ap_remove_all_wave_if_test(UserData *ud, TestParams *params,
			    gboolean  (*testFunc) () )
{
   GList *oldlist;
   GList *list;
   VisibleWave *vw;
   int removed = 0;

   GList *plist = ud->panelList;
   while (plist) {
      GList *pnext = plist->next;

      WavePanel *wp = (WavePanel *) plist->data;   
      params->wp = wp;
      oldlist = list = g_list_copy (wp->vwlist);

      while (list) {
	 GList *next = list->next;
	 
	 vw = params->vw = (VisibleWave *) list->data;
	 params->var = vw->var;
	 if ( testFunc ( params ) ) {
	    wave_destroy(vw);
	    removed++;
	 }
	 list = next;
      }
      g_list_free(oldlist);
      plist = pnext;
   }
   return removed;
}

/*
 * Remove from panels any VisibleWaves that have been
 * selected by clicking on their label-buttons.
 */
void
ap_remove_all_wave_selected(UserData *ud)
{
   TestParams *params; 
   params = g_new0(TestParams, 1);

   if ( ap_remove_all_wave_if_test(ud, params, ap_wave_if_selected ) == 0) {
      msg_info(_("\n\nNo wave selected for Delete") );
   }
   g_free(params);
}

/*
 * Delete a wave file.
 * callback from menu: wavelist->file->delete
 */

void
ap_remove_all_wave_if_wfile(DataFile *wdata)
{
   TestParams *params; 
   UserData *ud = wdata->ud;

   params = g_new0(TestParams, 1);
   params->data = (gpointer) wdata;
   ap_remove_all_wave_if_test(ud, params, ap_wave_if_wfile );
   g_free(params);
}

/*
 * Delete all waves from panel if var .
 *   do not remove the panel
 */

void
ap_remove_all_wave_if_panel_and_var(WavePanel *wp, WaveVar *var)
{
   TestParams *params; 
   UserData *ud = wp->ud;

   params = g_new0(TestParams, 1);
   params->data = (gpointer) wp;
   params->data1 = (gpointer) var;
   ap_remove_all_wave_if_test(ud, params, ap_wave_if_panel_and_var );
   g_free(params);
   pa_panel_full_redraw(wp);
}

/*
 * remove all waves corresponding to vars with the same base name
 */
void
ap_remove_all_wave_if_var_name( UserData *ud, WaveVar *var)
{
   TestParams *params; 

   params = g_new0(TestParams, 1);
   params->data = (gpointer) var;
   ap_remove_all_wave_if_test(ud, params, ap_wave_if_var_name );
   g_free(params);
   ap_all_redraw(ud);
}

/*
 * Remove a visible wave if its var is not found in new wdata
 */
void
ap_remove_all_wave_if_vw_var_wdata(DataFile *wdata)
{
   UserData *ud = wdata->ud;
   TestParams *params; 

   params = g_new0(TestParams, 1);
   params->data = (gpointer) wdata;
   ap_remove_all_wave_if_test(ud, params, ap_wave_if_var_wdata );
   g_free(params);
}


/*
 * command or callback from menu: wavelist->file->reload
 */
void
ap_reload_wave_file(DataFile *wdata)
{
   UserData *ud = wdata->ud;

   if (  wdata->wt ){
      datafile_reload(wdata);
   }

   if ( wdata->wt == NULL) {
      return;
   }
   msg_dbg( "(%s) old=%lx new=%lx", wdata->filename,
	   (unsigned long) wdata->old_wt, (unsigned long) wdata->wt );

   ap_remove_all_wave_if_vw_var_wdata(wdata);

   /* remove old buttons from list, and add new ones */    
   datafile_recreate_list_win (wdata);

   wavetable_destroy(wdata->old_wt);
   ap_all_redraw(ud);
   ap_mbtn_update_all(ud);
}


/*
 * Delete a data file wdata. 
 */
void ap_delete_datafile(DataFile *wdata)
{
   UserData *ud = wdata->ud;
   
   if ( wdata->wt ) {
      /* remove references from displayed waves */
      ap_remove_all_wave_if_wfile(wdata);
   }
   ud->wdata_list = g_list_remove(ud->wdata_list, wdata);
   
   msg_dbg( "destroy DataFile 0x%lx", (unsigned long) wdata);
   datafile_destroy(wdata);
   ap_all_redraw(ud);
}

/*
 * Recreate a table from displayed variables for exporting to a file
 */

WaveTable *
ap_wavetable_new_from_displayed(UserData *ud )
{
   int row;
   GList *list;
   VisibleWave *vw;
   WaveVar *var;
   WaveVar *ivar = NULL;
   WaveTable *wt = wavetable_new( NULL, "displayed" );
   WDataSet *wds = wavetable_get_dataset(wt, 0);
   WDataSet *swds = NULL;
   int first = 1;
   GList *panelList;

   panelList = ud->panelList;
   while (panelList) {
      GList *pnext = panelList->next;

      WavePanel *wp = (WavePanel *) panelList->data;   

      list = wp->vwlist;
      while (list) {
	 GList *next = list->next;
	 
	 vw = (VisibleWave *) list->data;
	 var = vw->var;
	 if ( first ) {
	    first = 0;
	    swds = var->wds;
	    ivar = (WaveVar *) dataset_get_wavevar(swds, 0 );
	    dataset_var_add(wds, ivar->varName, ivar->type, ivar->ncols);
	 }
	 dataset_var_add(wds, var->varName, var->type, var->ncols);

	 list = next;
      }
      panelList = pnext;
   }
   if ( ! ivar ) {
      wavetable_destroy(wt);
      return NULL;
   }
   dataset_dup_setname(wds, dataset_get_setname(swds) );

   int nrows = dataset_get_nrows(swds );

   for (row = 0 ; row < nrows; row++) {
      double val = dataset_val_get( swds, row, ivar->colno );
      dataset_val_add ( wds, val);

      panelList = ud->panelList;
      while (panelList) {
	 GList *pnext = panelList->next;

	 WavePanel *wp = (WavePanel *) panelList->data;   

	 list = wp->vwlist;
	 while (list) {
	    GList *next = list->next;
	    
	    vw = (VisibleWave *) list->data;
	    var = vw->var;
	    val = dataset_val_get( swds, row, var->colno );
	    dataset_val_add ( wds, val);

	    list = next;
	 }
	 panelList = pnext;
      }
   }
   return wt;
}

