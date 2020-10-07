/*
 * gawpanel.c - panel functions
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
 
#ifdef TRACE_MEM
#include <tracemem.h>
#endif
        

/*
 *** \brief Allocates memory for a new WavePanel object.
 */

WavePanel *pa_panel_new( UserData *ud )
{
   WavePanel *wp;

   wp = app_new0(WavePanel, 1);
   pa_panel_construct( wp, ud );
   app_class_overload_destroy( (AppClass *) wp, pa_panel_destroy );
   return wp;
}

/** \brief Constructor for the WavePanel object. */

void pa_panel_construct( WavePanel *wp, UserData *ud)
{
   UserPrefs *up = ud->up;
   
   app_class_construct( (AppClass *) wp );

   wp->ud = ud;
   pa_panel_set_drawing_func(wp, NULL );
   wp->showGrid = up->showGrid;
   wp->drawing = da_drawing_create(wp);
   gm_create_panel_popmenu ( wp );
   wp->grid_color = g_new0 (GdkRGBA, 1);
   pa_panel_color_grid_set (wp, wp->ud->pg_color);
   wp->yLabels = al_label_new( up, up->setLogY, 0 );
   
   wp->lmtable = pa_panel_label_meas_box_create(wp, ud);
   g_object_ref (wp->lmtable); /* increment ref to avoid destruction */

   pa_ylabel_box_show(wp);
   pa_panel_lmscroll_win_set_size_request(wp);
}

/** \brief Destructor for the WavePanel object. */

void pa_panel_destroy(void *wp)
{
   WavePanel *this = (WavePanel *) wp;

   if (wp == NULL) {
      return;
   }
   pa_panel_vw_delete(this);
   al_label_destroy(this->yLabels);

   app_free(this->grid_color);

   /* destroy texts */
   pa_panel_delete_all_texts(this);

   /* the 2 next unref create gtk warning - not necesary ? */
   g_object_ref_sink (this->popmenu);
   g_object_ref_sink (this->textpopmenu);
   g_object_unref (this->lmtable); /* deccrement ref */
   gtk_widget_destroy(this->lmtable);
   gtk_widget_destroy(this->drawing);

   app_class_destroy( wp );
}

void pa_panel_color_grid_set (WavePanel *wp, gpointer data)
{
   GdkRGBA *color = (GdkRGBA *) data;
 
   if ( color ) {
      memcpy( wp->grid_color, color, sizeof(GdkRGBA) );
   }
}

/*************** Y labels  ***************/


void pa_panel_draw_ylabels(WavePanel *wp)
{
   UserPrefs *up = wp->ud->up;
   GawLabels *lby = wp->yLabels;

   if ( (lby->changed & CV_INIT) == 0 ){
      return;
   }

   if ( up->showMoreYLabels ) {
      al_label_draw(lby);
   } else {
      gtk_label_set_text(GTK_LABEL(wp->ylabel_min),
		    val2str(lby->start_val, up->scientific));
      msg_dbg( "called %s", val2str(lby->start_val, up->scientific) );
      gtk_label_set_text(GTK_LABEL(wp->ylabel_max),
		    val2str(lby->end_val, up->scientific));
      msg_dbg( "called %s", val2str(lby->end_val, up->scientific) );
   }
}


void pa_ylabel_box_show( WavePanel *wp )
{
   UserPrefs *up = wp->ud->up;
   
   msg_dbg("showYLabels %d, logAxis %d", up->showYLabels,
	   al_label_do_logAxis(wp->yLabels) );

   if ( up->showMoreYLabels ) {
      ap_widget_show(wp->yLabels->label_layout, up->showYLabels);
   } else {
      ap_widget_show(wp->ylabel_min, up->showYLabels);
      ap_widget_show(wp->ylabel_max, up->showYLabels);
   }
   if ( up->showYLabels ) {
      ap_widget_show(wp->ylabel_log, al_label_do_logAxis(wp->yLabels) );
   }

   if ( wp->ud->moreYlabels ){
      gtk_widget_set_sensitive (wp->ud->moreYlabels, up->showYLabels );
   }
}

/*************** panel visible wave  ***************/

int pa_panel_next_color(WavePanel *wp)
{
   int colorn = wp->nextcolor;
   wp->nextcolor = (wp->nextcolor + 1) % wp->ud->NWColors;
   return colorn;
}

void pa_panel_vw_list_add(WavePanel *wp, gpointer vw)
{
   wp->vwlist = g_list_prepend(wp->vwlist, vw);
}

void pa_panel_vw_list_remove(WavePanel *wp, gpointer vw)
{
   if ( wp->vwlist ){
      wp->vwlist = g_list_remove(wp->vwlist, vw);
   }
}

