/*
 * gawdraw.c - functions to draw the waveform in drawingarea.
 * If you want to write you display function, hereis the place.
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
#include <gawdraw.h>

#ifdef TRACE_MEM
#include <tracemem.h>
#endif

void gawdraw_wave_algo0(VisibleWave *vw, WavePanel *wp);
void gawdraw_wave_algo(VisibleWave *vw, WavePanel *wp);
void gawdraw_lineclip(VisibleWave *vw, WavePanel *wp);

AwSubmenuList wave_algo_desc_tab[] = {
   { "per-pixel",          "system-run" },
   { "per-pixel_modified", "system-run" },
   { "correct-line",       "system-run" },
   { NULL, NULL },
};

WaveDrawMethod wavedraw_method_tab[] = {
   { gawdraw_wave_algo0 },
   { gawdraw_wave_algo  },
   { gawdraw_lineclip   },
   { NULL },
};


/*
 *  draw one wave in a panel - gwave algo original
 *  sweep from x and calculate  y point
 *
 */
void gawdraw_wave_algo0 (VisibleWave *vw, WavePanel *wp)
{
   int x0, x1;
   int y0, y1;
   int i;
   double xstep;
   double xval;
   double yval;
   double xmin = wavevar_ivar_get_min(vw->var) ;
   double xmax = wavevar_ivar_get_max(vw->var) ;
   GawLabels *lbx = wp->ud->xLabels;
   GawLabels *lby = wp->yLabels;
   cairo_t *cr = wp->cr;
   
   xstep = (lbx->end_val - lbx->start_val) / lbx->w;  /* linear only */

   xval = lbx->start_val;
   yval =  wavevar_interp_value(vw->var, xval);
   x1 = 0;
   y1 = al_label_val2y(lby, yval);

   for ( i = 0 ; i < lbx->w ; i++ ) {
      x0 = x1;
      y0 = y1;
      x1 = x0 + 1;
      if ( al_label_do_logAxis(lbx) ) {
         xval = X2LVAL(lbx, x1, lbx->w);
      } else {
         xval += xstep;
      }
      if (xmin <= xval && xval <= xmax ) {
         yval = wavevar_interp_value(vw->var, xval);
         y1 = al_label_val2y(lby, yval);         
         if ( x0 >= 0 ){
            cairo_move_to (cr, x0, y0);
            cairo_line_to (cr, x1, y1);
         }
      }
   }
}

/*
 *  draw one wave in a panel - gwave algo modified
 *  sweep from x and calculate  y point
 */
void gawdraw_wave_algo (VisibleWave *vw, WavePanel *wp)
{
   int x0 = -1;
   int x1;
   int y0 = 0;
   int y1;
   int i;
   double xval;
   double yval;
   GawLabels *lbx = wp->ud->xLabels;
   GawLabels *lby = wp->yLabels;
   cairo_t *cr = wp->cr;
   
   for ( i = 0 ; i < lbx->w ; i++ ) {
      xval = al_label_x2val(lbx, i);
      yval = wavevar_get_yvalue(vw->var, xval, lbx->npoints);
      x1 = i;
      y1 = al_label_val2y(lby, yval);
//      fprintf(stderr, "i %d, x0 = %d, y0 = %d, x1 = %d, y1 = %d\n",
//              i, x0, y0,  x1, y1);
      if ( x0 >= 0 ){
         cairo_move_to (cr, x0, y0);
         cairo_line_to (cr, x1, y1);
      }
      x0 = x1;
      y0 = y1;
   }
}


/*-----------------------------------------------------------------------
 * see http://en.wikipedia.org/wiki/Cohen%E2%80%93Sutherland_algorithm
 -----------------------------------------------------------------------*/
/* swap integer */
void swap_i(int *a, int *b)
{
   int temp;

   temp = *a;
   *a = *b;
   *b = temp;
   return;
}

/* swap doouble */
void swap_d(double *a, double *b)
{
   double temp;

   temp = *a;
   *a = *b;
   *b = temp;
   return;
}

