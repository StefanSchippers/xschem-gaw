/*
 * gawlabel.c - X or Y label or grid  functions
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

#include <gawlabel.h>
#include <gaw.h>
 
#ifdef TRACE_MEM
#include <tracemem.h>
#endif
        

/*
 *** \brief Allocates memory for a new GawLabels object.
 */

GawLabels *al_label_new( UserPrefs *up, int logAxis, int dir )
{
   GawLabels *lb;

   lb = app_new0(GawLabels, 1);
   al_label_construct( lb, up, logAxis, dir );
   app_class_overload_destroy( (AppClass *) lb, al_label_destroy );
   return lb;
}

/** \brief Constructor for the GawLabels object. */

void al_label_construct( GawLabels *lb, UserPrefs *up, int logAxis, int dir )
{
   app_class_construct( (AppClass *) lb );

   al_label_set_logAxis(lb, logAxis );
   lb->up = up;
   lb->xdir = dir;
}

/** \brief Destructor for the GawLabels object. */

void al_label_destroy(void *lb)
{
   GawLabels *this = (GawLabels *) lb;

   if (lb == NULL) {
      return;
   }
   g_list_foreach(this->label_list, (GFunc) lbd_destroy, this);
   g_list_free(this->label_list);

   app_class_destroy( lb );
}

int al_label_count_label(GawLabels *lb, int flag)
{
   int i;
   int j;
   double val;
   double step ;
   int max_count = g_list_length( lb->label_list);
   int count = 0;
   LabelData *lbd;
   

   for ( i = 0 ;  ; i++ ) {
      step = pow(10, lb->lb_Lstart + i ); /* 1, 2, 3 ... => 10, 100, 1000 */
      val = 0.0;
      for (j = 0; j < 9 ; j++ ) {
         val += step;
         if ( val < lb->start_val ) {
            continue;
         }
	 if ( val >= lb->end_val || count >= max_count ) {
	    return count;
	 }
         if ( (flag == 1 && j > 1 && (j % 2) == 0) ||
               (flag == 0 && j > 0) ) {
            continue;
         }
	 lbd = (LabelData *) g_list_nth_data ( lb->label_list, count++ );
	 lbd->lbval = val;
	 msg_dbg( "Log %s, val %f, step %f, count %d", XDIR(lb), val, step, count);
	 if ( flag == 0 ) {
            break;
         }
      }
   }
   return count;
}

void al_label_log_compute(GawLabels *lb, int max_labels)
{
   if ( ! lb->logAble ) {
      return;
   }
   double diff;
   
   lb->lb_start = floor(lb->start_Lval);
   lb->lb_Lstart = lb->lb_start;
   if ( lb->lb_start != lb->start_Lval ){
       lb->lb_start += 1.0;
   }
   
   diff = lb->end_Lval - lb->start_Lval ;

   if ( diff < 1.0 ) {
      lb->nlabels = al_label_count_label(lb, 2);
   } else if ( diff < 2.0 ) {
      lb->nlabels = al_label_count_label(lb, 1);
   } else {
      lb->nlabels = al_label_count_label(lb, 0);
   }
   msg_dbg("Log %s, [%.3f %.3f] [%.3f %.3f] start %.3f Lstart %.3f, nlabels (%d < %d)", XDIR(lb),
	   lb->start_val, lb->end_val, lb->start_Lval, lb->end_Lval, 
	   lb->lb_start, lb->lb_Lstart, lb->nlabels, max_labels );
}


void al_label_lin_compute(GawLabels *lb, int max_labels)
{
   int i;
   double nstep;
   LabelData *lbd;
   
   nstep = (lb->end_val - lb->start_val) / (double) max_labels;
   lb->step = find_rounded(nstep, 0);
   lb->lb_start = find_rounded(lb->start_val, 0);
   i = (int) ( (lb->lb_start - lb->start_val) / lb->step );
   if ( i > 0 ) {
      lb->lb_start = lb->lb_start - ( (double) i * lb->step);
   }
   if ( fabs(lb->lb_start) < lb->step ) {
      lb->lb_start = 0.0;
   }
   for ( i = 0 ; i < max_labels ; i++ ) {
      double val = lb->lb_start + ( (double) i * lb->step);
//      msg_dbg("Lin %s, [%d] val %f end val %", XDIR(lb),
//	      i, val,  lb->end_val );
      lbd = (LabelData *) g_list_nth_data ( lb->label_list, i );
      lbd->lbval = val;
      if ( val > lb->end_val ) {
	 break;
      }
   }
   lb->nlabels = i;
   msg_dbg("Lin %s, [%.3f %.3f] start %.3f step %.3f, nlabels (%d < %d)", XDIR(lb),
	   lb->start_val, lb->end_val, lb->lb_start, lb->step, lb->nlabels,
	   max_labels );

}