void pa_panel_text_list_add(WavePanel *wp, gpointer gtext)
{
   wp->textlist = g_list_prepend(wp->textlist, gtext);
}

void pa_panel_text_list_remove(WavePanel *wp, gpointer gtext)
{
   if ( wp->textlist ){
      wp->textlist = g_list_remove(wp->textlist, gtext);
   }
}

AppClass *pa_panel_inside_text(WavePanel *wp, int x, int y)
{
   GList *list;
   
   list = wp->textlist;
   while (list) {
      GList *next = list->next;
      AppClass *gtext = (AppClass *) list->data;

      if ( gawtext_inside_text(gtext, x, y) ) {
         return gtext;
      }
      list = next;
   }
   return NULL;
}

void pa_panel_delete_all_texts( WavePanel *wp )
{
   /* destroy texts */
   g_list_foreach(wp->textlist, (GFunc) gawtext_destroy, NULL);
   g_list_free(wp->textlist);
   wp->textlist = NULL;
}

void pa_panel_set_yvals(WavePanel *wp, double start, double end)
{
   GawLabels *lby = wp->yLabels;

   al_label_update_vals(lby, start, end);
   al_label_compute(lby);

   if ( wp->pPLogY ){
      gtk_widget_set_sensitive (wp->pPLogY, lby->logAble );
   }
   if ( wp == wp->ud->selected_panel && wp->ud->LogYPanel ){
      gtk_widget_set_sensitive (wp->ud->LogYPanel, lby->logAble );
   }
   msg_dbg(" start %f, end %f logable %d", start, end, lby->logAble);
}

/*
 * called with g_list_foreach to update a WavePanel
 * from all of its  VisibleWaves.
 */
static void
pa_panel_vw_min_max_update(gpointer p, gpointer d)
{
   VisibleWave *vw = (VisibleWave *) p;
   WavePanel *wp = (WavePanel *) d;
   GawLabels *lby = wp->yLabels;

   /* MIN(a, b)     ((a) < (b) ? (a) : (b)) */
   /* MAX(a, b)     ((a) > (b) ? (a) : (b)) */
   wp->min_xval = MIN( wavevar_ivar_get_min(vw->var), wp->min_xval);
   wp->max_xval = MAX( wavevar_ivar_get_max(vw->var), wp->max_xval);
   /* we should set it to the wds with max xvals */
   wp->ud->curwds = vw->var->wds;

   double min_yval = MIN( wavevar_val_get_min(vw->var), lby->min_val);
   double max_yval = MAX( wavevar_val_get_max(vw->var), lby->max_val);
   al_label_update_min_max_vals(lby, min_yval, max_yval);
}

/*
 * wavepanel_update_data
 *   update wavepanel values that sumarize things over all of the 
 *   VisibleWaves in the panel.
 */
void
pa_panel_update_min_max(WavePanel *wp)
{
   GawLabels *lby = wp->yLabels;
   double min_yval;
   double max_yval; 
   
   wp->ud->curwds = 0;
   if ( wp->vwlist ) {
      wp->min_xval = G_MAXDOUBLE;
      wp->max_xval = -G_MAXDOUBLE;

      /* use  lby->min_val lby->max_val as temp value */
      lby->min_val = G_MAXDOUBLE;
      lby->max_val = -G_MAXDOUBLE;

      /* set wp min, maw x, y values from min, max in datasets */
      g_list_foreach(wp->vwlist, pa_panel_vw_min_max_update, (gpointer) wp);
 
      min_yval = lby->min_val;
      max_yval = lby->max_val;

   } else {
      
      /*
       * set to something reasonable if they didn't change,
       * like if the panel was empty
       */
      wp->min_xval = 0.0; /* wtable->min_xval; */
      wp->max_xval = 1.0; /* wtable->max_xval; */
      min_yval = 0.0;
      max_yval = 1.0;
   }
   if (wp->man_yzoom == 0) {
      pa_panel_set_yvals(wp, min_yval, max_yval);
   }
   /* zero height? set to +- 0.1%  so a line is visible in the center */
   if ((lby->end_val - lby->start_val) < DBL_EPSILON) {
      pa_panel_set_yvals(wp, lby->start_val * 0.999, lby->end_val * 1.001);
      /* still zero?  maybe there's a waveform that is stuck at 0.000 */
      if ((lby->end_val - lby->start_val) < DBL_EPSILON) {
	 pa_panel_set_yvals(wp, lby->start_val - 1e-6, lby->end_val + 1e-6);
      }
   }
}

