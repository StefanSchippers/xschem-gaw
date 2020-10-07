/*
 * gawda.c - drawing area functions
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

#ifdef TRACE_MEM
#include <tracemem.h>
#endif
        


/*
 * simply forces widget w to redraw itself
 *    indirect call to expose event
 */
void da_drawing_redraw(GtkWidget *w)
{
   GdkRectangle rect;

   gtk_widget_get_allocation (w, &rect);
   rect.x = 0;
   rect.y = 0;

   if ( ! gtk_widget_get_realized (w) ) {
      return;
   }
   /* ask redraw */
   gdk_window_invalidate_rect (gtk_widget_get_window (w),
                               &rect, FALSE);
}

/* 
 * Set the X pointer cursor for all wavepanels: used to provide a
 * hint that we're expecting the user to drag out a line or region.
 */

void da_set_gdk_cursor(GtkWidget *w, int cursorType)
{
   GdkCursor *cursor;

   if (cursorType == -1) {
      cursor = NULL;
   } else {
      cursor = gdk_cursor_new(cursorType);
   }
   gdk_window_set_cursor(gtk_widget_get_window (w), cursor);

   if (cursor) {
      g_object_unref(cursor);
   }
}


void da_draw_srange(SelRange *sr)
{
   cairo_t *cr = sr->wp->cr;

   gdk_cairo_set_source_rgba (cr, sr->color);
   cairo_set_line_width (cr, 1.0);

   if (sr->type & SR_X) {
      cairo_move_to (cr, sr->x1, sr->y1);
      cairo_line_to (cr, sr->x2, sr->y1);
   }
   if (sr->type & SR_Y) {
      cairo_move_to (cr, sr->x1, sr->y1);
      cairo_line_to (cr, sr->x1, sr->y2);
   }
   if (sr->type == SR_XY) {
      cairo_move_to (cr, sr->x1, sr->y2);
      cairo_line_to (cr, sr->x2, sr->y2);

      cairo_move_to (cr, sr->x2, sr->y1);
      cairo_line_to (cr, sr->x2, sr->y2);
   }
   cairo_stroke (cr);
}

void
da_update_srange(SelRange *sr,  GdkEventMotion *event, int draw)
{
   int newx2, newy2;

   /*
    * the event->y does goofy things if the motion continues
    * outside the window, so we generate our own from the root
    * coordinates.
    */
   newx2 = event->x;
   newy2 = sr->y1 + (event->y_root - sr->y1_root);

   sr->drawn = draw;
   if (sr->type & SR_X) {
      sr->x2 = newx2;
   }
   if (sr->type & SR_Y) {
      sr->y2 = newy2;
   }
   /* will be forced to redrawd */
   msg_dbg( "type=%d newx=%d newy=%d draw=%d",
	   sr->type, sr->x2, sr->y2, draw);
   msg_dbg( "m %d %d %d %d",
	   (int) event->x, (int) event->y, 
	   (int) event->x_root, (int) event->y_root);
}

/*
 * done selecting range; do the callback
 */

void
da_callback_srange(UserData *ud, WavePanel *wp )
{
   SelRange *sr = ud->srange;
   double xstart, ystart;
   double xend, yend;
   GawLabels *lbx = ud->xLabels;
   GawLabels *lby = sr->wp->yLabels;
   
   msg_dbg( "type=%d x1=%d x2=%d  y1=%d y2=%d",
	   sr->type, sr->x1, sr->x2, sr->y1, sr->y2);

   xstart = al_label_x2val(lbx, sr->x1);
   xend   = al_label_x2val(lbx, sr->x2);

   ystart = al_label_y2val(lby, sr->y1 );
   yend   = al_label_y2val(lby, sr->y2 );
 
   if ( ystart < yend ) {
      pa_panel_set_yvals( wp, ystart, yend);
   } else {
      pa_panel_set_yvals( wp, yend, ystart);
   }

   switch (sr->type) {
    case SR_X:
      az_cmd_zoom_absolute(ud, xstart, xend );
      break;
    case SR_Y:
      wp->man_yzoom = 1;
      break;
    case SR_XY:
      wp->man_yzoom = 1;
      az_cmd_zoom_absolute(ud, xstart, xend );
      break;
   }
}