void al_label_compute(GawLabels *lb)
{
   int max_labels;
   
   if ( ! lb->wh ){
      return;
   }
   if ( lb->xdir ) { /* X label */
      max_labels = lb->wh / lb->lbwidth;
   } else {         /* Y label */
      max_labels = lb->wh / ( lb->lbheight + ( lb->lbheight / 4));
   }
   al_label_add_remove( lb, max_labels );
   if ( lb->logAxis ){
      al_label_log_compute(lb, max_labels);
   } else {
      al_label_lin_compute(lb, max_labels);
   }
}

void al_label_set_logAxis(GawLabels *lb, int logAxis )
{
   lb->logAxis = logAxis;
}

int al_label_get_logAxis(GawLabels *lb )
{
   return lb->logAxis;
}

int al_label_do_logAxis(GawLabels *lb )
{
   return (lb->logAble & lb->logAxis);
}

int al_label_get_logAble(GawLabels *lb )
{
   return lb->logAble;
}

void al_label_update_vals(GawLabels *lb, double start, double end )
{
   lb->start_val = start;
   lb->end_val = end;

   if ( lb->logAble ) {
       lb->start_Lval = log10(start);
       lb->end_Lval = log10(end);
   }
}

void al_label_update_min_max_vals(GawLabels *lb, double min, double max )
{
   lb->min_val = min;
   lb->max_val = max;
   if ( lb->min_val > 0.0 ) { 
      lb->logAble = 1;
      lb->min_Lval = log10(lb->min_val);
      lb->max_Lval = log10(lb->max_val);
   } else {
      lb->logAble = 0;
   }
}

double al_label_get_label_val( GawLabels *lb, double i )
{
   double val =  lb->lb_start + lb->step * i;
   if ( val < lb->start_val ) {
      val = lb->start_val;
   }
   if ( val > lb->end_val ) {
      val = lb->end_val;
   }
   return val;
}


void lbd_destroy( LabelData *lbd, gpointer data)
{
   GawLabels *lb = (GawLabels *) data;
   GtkWidget *label;

   label = lbd->label;
   /* destroy the label as well because it has only 1 reference */
   gtk_container_remove (GTK_CONTAINER(lb->label_layout), label);

   app_free(lbd);
   lb->label_list = g_list_remove (lb->label_list, lbd );
   app_memcheck();
}


/*
 * remove a gtklabel from the list
 */
void al_label_remove( GawLabels *lb )
{
   LabelData *lbd;
   int n;

   if ( ! lb->label_list ){
      return ;
   }
      
   n =  g_list_length( lb->label_list);

   lbd = (LabelData *) g_list_nth_data ( lb->label_list, n - 1 );
   lbd_destroy( lbd, (gpointer) lb);
}


/*
 * add a gtklabel to the list
 */
void al_label_add( GawLabels *lb )
{
   GtkWidget *label;
   LabelData *lbd;
   
   label = gtk_label_new(" ");
   gtk_widget_show(label);
   lbd = app_new0( LabelData, 1 );
   lbd->label = label;
   
   lb->label_list = g_list_append(lb->label_list, lbd);
   gtk_layout_put (GTK_LAYOUT(lb->label_layout), label, 1, 1);
}


void al_label_add_remove(GawLabels *lb, int nreq)
{
   int i;
   
   int nexisting = g_list_length( lb->label_list);
   msg_dbg("%s,  req nlabels %d existing %d", XDIR(lb), nreq, nexisting);
   if ( nreq == nexisting ) {
      return;
   } else if ( nreq < nexisting ) {
      for ( i = 0 ; i < nexisting - nreq ; i++ ){
	 al_label_remove(lb);
      }
   } else {
      for ( i = 0 ; i < nreq - nexisting ; i++ ){
         al_label_add( lb );
      }
   }
}