/* 
 * Update parameters : min, maw, start end on all panels
 */
void pa_panel_update_all_data(UserData *ud)
{
   WavePanel *wp;
   GawLabels *lbx = ud->xLabels;
   double old_min_x = lbx->min_val;
   double old_max_x = lbx->max_val;
   double min_xval;
   double max_xval ;
   GList *list;
   
   min_xval =   G_MAXDOUBLE;
   max_xval = - G_MAXDOUBLE;
   
   list = ud->panelList;
   while (list) {
      GList *next = list->next;
      wp = (WavePanel *) list->data;

      if ( wp && wp->vwlist ) {
	 min_xval = MIN( wp->min_xval, min_xval);
	 max_xval = MAX( wp->max_xval, max_xval);
      }
      list = next;
   }
   /* still nothing? set back to zero */
   if (min_xval == G_MAXDOUBLE) {
      min_xval = 0.0;
   }
   if (max_xval == - G_MAXDOUBLE) {
      max_xval = 1.0;
   }

   /*
    * if start & end were the same or out of range, 
    * just zoom-full so we can see somthing.
    */
   int xchanged = (min_xval != old_min_x || max_xval != old_max_x);

   if ( xchanged ) {
      if ( lbx ) {
         al_label_update_min_max_vals(lbx, min_xval, max_xval );
      }
      az_cmd_zoom_absolute(ud, min_xval, max_xval );
      ap_set_xvals(ud);
   }
   
   if ((fabs(lbx->end_val - lbx->start_val) < DBL_EPSILON ||
        lbx->start_val < lbx->min_val ||
        lbx->end_val > lbx->max_val) ) {
      ud->suppress_redraw = 1;
//      az_cmd_zoom_absolute(ud, lbx->min_val, lbx->max_val );
      ud->suppress_redraw = 0;
   } else if( xchanged ) {
      /*
       * min/max changed, might have added first (or removed last)
       * wave from a file with different range.
       * try to keep start/end same, but make them sane if needed.
       * then update scrollbar.
       */

//      az_cmd_zoom_absolute( ud, min_xval, max_xval );
//      ap_set_xvals(ud);
   }
}


void pa_panel_full_redraw(WavePanel *wp)
{
   /* update max and min values */
   pa_panel_update_min_max(wp);
   pa_panel_update_all_data(wp->ud);
   
   da_drawing_redraw(wp->drawing);
   /* Update y-axis labels */
   pa_panel_draw_ylabels(wp);
   pa_ylabel_box_show( wp );
}

/*
 * left most scrolled window table
 */
void pa_panel_lmswtable_row_add(WavePanel *wp, VisibleWave *vw, int row )

{
   int i;

   gtk_grid_attach(GTK_GRID(wp->lmswtable), vw->button, 
                /* left,    top,  width,   height    */
                      0,    row,      1,        1  );   

   /* add Y measurement buttons */
   for ( i = 0 ; i < AW_NY_MBTN ; i++ ) {
      GtkWidget *button = vw->mbtn[i]->button;

      gtk_grid_attach(GTK_GRID(wp->lmswtable), button, 
                /* left,    top,  width,   height    */
                  1 + i,    row,      1,        1  );   
   }
}


/*
 * left most scrolled window table
 */
void pa_panel_lmswtable_setup(WavePanel *wp )
{
   int newrow;
   int n;

   if ( ! wp->lmswtable ) {
      return;
   }
   ap_container_empty(wp->lmswtable, 0);
   n =  g_list_length( wp->vwlist);

   msg_dbg( "lmswtable rows %d", n );
   
   newrow = 0;

   GList *list = wp->vwlist;
   while (list) {
      GList *next = list->next;
      pa_panel_lmswtable_row_add( wp, (VisibleWave *) list->data, newrow );
      list = next;
      newrow++;
   }
}

void pa_panel_label_meas_box_update(WavePanel *wp)
{
   GawLabels *lby = wp->yLabels;
   
   msg_dbg( "called");
   if ( wp->ud->up->showMoreYLabels ) {
      gtk_widget_hide(wp->ylabel_max);
      gtk_widget_hide(wp->ylabel_min);
      gtk_widget_show(lby->label_layout);
   } else {
      gtk_widget_hide(lby->label_layout);
      gtk_widget_show(wp->ylabel_max);
      gtk_widget_show(wp->ylabel_min);
   }
   pa_panel_draw_ylabels(wp);
}


/*
 * contents of the left measure box scrolled window
 */