/*
 * drawing area configure handler
 * The "configure_event" signal to take any necessary actions
 *   when the widget changes size.
 */
static gboolean
da_drawing_configure_cb (GtkWidget *widget, GdkEventConfigure *event,
		      gpointer data )
{
   WavePanel *wp =  (WavePanel *) data;
   GtkAllocation walloc;
   
   gtk_widget_get_allocation (widget, &walloc);
   msg_dbg( "w %d h = %d new %d %d 0x%lx",
	    walloc.width,
	    walloc.height,
	    event->width,  event->height,
	    (long unsigned int) widget );

   if ( ! wp->ud->bg_color ) {
      ac_color_initialize(wp);
   }
   if ( wp->grid_color ) {
      pa_panel_color_grid_set(wp, wp->grid_color);
   }
   
   pa_panel_label_size(wp);
   wp->configure_seen = 1;
   return TRUE;
}

/*
 *  draw one wave in a panel
 */
void da_drawing_draw_wave (VisibleWave *vw, WavePanel *wp)
{
   cairo_t *cr = wp->cr;

   gdk_cairo_set_source_rgba (cr, vw->color);
   cairo_set_line_join (cr, CAIRO_LINE_JOIN_MITER);
   cairo_set_line_cap (cr, CAIRO_LINE_CAP_BUTT);

   double line_width = 1.0;
   if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(vw->button))) {
      line_width = 2.0;
   }
   cairo_set_line_width (cr, line_width);

   wp->drawFunc (vw, wp); /* call the selected drawing function */
   cairo_stroke (cr);
}
/*
 *  draw segments
 */
static void
da_draw_segments(WavePanel *wp, GawSegment *segp, gint nsegments )
{
   int i;
   cairo_t *cr = wp->cr;
   
   for ( i = 0 ; i < nsegments; i++ ){
      cairo_move_to (cr, segp->x1, segp->y1);
      cairo_line_to (cr, segp->x2, segp->y2);
      segp++;
   }
}

/*
 *  draw grid in a panel
 */
static void
da_drawing_draw_grid (WavePanel *wp)
{
   GtkAllocation walloc;
   gtk_widget_get_allocation (wp->drawing, &walloc);
   int w = walloc.width;
   int h = walloc.height;

   ArrayStruct *ary = array_struct_new( sizeof(GawSegment), 128, NULL);
   GawLabels *lbx = wp->ud->xLabels;
   GawLabels *lby = wp->yLabels;

   /* calculate grid segments */
   
   /* vertical lines */
   if ( al_label_do_logAxis( lbx) ) {
      al_label_draw_vlog_grid( lbx, ary, w, h );
   } else {
      al_label_draw_vlin_grid( lbx, ary, w, h );
   }

   /* horizontal lines */
   if ( al_label_do_logAxis( lby) ) {
      al_label_draw_hlog_grid( lby, ary, w, h );
   } else {
      al_label_draw_hlin_grid( lby, ary, w, h );
   }

   /* draw grid */
   da_draw_segments(wp, (GawSegment *) array_struct_table_get(ary),
                    array_struct_get_nelem(ary) );
   array_struct_destroy(ary);
}

