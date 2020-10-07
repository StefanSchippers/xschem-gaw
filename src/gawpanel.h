#ifndef GAWPANEL_H
#define GAWPANEL_H

/*
 * gawpanel.h - panel interface
 * WavePanel -- describes a single panel containing zero or more waveforms.
 * 
 * include LICENSE
 */

#include <appclass.h>

typedef struct _WavePanel WavePanel;
//typedef void (*WaveDraw_FP) (AppClass *vw, WavePanel *wp);
typedef void (*WaveDraw_FP) ();

struct _WavePanel {
   AppClass parent;
   UserData *ud;           /* pointer to application data */
   GtkWidget *lmtopbox;    /* the box at top of lmswtable */
   GtkWidget *drawing;     /* DrawingArea for waveforms */
   GSimpleActionGroup *wpgroup;  /* the panel action group */
   gboolean selected;
   gboolean configure_seen;   /* set to 1 in configure_cb */
   
   GList *vwlist;  /* list of VisibleWaves shown in this panel. */

   GList *textlist;  /* list of GawText shown in this panel. */

   GtkWidget *top_ylabel_log;   /* label logY in lmtopbox       */
   GtkWidget *ylabel_log;       /* label logY             */
   GtkWidget *logy_box;         /* box for label logY ylabel_max */
   GtkWidget *ylabel_max;       /* ymax label */
   GtkWidget *ylabel_min;       /* ymin label */
   
   GtkWidget *lmscroll_win; /* scrolled window for lmtable */
   GtkWidget *lmswtable;     /* left most scrolled window table */
   GtkWidget *lmtable;      /* left measure global table */
   GtkWidget *popmenu;      /*  popup menu in panel */
   GtkWidget *textpopmenu;    /*  popup menu in panel for text edit */
   
   GawLabels *yLabels;  /* structure to hold data about the axis */
   double min_xval; /* min/max data x/y values over whole vwlist */        
   double max_xval;
   
   int man_yzoom;  /* toggle button state in zoom dialog */
   int nextcolor;  /* color to use for next added waveform */
   int showGrid;  /* show grid in panel */
   
   GdkRGBA *grid_color;      /* color for panel grid graticule */
   cairo_t *cr;              /* cairo context for drawing */
   WaveDraw_FP drawFunc;     /* function to draw the waveform */
   /* sensitive widget */
   GtkWidget *pPLogY;   
};

/*
 * prototypes
 */
WavePanel *pa_panel_new(  UserData *ud );
void pa_panel_construct( WavePanel *wp,  UserData *ud );
void pa_panel_destroy(void *wp);

void pa_panel_color_grid_set (WavePanel *wp, gpointer data);
int pa_panel_next_color(WavePanel *wp);
void pa_panel_vw_list_add(WavePanel *wp, gpointer vw);
void pa_panel_vw_list_remove(WavePanel *wp, gpointer vw);
void pa_panel_text_list_add(WavePanel *wp, gpointer gtext);
void pa_panel_text_list_remove(WavePanel *wp, gpointer gtext);

void pa_panel_set_yvals(WavePanel *wp, double start, double end);
void pa_panel_full_redraw(WavePanel *wp);
gint pa_panel_find_pos( WavePanel *wp);
WavePanel *pa_panel_find_selected(UserData *ud);
void pa_panel_set_selected(WavePanel *wp, UserData *ud );
void pa_panel_update_min_max(WavePanel *wp);;
void pa_panel_lmswtable_setup(WavePanel *wp );
void pa_panel_vw_delete(WavePanel *wp);
void pa_panel_drawing_set_gdk_cursor(WavePanel *wp, int cursorType );
void pa_panel_drawing_redraw(WavePanel *wp, gpointer data );
void pa_panel_background_set(WavePanel *wp, gpointer data );
void pa_panel_lmscroll_win_set_size_request(WavePanel *wp);
void pa_ylabel_box_show( WavePanel *wp );
void pa_panel_label_meas_box_update(WavePanel *wp);
void pa_panel_label_size(WavePanel *wp);
void pa_panel_draw_ylabels(WavePanel *wp);
GtkWidget *pa_panel_label_meas_box_create(WavePanel *wp, UserData *ud);
void pa_panel_update_all_data(UserData *ud);
AppClass *pa_panel_inside_text(WavePanel *wp, int x, int y);
void pa_panel_delete_all_texts( WavePanel *wp );

void pa_panel_set_size_request(WavePanel *wp);
void pa_panel_set_size_request_min(WavePanel *wp);
void pa_panel_set_drawing_func(WavePanel *wp, gpointer data );

#endif /* GAWPANEL_H */