void pa_panel_lmswtable_create(WavePanel *wp, UserData *ud)
{
   /*
    * table for buttons and masurements.
    * row 0 is space-filling dummy.
    */
   
   wp->lmswtable = gtk_grid_new (); 
   gtk_widget_set_vexpand(wp->lmswtable, TRUE );
   gtk_widget_show(wp->lmswtable);

   pa_panel_lmswtable_setup( wp );
}

/*
 * this box is lmtable fixed table
 */

GtkWidget *pa_panel_label_meas_box_create(WavePanel *wp, UserData *ud)
{
   GtkWidget *scrolled_window;
   GtkWidget *layout;
   GtkWidget *lmtable;

   lmtable = gtk_grid_new ();
   gtk_widget_set_halign (lmtable, GTK_ALIGN_FILL);
   gtk_widget_show(lmtable);

   /* top : ylabel_log  ylabel_max if moreYLabels = 0 */
   wp->ylabel_log = gtk_label_new("LogY");
   gtk_widget_show (wp->ylabel_log);
   gtk_widget_set_halign(wp->ylabel_log, GTK_ALIGN_START | GTK_ALIGN_FILL);
   gtk_grid_attach(GTK_GRID(lmtable), wp->ylabel_log, 
                /* left,    top,  width,   height    */
                     0,      0,      1,        1  );   
     /* ylabel_max */
   wp->ylabel_max = gtk_label_new("0.0");
   gtk_widget_show (wp->ylabel_max);
   gtk_widget_set_halign(wp->ylabel_max, GTK_ALIGN_END | GTK_ALIGN_FILL);
   gtk_grid_attach(GTK_GRID(lmtable), wp->ylabel_max, 
                /* left,    top,  width,   height    */
                     1,      0,      1,        1  );
   
   /* container ylabel_box if moreYLabels = 1 */
   /* create layout for ylabels */
   GawLabels *lby = wp->yLabels;
   layout =  gtk_layout_new (NULL, NULL);
   lby->label_layout = layout;
   gtk_widget_show (layout);
   gtk_grid_attach(GTK_GRID(lmtable), lby->label_layout, 
                /* left,    top,  width,   height    */
                      2,      0,      1,        3  );   

   /* create the content of the scrolled window */
   pa_panel_lmswtable_create(wp, ud);   

   /* wrap scrolled window with vertical scrollbar around table */
   scrolled_window = gtk_scrolled_window_new (NULL, NULL);
   gtk_widget_set_halign(scrolled_window, GTK_ALIGN_FILL);
   wp->lmscroll_win = scrolled_window ;
   g_object_ref (scrolled_window); /* increment ref to avoid destruction */
   gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW (scrolled_window),
				  GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
   gtk_scrolled_window_set_placement(GTK_SCROLLED_WINDOW (scrolled_window),
				     GTK_CORNER_TOP_RIGHT);
   GtkWidget *vscrollbar = gtk_scrolled_window_get_vscrollbar(GTK_SCROLLED_WINDOW(scrolled_window));
   gtk_widget_set_can_focus (vscrollbar, FALSE);
   gtk_widget_show (scrolled_window);
//   gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(scrolled_window),
//					 wp->lmswtable);
   gtk_container_add(GTK_CONTAINER(scrolled_window), wp->lmswtable);

   gtk_container_set_focus_hadjustment (GTK_CONTAINER (wp->lmswtable),
     gtk_scrolled_window_get_hadjustment (GTK_SCROLLED_WINDOW (scrolled_window)));
   gtk_container_set_focus_vadjustment( GTK_CONTAINER (wp->lmswtable),
     gtk_scrolled_window_get_vadjustment(GTK_SCROLLED_WINDOW(scrolled_window)));

   gtk_grid_attach(GTK_GRID(lmtable), wp->lmscroll_win,
                /* left,    top,  width,   height    */
                      0,      1,      2,        1  );   

   /* create 2 ylabels */
   al_label_add_remove(lby, 2);

   /* bottom:  ylabel min  if moreYLabels = 0 */
   wp->ylabel_min = gtk_label_new("0.0");
   gtk_widget_set_halign(wp->ylabel_min, GTK_ALIGN_END);
   gtk_widget_show (wp->ylabel_min);

   gtk_grid_attach(GTK_GRID(lmtable), wp->ylabel_min,
                /* left,    top,  width,   height    */
                      1,      2,      1,        1  );   

   pa_panel_label_meas_box_update(wp);
   return lmtable;
}


gint pa_panel_find_pos(WavePanel *wp)
{
   if ( wp ) {
      UserData *ud = wp->ud;
      return g_list_index( ud->panelList, wp);    
   }
   return -1;
}