void
da_drawing_post_configure (GtkWidget *widget, WavePanel *wp, int w, int h )
{
   UserData  *ud =   wp->ud;
   GawLabels *lby = wp->yLabels;

   wp->configure_seen = 0;
   /*
    * Need this here for getting data when they are valid for :
    *    - init purpose
    *    - set the size of label box :
    *         - after a main window change
    *         - main scroll bar show/hide
    */
   if ( (lby->changed & CV_INIT) == 0){ /* panel creation */
      lby->changed |= CV_INIT;
      pa_panel_update_min_max(wp);
   }
   if ( lby->changed & CV_CHANGED  ){
      lby->changed &= ~CV_CHANGED ;
      pa_panel_draw_ylabels( wp );
   }
   
   GawLabels *lbx = wp->ud->xLabels;
   if ( lbx->changed & (CV_CHANGED | CV_SBCHANGED) ){
      int vis = lbx->changed & CV_SBSHOW ;
      int xlwidth = w;
      
      if ( vis ) {
         xlwidth += ud->sbSize;
      }
      if ( xlwidth != lbx->wh ){ 
         gtk_widget_set_size_request (GTK_WIDGET(ud->xlabel_box),
                                      xlwidth, lbx->lbheight);
         lbx->wh = xlwidth; /* x label box width */
         lbx->changed |= CV_CHANGED ;
      }
      lbx->changed &= ~CV_SBCHANGED ;
      msg_dbg("w %d, h %d, lbx->wh %d, visible %d", lbx->w, lbx->h, lbx->wh, vis);
   }
   if ( (lbx->changed & CV_INIT) == 0){
      /* init data at first run */
      pa_panel_update_all_data(ud); /* this call al_label_draw */
      lbx->changed |= CV_INIT | CV_CHANGED;
      lbx->changed &= ~CV_CHANGED;
   }
   if ( lbx->changed & CV_CHANGED ){
      lbx->changed &= ~CV_CHANGED ;
      al_label_draw( lbx );
   }

   ud->panelHeight = h;
   ud->panelWidth = w;
//   msg_dbg("HQ1 w %d, h %d", ud->panelWidth, ud->panelHeight);
   GtkWidget *scr = gtk_scrolled_window_get_vscrollbar(
                            GTK_SCROLLED_WINDOW(ud->panel_scrolled)) ;
   if ( gtk_widget_get_visible (scr) == FALSE ) {
	 ud->panelWidth = w - ud->sbSize ;
   }
   if ( ud->panelWidth % 2 ) {
      ud->panelWidth += 1;
   }
//  msg_dbg("HQ2 w %d, h %d, sb %d", ud->panelWidth, ud->panelHeight, ud->sbSize);
}

void
da_drawing_draw_all (GtkWidget *widget, cairo_t *cr, WavePanel *wp,
                     int w, int h )
{
   UserData  *ud =   wp->ud;
   GawLabels *lby = wp->yLabels;
   int i;
   int y;
//   GdkRGBA color;

   wp->cr = cr;
//   GtkStyleContext *context = gtk_widget_get_style_context (widget);

   /* draw background */
//   gtk_style_context_get_background_color(context, GTK_STATE_FLAG_NORMAL, &color);
   gdk_cairo_set_source_rgba (cr, ud->bg_color);
   cairo_paint(cr);  /* set background */

   cairo_set_line_width (cr, 0.1);
   cairo_rectangle (cr, 0, 0,  w, h);
   cairo_stroke (cr);

   if (wp->selected) {
      gdk_cairo_set_source_rgba (cr, ud->hl_color );
      cairo_set_line_width (cr, 1.0);
      cairo_rectangle (cr, 1, 1,  w - 2, h - 2);
      cairo_stroke (cr);
   }
   /* set color for grid */
   gdk_cairo_set_source_rgba (cr, wp->grid_color);
   cairo_set_line_width (cr, 1.0);
   
   if ( wp->showGrid ) {
      /* graticule */
      da_drawing_draw_grid (wp);
   } else {
      /* draw horizontal line at y=zero. */
      if (lby->start_val < 0 && lby->end_val > 0) {
	 y = VAL2Y(lby, 0, h);
         cairo_move_to( cr, 0, y);
         cairo_line_to( cr, w, y);
      }
   }
   cairo_stroke (cr);
   
   /* draw waves */
   g_list_foreach(wp->vwlist, (GFunc) da_drawing_draw_wave, wp); 

   /* draw the 2 cursors in the panel */
   for (i = 0 ; i < 2 ; i++) {                        
      AWCursor *csp = ud->cursors[i];
      if (csp->shown) {
         gdk_cairo_set_source_rgba (cr, csp->color);
         cairo_set_line_width (cr, 1.0);
         cairo_move_to(cr, csp->x, 0);
         cairo_line_to(cr, csp->x, h);
         cairo_stroke (cr);
      }
   }

   /* draw select-range line, if in this WavePanel */
   if ( ud->srange && ud->srange->drawn && ud->srange->wp == wp) {
      da_draw_srange(ud->srange);
   }

   /* draw text */
   g_list_foreach(wp->textlist, (GFunc) gawtext_draw_text, wp);
}