/*-----------------------------------------------------------------------
 * see http://en.wikipedia.org/wiki/Cohen%E2%80%93Sutherland_algorithm
 -----------------------------------------------------------------------*/
int point_code(double x, double y,
	       double xmin, double ymin, double xmax, double ymax)
{
   int ic;

   ic = 0;
   if (x < xmin){
      ic = 1;
   } else if (x > xmax){
      ic = 2;
   }
   if (y < ymin){
      ic += 4;
   } else if (y > ymax){
      ic += 8;
   }
   return ic;
}

static int line_clip(double *x1, double *y1, double *x2, double *y2,
	      double xmin, double ymin, double xmax, double ymax)
{
   int i1, i2;

/* Code each point.   */
   i1 = point_code(*x1, *y1, xmin, ymin, xmax, ymax);
   i2 = point_code(*x2, *y2, xmin, ymin, xmax, ymax);

   /* ARE BOTH ENDPOINTS IN VIEWING PORT? */
   while (i1 != 0 || i2 != 0) {
      if ((i1 & i2) != 0) {
	 /* Is segment outside viewport?   */
	 return FALSE;
      }
      if (i1 == 0) {
	 /* Is point 1 outside viewport?   */
	 swap_i(&i1, &i2);
	 swap_d(x1, x2);
	 swap_d(y1, y2);
      }

      if ((i1 & 1) != 0) {
	 /* Move toward left edge.   */
	 *y1 = *y1 + (*y2 - *y1) * (xmin - *x1) / (*x2 - *x1);
	 *x1 = xmin;
      } else {
	 if ((i1 & 2) != 0) {
	    /* Move toward right edge.   */
	    *y1 = *y1 + (*y2 - *y1) * (xmax - *x1) / (*x2 - *x1);
	    *x1 = xmax;
	 } else {
	    if ((i1 & 4) != 0) {
	       /* Move toward bottom.   */
	       *x1 = *x1 + (*x2 - *x1) * (ymin - *y1) / (*y2 - *y1);
	       *y1 = ymin;
	    } else {
	       /* Move toward top.   */
	       *x1 = *x1 + (*x2 - *x1) * (ymax - *y1) / (*y2 - *y1);
	       *y1 = ymax;
	    }
	 }
      }
      i1 = point_code(*x1, *y1, xmin, ymin, xmax, ymax);
   }
   return TRUE;
}

void gawdraw_lineclip(VisibleWave *vw, WavePanel *wp)
{
   int x0, x1;
   int y0, y1;
   int i;
   double xval1, yval1;
   double xval0d, yval0d, xval1d, yval1d;
   GawLabels *lbx = wp->ud->xLabels;
   GawLabels *lby = wp->yLabels;
   cairo_t *cr = wp->cr;
   
   xval1 = wavevar_ivar_get(vw->var, 0);
   yval1 = wavevar_val_get(vw->var, 0);

   for (i = 1; i < wavevar_nrows_get(vw->var); i++) {
      xval0d = xval1;
      yval0d = yval1;
      xval1d = xval1 = wavevar_ivar_get(vw->var, i);
      yval1d = yval1 = wavevar_val_get(vw->var, i);
//      fprintf(stderr, "i %d, xval0d = %f, yval0d = %f, xval1d = %f, yval1d = %f\n",
//              i, xval0d, yval0d, xval1d, yval1d);

      if (line_clip(&xval0d, &yval0d, &xval1d, &yval1d, 
		    lbx->start_val, lby->start_val,
		    lbx->end_val, lby->end_val) ) {
	 x0 = al_label_val2x(lbx, xval0d);
	 y0 = al_label_val2y( lby, yval0d);
	 x1 = al_label_val2x(lbx, xval1d);
	 y1 = al_label_val2y( lby, yval1d);
	 if (x0 != x1 || y0 != y1){
            cairo_move_to (cr, x0, y0);
            cairo_line_to (cr, x1, y1);
         }
      }
   }
}