WavePanel *
pa_panel_find_selected(UserData *ud)
{
   GList *list = ud->panelList;
   
   while (list) {
      GList *next = list->next;

      WavePanel *wp = (WavePanel *) list->data;  
      if ( wp->selected ){
	 return wp;
      }
      list = next;
   }
   return NULL;
}

void pa_panel_set_selected( WavePanel *wpsel, UserData *ud )
{
   GList *list = ud->panelList;
   
   while (list) {
      GList *next = list->next;

      WavePanel *wp = (WavePanel *) list->data;
      wp->selected = 0;
      if ( wp == wpsel ){
	 wp->selected = 1;
	 ud->selected_panel = wp;
         GAction *action = g_action_map_lookup_action ((GActionMap  *) wp->wpgroup, "pPLogY");
         int toggled = g_variant_get_boolean ( 
                                   g_action_get_state (G_ACTION (action)) );
         gm_update_toggle_state(wp->ud->group, "LogYPanel", toggled);
         if ( wp->ud->LogYPanel ){
            gtk_widget_set_sensitive (wp->ud->LogYPanel,
                                   al_label_get_logAble(wp->yLabels) );
         }
      }
      list = next;
   }
}
/*
 * Delete all waves in this panel
 */
void pa_panel_vw_delete(WavePanel *wp)
{
   VisibleWave *vw;
    
   while((vw = g_list_nth_data(wp->vwlist, 0)) != NULL) {
      wave_destroy(vw);
   }
   if ( wp->ud->xLabels ){
      ap_all_redraw(wp->ud);
   }
}

void pa_panel_lmscroll_win_set_size_request(WavePanel *wp)
{
   gtk_scrolled_window_set_min_content_width(
                      GTK_SCROLLED_WINDOW (wp->lmscroll_win),
                       wp->ud->up->lmtableWidth );
   gtk_widget_set_size_request (GTK_WIDGET(wp->lmtable),
			 wp->ud->up->lmtableWidth, -1 );

   msg_dbg( "w %d, h %d", wp->ud->up->lmtableWidth, -1  );
}

void pa_panel_drawing_set_gdk_cursor(WavePanel *wp, int cursorType )
{
   da_set_gdk_cursor(wp->drawing, cursorType);
}

void pa_panel_drawing_redraw(WavePanel *wp, gpointer data )
{
   da_drawing_redraw(wp->drawing);
}

void pa_panel_background_set(WavePanel *wp, gpointer data )
{
//   da_background_color_set (wp->drawing, wp->ud->bg_color );
   g_list_foreach(wp->vwlist, (GFunc) wave_color_set, NULL );
}

/*
 * called from da_drawing_configure_cb
 */
void pa_panel_label_size(WavePanel *wp)
{
   UserPrefs *up = wp->ud->up;
   GtkAllocation walloc;
   gtk_widget_get_allocation (wp->drawing, &walloc);
   
   if ( ! wp->drawing ) {
      return;
   }
   int h = walloc.height;
   int w = walloc.width;
   if ( w <  up->minPanelWidth || h < up->minPanelHeight ) {
      /* when recalculating tables, da comes with w = 1, and h = 1 */
      return;
   }
   UserData *ud = wp->ud ;
   GawLabels *lby = wp->yLabels;
   int width = ud->char_width * (6 + 2 * up->scientific);

//   msg_dbg("w %d lby->w %d , h %d, lby->h %d", w, lby->w, h, lby->h );
   if ( lby->h != h || lby->w != w  ){
      lby->w = w;
      lby->h = h;
      lby->wh = h;  /* y label box height */
      lby->char_width =  ud->char_width;
      lby->lbheight = ud->char_height;
      lby->changed |= CV_CHANGED;
      msg_dbg("Y w %d, h %d", lby->w, lby->h);
   }
   if ( lby->lbwidth != width ){
      /* label box width changed */
      lby->lbwidth = width;
      gtk_widget_set_size_request (GTK_WIDGET(lby->label_layout), width, -1);
      msg_dbg("Y w %d, h %d, lby width %d", lby->w, lby->h, width);
   }
      
   GawLabels *lbx = ud->xLabels;
   if ( lbx->w != w ){
      lbx->w = w;
      lbx->h = h;
      lbx->char_width =  ud->char_width;
      lbx->lbwidth = width;
      lbx->lbheight = ud->char_height;

      lbx->changed |= CV_CHANGED;
      msg_dbg("X w %d, h %d", w, h);
   }
}

void pa_panel_set_drawing_func(WavePanel *wp, gpointer data )
{
   wp->drawFunc = wavedraw_method_tab[wp->ud->up->drawAlgo].func ;
}