/*
 * drawing area draw handler gtk3
 *  draw all in the panel
 */

gboolean
da_drawing_draw_cb (GtkWidget *widget, cairo_t *cr, gpointer data )
{
   WavePanel *wp =  (WavePanel *) data;
   UserData  *ud =   wp->ud;
   int w = gtk_widget_get_allocated_width(widget);
   int h = gtk_widget_get_allocated_height(widget);

   msg_dbg( "called width %d height %d 0x%lx", w, h, (long unsigned int) widget );

   if ( wp->configure_seen ) {
         da_drawing_post_configure (widget, wp, w, h);
   }
   if ( ud->suppress_redraw ) {
      return FALSE;
   }

   /* Panel drawing with cairo */
   da_drawing_draw_all (widget, cr, wp, w, h );

   return FALSE;
}

char *da_statusFormat = N_("Panel W %d H %d X %d Y %d");

/*
 * drawing area mouse button press handler
 */
static gboolean
da_drawing_button_press_cb (GtkWidget *widget, GdkEventButton *event,
			 gpointer data )
{
   WavePanel *wp =  (WavePanel *) data;
   UserData  *ud =   wp->ud;
   GawText *gtext;
   
   msg_dbg( "button %d state %d mouseState %d",
	   event->button, event->state, ud->mouseState  );

   if ( wp->grid_color == NULL){
      return FALSE; /* we haven't gotten a configure event */
   }
   if ( event->button == 3 ){
      GtkWidget *menu;
      
      if ( wp->selected ) {
	 /* unselect all */
	 pa_panel_set_selected( NULL, ud );
	 da_drawing_redraw(wp->drawing);
      }
      gtext = (GawText *) pa_panel_inside_text( wp, event->x, event->y);
      if ( gtext ){
         menu = wp->textpopmenu;
         g_object_set_data (G_OBJECT(menu), "PanelTextPopup-action", (gpointer) gtext);
      } else {
         menu = wp->popmenu;
      }
      
      gtk_menu_popup (GTK_MENU (menu), NULL, NULL,
		      NULL, NULL,  3, event->time);

      return TRUE;
   }
   char text[256];

   GtkAllocation walloc;
   gtk_widget_get_allocation (widget, &walloc);
   if ( event->button == 1 ) {
      sprintf (text, gettext(da_statusFormat), walloc.width,
               walloc.height, (int) event->x, (int) event->y );
      gtk_statusbar_push (GTK_STATUSBAR(ud->statusbar), 2, text);
   }
   switch (ud->mouseState) {
    case M_NONE:
      pa_panel_set_selected( wp, ud );
      gtk_grab_add(widget);
      ud->button_down = event->button;

      gtext = (GawText *) pa_panel_inside_text( wp, event->x, event->y);
      if ( gtext ){
         ud->mouseState = M_TEXT_DRAG;
         gtext->cx = (int) event->x;
         gtext->cy = (int) event->y;
         gtext->maxwidth = walloc.width;
         gtext->maxheight = walloc.height;
      } else {
         ud->mouseState = M_CURSOR_DRAG;
         ud->drag_button = event->button;
         da_set_gdk_cursor(widget, GDK_SB_H_DOUBLE_ARROW);
         cu_display_xcursor(wp, ud->drag_button, event->x, 0);
      }
      break;

    case M_SELRANGE_ARMED:
      gtk_grab_add(widget);
      ud->button_down = event->button;
      ud->mouseState = M_SELRANGE_ACTIVE;

      ud->srange->y1 = ud->srange->y2 = event->y;
      ud->srange->x1 = ud->srange->x2 = event->x;
      ud->srange->x1_root = event->x_root;
      ud->srange->y1_root = event->y_root;
      ud->srange->wp = wp;

      break;
      /* can't start another drag until first one done */

    case M_DRAW_TEXT:
      ud->button_down = event->button;
      break;

    case M_CURSOR_DRAG:
    case M_SELRANGE_ACTIVE:
    default:
      break;
   }
   ap_all_panel_redraw(ud);

   return TRUE;
}
/*
 * drawing area mouse button release handler
 */
