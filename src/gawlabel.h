#ifndef GAWLABEL_H
#define GAWLABEL_H

/*
 * gawlabel.h -  labels and grid functions
 * 
 * include LICENSE
 */

#include <arraystruct.h>
#include <userprefs.h>

#define VAL2X(lb,v,w) (al_label_val2xy(lb, v, w))
#define VAL2Y(lb,v,h) (h - 3 - al_label_val2xy(lb, v, h - 6))
#define VAL2LX(lb,v,w) (al_label_val2Lxy(lb, v, w))
#define VAL2LY(lb,v,h) (h - 3 - al_label_val2Lxy(lb, v, h - 6))

#define X2VAL(lb,x,w) (al_label_xy2val(lb, x, w))
#define Y2VAL(lb,y,h) (al_label_xy2val(lb, (h - 3) - y, h - 6))
#define X2LVAL(lb,x,w) (al_label_xy2Lval(lb, x, w))
#define Y2LVAL(lb,y,h) (al_label_xy2Lval(lb, (h - 3) - y, h - 6))

#define XDIR(lb) (lb->xdir ? "X" : "Y")

enum _changedValueInfo {
   CV_INIT = ( 1 << 0),         /* 1 after  initialization */
      CV_STANDBY    = ( 1 << 1),       /* nothing to do                 */
      CV_CHANGED    = ( 1 << 2),       /* values have changed           */
      CV_SBCHANGED  = ( 1 << 3),       /* scrollbar state has changed   */
      CV_SBSHOW     = ( 1 << 4),       /* 1 if scrollbar is shown       */
};
      
typedef struct _GawLabels GawLabels;
typedef struct _LabelData LabelData;

struct _LabelData {
   GtkWidget *label;  /* the pointer to the label widget */
   double lbval;      /* value for the label */
};

/*
 * structure to hold values related to x or y labels and grid.
 */

struct _GawLabels {
   AppClass parent;
   UserPrefs *up;      /* pointer to userPrefs */
   double min_val;     /* min/max data x/y values over whole vwlist */
   double max_val;
   double min_Lval;        /* their log values */
   double max_Lval;

   double start_val;    /* displayed start and end val */      
   double end_val;
   double start_Lval;   /* their log values */        
   double end_Lval;

   int npoints;          /* number of data points per x pixel value */
   
   GtkWidget *label_layout; /* layout for the  x/y labels */
   int w;                   /* drawing w width   */
   int h;                   /* drawing h height  */
   int wh;                  /* w x label width, y label box height, for positionning */ 
   int changed;             /* w or h has need redisplay */ 
   GtkWidget *label_table;  /* table for the  x/y labels */  
   int nlabels;         /* number of required labels */
   int lb_num;          /* first value in decade       */
   double lb_start;     /* first value for label       */
   double lb_Lstart;    /* first value for grid in log mode   */
   double step;         /* step */
   int logAble;         /* if set can be displayed in log */
   int logAxis;         /* axis scaling: 0=linear 1=log base 10 */
   int xdir;            /* set to 1 for X labels */
   int lbwidth;         /* box width of a label in pixel */
   int lbheight;        /* box height of a label in pixel = character height */
   int char_width;      /* character width */
   GList *label_list;   /* list of labels for this axis. */
};


/*
 * Global functions
 */

GawLabels *al_label_new( UserPrefs *up, int logAxis, int dir );
void al_label_construct( GawLabels *lb, UserPrefs *up, int logAxis, int dir );
void al_label_destroy(void *lb);

void al_label_compute(GawLabels *lb);
void al_label_set_logAxis(GawLabels *lb, int toggled );
int al_label_get_logAxis(GawLabels *lb );
int al_label_get_logAble(GawLabels *lb );
int al_label_do_logAxis(GawLabels *lb );
void al_label_remove(GawLabels *lb);
void al_label_add_remove(GawLabels *lb, int n);
void al_label_draw(GawLabels *lb);

int al_label_draw_vlin_grid(GawLabels *lb, ArrayStruct *ary, int w, int h );
int al_label_draw_hlin_grid(GawLabels *lb, ArrayStruct *ary, int w, int h );
int al_label_draw_vlog_grid(GawLabels *lb, ArrayStruct *ary, int w, int h );
int al_label_draw_hlog_grid(GawLabels *lb, ArrayStruct *ary, int w, int h );

double al_label_val2n(GawLabels *lb, double val);
double al_label_val2Ln(GawLabels *lb, double val);

int al_label_val2xy(GawLabels *lb, double val, int d);
int al_label_val2Lxy(GawLabels *lb, double val, int d);

double al_label_xy2val(GawLabels *lb, int v, int d);
double al_label_xy2Lval(GawLabels *lb, int v, int d);

void al_label_update_vals(GawLabels *lb, double start, double end );
void al_label_update_min_max_vals(GawLabels *lb, double min, double max);

void lbd_destroy( LabelData *lbd, gpointer data);

int al_label_val2x(GawLabels *lbx, double xval);
double al_label_x2val(GawLabels *lbx, int x);

int al_label_val2y(GawLabels *lby, double yval);
double al_label_y2val(GawLabels *lby, int y);
   
#endif /* GAWLABEL_H */