void al_label_get_pos(GawLabels *lb, double val, int i, int *xpos, int *ypos)
{
   int x = 0;
   int y = 0;
   
   if ( lb->xdir ) { /* X label */
      if ( al_label_do_logAxis(lb) ) {
         x =  VAL2LX(lb, val, lb->wh);
      } else {
         x =  VAL2X(lb, val, lb->wh);
      }
      if ( x >= lb->char_width ) {
	 x -= lb->char_width;
      }
   } else {         /* Y label */
      if ( al_label_do_logAxis(lb) ) {
         y =  VAL2LY(lb, val, lb->wh);
      } else {
         y =  VAL2Y(lb, val, lb->wh);
      }
      if ( y >= lb->lbheight ) {
	 y = y - lb->lbheight + 5;
      } else {
	 y -= 3;
      }
   }
   *xpos = x;
   *ypos = y;
}

/*
 * update text labeling the waveform graphs X or Y axis
 */
void al_label_draw(GawLabels *lb)
{
   int i;
   int x;
   int y;
   LabelData *lbd;
   int nlabels = g_list_length( lb->label_list);
   
   for ( i = 0 ; i < lb->nlabels ; i++ ){
      lbd = (LabelData *) g_list_nth_data ( lb->label_list, i );
      double val = lbd->lbval;
      al_label_get_pos(lb, val, i, &x, &y);
      msg_dbg("%s, [val %.3f] i %d x %d y %d", XDIR(lb), val, i, x, y);
      gtk_layout_move (GTK_LAYOUT(lb->label_layout), lbd->label, x, y);
      gtk_label_set_text(GTK_LABEL(lbd->label),
                 val2str_lb(val, lb->up->scientific) );
      gtk_widget_show(lbd->label);
   }
   for (  ; i < nlabels ; i++ ){
      lbd = (LabelData *) g_list_nth_data ( lb->label_list, i );
      gtk_widget_hide(lbd->label);
   }
}


/*
 * vertical lines, linear scale
 */
int al_label_draw_vlin_grid(GawLabels *lb, ArrayStruct *ary, int w, int h )
{
   guint i;
   int n = 2;
  
   for ( i = 0; i < lb->nlabels * n ; i++ ) {
      double val =  al_label_get_label_val( lb, (double) i / (double) n ) ;
      if ( val >= lb->end_val ) {
	 break;
      }
      GawSegment *segp = (GawSegment *) array_struct_next(ary);
      segp->x1 = segp->x2 = VAL2X(lb, val, w);
//      msg_dbg( "%s, val %f, x %d", XDIR(lb), val, segp->x1);
      segp->y1 = 0;
      segp->y2 = h - 1;
   }
   return array_struct_get_nelem(ary);
}

/*
 * horizontal lines, linear scale
 */
int al_label_draw_hlin_grid(GawLabels *lb, ArrayStruct *ary, int w, int h )
{
   guint i;
   int n = 2;
  
   for ( i = 0; i < lb->nlabels * n ; i++ ) {
      double val =  al_label_get_label_val( lb, (double) i / (double) n ) ;
      if ( val >= lb->end_val ) {
	 break;
      }
      GawSegment *segp = (GawSegment *) array_struct_next(ary);
      segp->y1 = segp->y2 = VAL2Y(lb, val, h);
//      msg_dbg( "%s, val %f, y %d", XDIR(lb), val, segp->y1);
      segp->x1 = 0;
      segp->x2 = w - 1;
   }
   return array_struct_get_nelem(ary);
}

/*
 * vertical lines, log scale
 */

int al_label_draw_vlog_grid(GawLabels *lb, ArrayStruct *ary, int w, int h )
{
   int i;
   int j;
   double val;
   double step ;

   for ( i = 0 ;  ; i++ ) {
      step = pow(10, lb->lb_Lstart + i ); /* 1, 2, 3 ... => 10, 100, 1000 */
      val = 0.0;
      for (j = 0; j < 9 ; j++ ) {
         val += step;
         if ( val < lb->start_val ) {
            continue;
         }
	 if ( val >= lb->end_val ) {
	    return array_struct_get_nelem(ary) ;
	 }
         GawSegment *segp = (GawSegment *) array_struct_next(ary);
	 segp->x1 = segp->x2 = VAL2LX(lb, val, w);
//	 msg_dbg( "Log %s, val %f, step %f, x %d", XDIR(lb), val, step, segp->x1);
	 segp->y1 = 0;
	 segp->y2 = h - 1;
      }
   }
   return array_struct_get_nelem(ary) ;
}