static gboolean
da_drawing_button_release_cb (GtkWidget *widget, GdkEventButton *event,
			   gpointer data )
{
   WavePanel *wp =  (WavePanel *) data;
   UserData  *ud =  wp->ud;

   app_memcheck();
   msg_dbg( "button %d state %d", event->button, ud->mouseState );

   if ( wp->grid_color == NULL || ud->button_down != event->button ){
      return FALSE; /* we haven't gotten a configure event */
   }
   if ( event->button == 1 ) {
      gtk_statusbar_pop (GTK_STATUSBAR(ud->statusbar), 2);
   }
   GdkWindow *window = gtk_widget_get_window (widget);
   switch(ud->mouseState) {
    case M_TEXT_DRAG:
    case M_CURSOR_DRAG:
      gtk_grab_remove(widget);
      gdk_window_set_cursor(window, NULL);
      if ( ud->mouseState == M_CURSOR_DRAG) {
         cu_display_xcursor(wp, ud->drag_button, event->x, 0);
      }
      ud->drag_button = -1;
      break;

    case M_SELRANGE_ACTIVE:
      gtk_grab_remove(widget);
      g_list_foreach(ud->panelList, (GFunc) pa_panel_drawing_set_gdk_cursor,
		     GINT_TO_POINTER (-1) ); /* clear gdk cursor */
      da_update_srange(ud->srange, (GdkEventMotion *) event, 0);
      da_callback_srange(ud, wp);
      break;

    case M_DRAW_TEXT:
      ud->gtexttmp = NULL;      
      ud->mouseState = M_NONE;
      break;

    default:
      break;
   }
   ud->button_down = -1;
   ud->mouseState = M_NONE;
   pa_panel_full_redraw(wp);
   return TRUE;
}

/*
 * drawing area mouse motion handler
 */
static gboolean
da_drawing_motion_cb (GtkWidget *widget, GdkEventMotion *event, gpointer data )
{
   WavePanel *wp =  (WavePanel *) data;
   UserData  *ud =  wp->ud;
   char text[256];
   GawText *gtext;
   int x = (int) event->x;
   int y = (int) event->y;
   
//   msg_dbg( "state %d, x %d, y %d", ud->mouseState, x, y );

   if ( wp->grid_color == NULL){
      return FALSE; /* we haven't gotten a configure event */
   }
   GtkAllocation walloc;
   gtk_widget_get_allocation (widget, &walloc);
   if (ud->up->xconvert == 1 ) {
      time_t mytime = (time_t) al_label_x2val(ud->xLabels,  x );
      convert_time_t_to_date( mytime, ud->up->date_fmt, text, sizeof(text) );
   } else {
      sprintf (text, gettext(da_statusFormat), walloc.width,
               walloc.height, x, y );
   }
   gtk_label_set_text ( GTK_LABEL(ud->statusLabel), text); 

   switch(ud->mouseState) {
    case M_NONE:
      gtext = (GawText *) pa_panel_inside_text( wp, x, y);
      if ( gtext) {
         da_set_gdk_cursor(widget, GDK_HAND2);
      } else {
         da_set_gdk_cursor(widget, -1);
      }
      break;
    case M_CURSOR_DRAG:
      cu_display_xcursor(wp, ud->drag_button, x, 1);
      break;

    case M_SELRANGE_ACTIVE:
      da_update_srange(ud->srange, event, 1);
      da_drawing_redraw(widget);
      break;
      
    case M_TEXT_DRAG:
      gtext = (GawText *) pa_panel_inside_text( wp, x, y);
      if ( ! gtext) {
         break;
      }
      gawtext_update_pos( gtext, x, y);
      da_drawing_redraw(widget);
      break;
      
    case M_DRAW_TEXT:
      gawtext_update_pos( ud->gtexttmp, event->x, event->y);
      da_drawing_redraw(widget);
      break;
      
    default:
      break;
   }

   return TRUE;
}

/*
 * drawing area mouse motion handler
 */
static gboolean
da_drawing_crossing_cb (GtkWidget *widget, GdkEventCrossing *event, gpointer data )
{
   WavePanel *wp =  (WavePanel *) data;
   UserData  *ud =  wp->ud;

   if ( wp->grid_color == NULL){
      return FALSE; /* we haven't gotten a configure event */
   }

//   msg_dbg( "state %d, type %s", ud->mouseState,
//            (event->type == GDK_ENTER_NOTIFY ? "Enter" : "Leave") );
   switch(ud->mouseState) {
    case M_DRAW_TEXT:
      if ( event->type == GDK_ENTER_NOTIFY) {
         GtkAllocation walloc;
         gtk_widget_get_allocation (widget, &walloc);
         wp->textlist = g_list_prepend(wp->textlist, ud->gtexttmp);
         ud->gtexttmp->maxwidth = walloc.width;
         ud->gtexttmp->maxheight = walloc.height;
      } else if ( event->type == GDK_LEAVE_NOTIFY) {
         wp->textlist = g_list_remove(wp->textlist, ud->gtexttmp);
      }
      da_drawing_redraw(widget);
      break;

    default:
      break;
   }

   return TRUE;
}

/*
 * da are created to their minimal size
 * They automtically expand to the main window allocated size.
 */

void da_drawing_set_size_request(GtkWidget *drawing, int w, int h)
{
   gtk_widget_set_size_request (GTK_WIDGET(drawing), w, h );
   msg_dbg( "w %d, h %d", w, h );
}


/*
 * Construct drawing area.
 */ 
 GtkWidget *da_drawing_create( WavePanel *wp )
{
   UserData *ud = wp->ud;
   UserPrefs *up = ud->up;
   GtkWidget *drawing;
   
   msg_dbg("width = %d, height = %d, showXlabels = %d",
	   up->panelWidth, up->panelHeight, up->showXLabels);

   /* drawing area for waveform */
   drawing = gtk_drawing_area_new();
   gtk_widget_set_name( drawing, "wavepanel");
   gtk_widget_set_hexpand(drawing, TRUE );
   g_object_ref (drawing); /* increment ref to avoid destruction */

   g_signal_connect( drawing, "draw", 
		     G_CALLBACK (da_drawing_draw_cb), (gpointer) wp);
   g_signal_connect (drawing,"configure_event",
		     G_CALLBACK (da_drawing_configure_cb), (gpointer) wp);

   g_signal_connect( drawing, "button_press_event", 
		     G_CALLBACK (da_drawing_button_press_cb), (gpointer) wp);
   g_signal_connect( drawing, "button_release_event", 
		     G_CALLBACK (da_drawing_button_release_cb), (gpointer) wp);
   g_signal_connect( drawing, "motion_notify_event", 
		     G_CALLBACK ( da_drawing_motion_cb), (gpointer) wp);
   g_signal_connect( drawing, "enter_notify_event", 
		     G_CALLBACK ( da_drawing_crossing_cb), (gpointer) wp);
   g_signal_connect( drawing, "leave_notify_event", 
		     G_CALLBACK ( da_drawing_crossing_cb), (gpointer) wp);

   da_drawing_set_size_request(drawing, up->minPanelWidth, up->minPanelHeight );
   
   gtk_widget_show(drawing);
   
   /* Set up a drawing as a drop target */
   ad_set_drag_dest(drawing, ud, wp, DND_PANEL);

   gtk_widget_set_events(drawing, GDK_EXPOSURE_MASK |
			 GDK_BUTTON_RELEASE_MASK |
			 GDK_BUTTON_PRESS_MASK |
			 GDK_POINTER_MOTION_MASK |
			 GDK_BUTTON1_MOTION_MASK |
			 GDK_BUTTON2_MOTION_MASK |
                         GDK_LEAVE_NOTIFY_MASK |
                         GDK_ENTER_NOTIFY_MASK );
   return drawing;
}