/*
 * horizontal lines, log scale
 */
int al_label_draw_hlog_grid(GawLabels *lb, ArrayStruct *ary, int w, int h )
{
   int i;
   int j;
   double val;
   double step;

   for ( i = 0 ;   ; i++ ) {
      step = pow(10, lb->lb_Lstart + i); /* 1, 2, 3 ... => 10, 100, 1000 */
      val = 0.0;
      for (j = 0; j < 9 ; j++ ) {
         val += step;
	 if ( val < lb->start_val ) {
	    continue;
	 }
	 if ( val >= lb->end_val ) {
	    return array_struct_get_nelem(ary);
	 }
         GawSegment *segp = (GawSegment *) array_struct_next(ary);
	 segp->y1 = segp->y2 = VAL2LY(lb, val, h);
//	 msg_dbg( "%s, val %f, step %f, y %d", XDIR(lb), val, step, segp->y1);
	 segp->x1 = 0;
	 segp->x2 = w - 1;
      }
   }
   return array_struct_get_nelem(ary);
}


/******************************************************/

/*
 * Linear scale
 * data        |min        |val            max|
 * view            |start  |val     end|
 * pixel           |0      |x     width|
 * 
 * x_pixel = width * (val - start) / (end - start)
 * 
 * What is a log scale ?
 * A logarithmic scale is a scale of measurement that uses the logarithm
 *   of a physical quantity instead of the quantity itself.
 *   a linear space           |0      |x     width|
 *   mapped to                |Lstart |Lval   Lend|
 *   Lstart = log10 (Fstart)   : 10     hz -> 1
 *   Lend = log10 (Fend)       : 100000 hz -> 5
 *   Lval = log10 (Fval)       : 1000   hz -> 3
 */

/*
 * convert a variable value to a normalized value 0.0 ... 1.0
 */
double al_label_val2n(GawLabels *lb, double val)
{
   if ( lb->end_val == lb->start_val ) {
      return 0.0;
   }
   return (val - lb->start_val) /  (lb->end_val - lb->start_val);
}

double al_label_val2Ln(GawLabels *lb, double val)
{
   if ( lb->end_Lval == lb->start_Lval ) {
      return 0.0;
   }
   return (log10(val) - lb->start_Lval) / (lb->end_Lval - lb->start_Lval);
}

int al_label_val2xy(GawLabels *lb, double val, int d)
{
   return d * al_label_val2n(lb, val);
}

int al_label_val2Lxy(GawLabels *lb, double val, int d)
{
   return d * al_label_val2Ln(lb, val);
}

/*
 * convert pixmap X or Y coordinate to user dependent-variable value
 */
double al_label_xy2val(GawLabels *lb, int v, int d)
{
   double frac = (double) v / (double) d;
   return lb->start_val + (frac * (lb->end_val - lb->start_val));
}

double al_label_xy2Lval(GawLabels *lb, int v, int d)
{
   double frac = (double) v / (double) d;
   double a = lb->start_Lval + (frac * ( lb->end_Lval - lb->start_Lval)) ;
   return pow(10, a);
}

/*
 * convert double yval to y coordinate
 */
int al_label_val2y(GawLabels *lby, double yval)
{
   if ( al_label_do_logAxis(lby) ) {
      return  VAL2LY(lby, yval, lby->h);
   } else {
      return  VAL2Y(lby, yval, lby->h);
   }
}
/*
 * convert y coordinate to double yval 
 */
double al_label_y2val(GawLabels *lby, int y)
{
   if ( al_label_do_logAxis(lby) ) {
      return  Y2LVAL(lby, y, lby->h);
   } else {
      return  Y2VAL(lby, y, lby->h);
   }
}
/*
 * convert double xval to x coordinate
 */
int al_label_val2x(GawLabels *lbx, double xval)
{
   if ( al_label_do_logAxis(lbx) ) {
      return  VAL2LX(lbx, xval, lbx->w);
   } else {
      return  VAL2X(lbx, xval, lbx->w);
   }
}
/*
 * convert x coordinate to double xval 
 */
double al_label_x2val(GawLabels *lbx, int x)
{
   if ( al_label_do_logAxis(lbx) ) {
      return  X2LVAL(lbx, x, lbx->w);
   } else {
      return  X2VAL(lbx, x, lbx->w);
   }
}

