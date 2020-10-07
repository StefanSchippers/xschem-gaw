#ifndef GAW_H
#define GAW_H

#define MSG_DEBUG 1

#include <userprefs.h>
#include <wavetable.h>
#include <spicestream.h>
#include <sndstream.h>
#include <fileformat.h>
#include <datafile.h>


#define WIN_BORDER_SIZE 3      /* globalTable border width */
#define AW_MSG_OK 0
#define AW_MSG_FATAL 1

#define AW_NX_MBTN 3
#define AW_NY_MBTN 3

#define AW_LMSWTABLE_COLS 4  /* number of columns in lmtable */
#define AW_PANELTABLE_COLS 2  /* number of columns in panel table */

// #define PACKAGE "gaw"

#define AW_MSG_T_SHOW_CANCEL (1 << 16)
#define AW_MSG_T_MASK (AW_MSG_T_SHOW_CANCEL - 1)

typedef struct _UserData UserData;

#include <gawsnd.h>
#include <gawmisc.h>
#include <gawlabel.h>
#include <gawpanel.h>
#include <gawcursor.h>
#include <gawwave.h>
#include <gawdnd.h>
#include <gawda.h>
#include <gawgrip.h>
#include <gawtext.h>
#include <gawdraw.h>
#include <gawmenus.h>
#include <sockcon.h> /* stefan, used in GawIoData */

/*
 * structure for submenu list
 */
typedef struct _AwSubmenuList AwSubmenuList;

struct _AwSubmenuList {
   char *desc;
   char *icon_str;
};


typedef struct _GawIoData GawIoData; /* stefan, moved here from gawio.c */
struct _GawIoData {
   int fd;
   UserData *ud;
   GIOChannel *iochannel;
   GIOChannel *listenchannel;
   SockCon *listen;
   SockCon *cnx;
   guint listenid; /* id of the watch */
   guint sourceid; /* id of the watch */

   char *curtbl;
   int state;
   int curcol;
   int currow;
   DataFile *wdata;
   WDataSet *wds;
   char *msg;
};


/*
 * structure for submenu action
 */

typedef struct _AwSubmenuAction AwSubmenuAction;

typedef void (*SubmenuAction_FP)( AwSubmenuAction *sa );

struct _AwSubmenuAction {
   UserData *ud;
   AwSubmenuList *tbl;      /* table of submenu items */
   int *up_index;           /* address of the variable holding the index */
   SubmenuAction_FP func;   /* function to realize the choice */
};



/*
 * Global structures
 */

struct _UserData {
   gchar *prog;
   UserPrefs *up;         /* pointer to preference structure */
   gchar *mainName;
   int restart;
   GSimpleActionGroup *group;  /* the main action group */
   GtkWidget *window;       /* top level window */
   GtkWidget *globalTable;  /* global table to contains the other widgets */
   GtkWidget *dialog_window;
   GtkWidget *meas_hbox;   /* hbox containing x measure buttons */
   int meas_hbox_shown;  /* 1 hbox containing x measure buttons is displayed */
   GtkWidget *menuBar;   /* menu bar */
   GMenuModel *algomodel;      /* menu model for algo selection */
   AwSubmenuAction *algodata;  /* data for algo action */
   GMenuModel *xconvmodel;   /* menu model for x conversion method */
   AwSubmenuAction *xconvdata;  /* data for xconv action */
   GMenuModel *vlmmodel;     /* menu model for variable list  */
   GtkWidget *toolBar; /* tool bar */
   GtkWidget *panel_scrolled; /*  scrolled window for panel table */
   int sbSize;                /* allocated size for the scrollbar */
   GtkWidget *allline_box; /*   hbox for logXbox, xlabels */
   GtkActionGroup *actions;   /* action group for mainmenu     */
   GtkActionGroup *vlactions; /* action group for variable list */
   GtkWidget *statusbar;   /* bottom status bar */
   GtkWidget *statusLabel; /* label in bottom status bar */
   GList *xlabel_list;        /* xlabel list */
   GList *destdata_list;      /* DnDDestData * list */
   GtkWidget *xlabel_table;   /* gtktable for xlabels */
   GtkWidget *xlabel_ev_box;
   GtkWidget *xlabel_box;
   GtkWidget *logx_box;
   GtkWidget *xscrollbar;
   GtkAdjustment *xadj;
   GtkWidget *win_xlabel_log;

   GdkRGBA  *bg_color;     /* color for background */
   GdkRGBA  *pg_color;     /* color for panel grid graticule */
   GdkRGBA  *hl_color;     /* color for panel grid graticule */
   GdkRGBA  *wbut_color;   /* color for wave button bg */
   GdkRGBA  *lbbut_fgcolor;  /* color for list box button fg */
   GdkRGBA  *lbbut_bgcolor;  /* color for list box button bg */

   GSList *listFiles;     /* list of filename to load */
   gchar *filename;
   gchar *format;        /* format of the data file */
   int bits;             /* sample size for writing .wav files */
   int rate;             /* sample rate for writing .wav files */
   gchar *printFmt;      /* printf format used to export write file */
   gint  reqpanels;
   
   AWCursor **cursors;  /* 3 cursor storage pointer */
   
   GtkWidget *panelTable;     /* gtk table for panels */
   GList *panelList;          /* list of panels  */
   WavePanel *selected_panel; /* selected panel */
   WDataSet *curwds;       /* the last dataset used; for x processing */

   GawLabels *xLabels;     /* structure to hold data about the axis */
   int char_width;         /* char width  in pixel for the default font */
   int char_height;        /* char height in pixel for the default font */
   
   MouseState mouseState;
   gint drag_button;      /* which button was dragged   */
   int button_down;
   SelRange *srange;       /* structure to store selected range */
   
   int suppress_redraw;
   int NWColors ;        /* # of wavecolorN styles expected in the .gtkrc */
   
   GList *all_measure_buttons; /* measure buttons list */
   GList *wdata_list;     /* dataset list */
   
   GSList *imgFormats;   /* list of writtable img formats */
   const gchar *imgFmt;  /* selected img format */
   
   GawSndData *sndData;   /* sound data structure */
   
   int listenPort;
   void *gawio;           /* pointer to gawio struct */
   int gripdelta;         /* delta on displacement of the grip */
   int winWidth;          /* main current window actual width */
   int winHeight;         /* main current window height */
   int reqWinWidth;       /* requested main current window actual width */
   int reqWinHeight;      /* requested main current window height */
   int maxHeight;         /* maximum height for panel_scrolled */
   int waHeight;          /* work area height */
   int panelWidth;        /* current panel width */
   int panelHeight;       /* current panel height */
   int panelScrolledHeight;  /* current height of panel scrolled widget */
   GawText *gtexttmp;       /* temp pointer to a Gawtext */
   const gchar *panelfont;  /* panel fontname */
   /* sensitive widget */
   GtkWidget *LogX;   
   GtkWidget *LogYPanel;   
   GtkWidget *moreYlabels;   
};

/*
 * Global variables
 */

extern char *aw_panel_not_selected_msg;
extern UserData *userData; 
extern AwSubmenuList wave_algo_desc_tab[];

/*
 * Global functions
 */

int aw_dialog_show ( int type, const char *text);
void aw_vl_menu_item_add( DataFile *wdata);
void aw_redraw_all_gaction (GSimpleAction *action, GVariant *param, gpointer user_data );
void aw_show_grid_cmd (UserData *ud, int on );
void aw_vl_menu_item_remove( DataFile *wdata);
void aw_panel_scrolled_set_size_request( UserData *ud);
void aw_window_size(UserData *ud);
void aw_main_window_destroy ( UserData *ud );
void aw_do_exit (GtkWidget *widget,  UserData *ud );
void aw_do_save_config ( UserData *ud );
void aw_algo_menu_create( UserData *ud );
void aw_xconv_menu_create( UserData *ud );

void im_export_panels_img_gaction (GSimpleAction *action, GVariant *param, gpointer user_data );
void im_pop_export_to_img_gaction (GSimpleAction *action, GVariant *param, gpointer user_data );
void im_export_panels_img_cmd (char *file, char *format, UserData *ud );

gint az_cmd_zoom_absolute(UserData *ud, double start, double end );
void az_zoom_in_gaction (GSimpleAction *action, GVariant *param, gpointer user_data );
void az_pop_zoom_in_gaction (GSimpleAction *action, GVariant *param, gpointer user_data );
void az_zoom_out_gaction (GSimpleAction *action, GVariant *param, gpointer user_data );
void az_pop_zoom_out_gaction (GSimpleAction *action, GVariant *param, gpointer user_data );
void az_zoom_cursor0_centered_gaction (GSimpleAction *action, GVariant *param,  gpointer user_data );

void az_zoom_cursors_gaction (GSimpleAction *action, GVariant *param,  gpointer user_data );
void az_pop_zoom_cursors_gaction (GSimpleAction *action, GVariant *param, gpointer user_data );

void az_zoom_x_full_gaction (GSimpleAction *action, GVariant *param, gpointer user_data );
void az_pop_zoom_x_full_gaction (GSimpleAction *action, GVariant *param, gpointer user_data );
void az_zoom_y_full_gaction (GSimpleAction *action, GVariant *param, gpointer user_data );
void az_pop_zoom_y_full_gaction (GSimpleAction *action, GVariant *param, gpointer user_data);
   
void az_pop_zoom_y_full_gaction (GSimpleAction *action, GVariant *param, gpointer user_data );
void az_zoom_x_gaction (GSimpleAction *action, GVariant *param,  gpointer user_data);
void az_pop_zoom_x_gaction (GSimpleAction *action, GVariant *param, gpointer user_data );
void az_zoom_y_gaction (GSimpleAction *action, GVariant *param,  gpointer user_data);
void az_pop_zoom_y_gaction (GSimpleAction *action, GVariant *param, gpointer user_data );
void az_zoom_xy_area_gaction (GSimpleAction *action, GVariant *param,  gpointer user_data);
void az_pop_zoom_xy_area_gaction (GSimpleAction *action, GVariant *param, gpointer user_data );

void az_zoom_dialog_gaction (GSimpleAction *action, GVariant *param,  gpointer user_data);
void az_pop_zoom_dialog_gaction (GSimpleAction *action, GVariant *param, gpointer user_data);

WavePanel *ap_panel_add_line( UserData *ud, WavePanel *wp, int relpos);
void ap_panel_remove_line(UserData *ud, WavePanel *ppos);
void ap_panel_update_table(UserData *ud );

void ap_redraw_x(UserData *ud);
void ap_all_redraw(UserData *ud);
void ap_all_panel_redraw(UserData *ud);
void ap_container_empty(GtkWidget *widget, int del);
void ap_xmeasure_button_draw(UserData *ud);
void ap_xmeasure_button_show(UserData *ud);
void ap_set_user_panel_size(UserData *ud);

void ap_xlabel_box_show(UserData *ud);
void ap_reload_wave_file (DataFile *wdata);
void ap_panel_add_var(WavePanel *wp, WaveVar *dv, VisibleWave *vw, GdkRGBA *usercolor); /* stefan added usercolor */
void ap_remove_all_wave_selected( UserData *ud);
void ap_widget_show(GtkWidget *w,  gboolean show);
void ap_mbtn_update_all(UserData *ud);
void ap_delete_datafile(DataFile *wdata);
WaveTable *ap_wavetable_new_from_displayed(UserData *ud );
void ap_draw_xlabels(UserData *ud);
void ap_remove_all_wave_if_panel_and_var(WavePanel *wp, WaveVar *var);
DataFile *ap_load_wave (DataFile *wdata);
void ap_mbtn_update(MeasureBtn *mbtn, UserData *ud );
GtkWidget *ap_create_measure_label(GtkWidget *box, gchar *blabel,  gchar *lrc );
GtkWidget *ap_create_measure_button(GtkWidget *box, gchar *brc, gchar *tips );
GtkWidget *ap_create_toggle_button(GtkWidget *box, gchar *brc, gchar *tips);
void ap_create_xmeasure_unit(UserData *ud);
void ap_xlabel_box_create(UserData *ud);
void ap_create_win_bottom(UserData *ud);
void ap_remove_all_wave_if_var_name( UserData *ud, WaveVar *var);
void ap_set_xvals(UserData *ud);

void gaw_show_about_dialog (GtkWidget *parent);

void aio_create_channel(UserData *ud);
void aio_destroy_channel(UserData *ud);

void ac_bp_color_wave_gaction (GSimpleAction *action, GVariant *param, gpointer user_data );
void ac_pop_color_grid_gaction (GSimpleAction *action, GVariant *param, gpointer user_data );
void ac_color_widget_style_color_set( GtkWidget *widget, gchar *fg_color, gchar *bg_color );
int ac_color_grid_cmd (UserData *ud, char *colorspec );

void ac_color_initialize (WavePanel *wp);
void ac_color_find_style_color( GtkWidget *widget, gchar *name, GdkRGBA *rgba);
int ac_color_background_cmd (UserData *ud, char *colorspec );
void ac_color_panel_colors_gaction (GSimpleAction *action, GVariant *param, gpointer user_data );
int ac_color_rgba_str_set(char **valp, GdkRGBA *rgba );
void ac_color_rgba_init(GdkRGBA *destColor, GdkRGBA *styleColor, char *rcColor );

void af_open_file_gaction (GSimpleAction *action, GVariant *param,  gpointer user_data);
void af_export_displayed_gaction (GSimpleAction *action, GVariant *param, gpointer user_data );
void af_vl_export_data_gaction (GSimpleAction *action, GVariant *param, gpointer user_data );
int af_export_displayed_data_cmd (char *filename, char *format, UserData *ud );
DataFile *af_load_wave_file (UserData *ud);
void af_list_files_free(GSList **list);
void af_unload_all_files (UserData *ud);

void  ah_show_userguide_dialog (UserData *ud);
void  ah_show_website_dialog (UserData *ud);
void aw_reload_all_files_gaction (GSimpleAction *action, GVariant *param, gpointer user_data ); /* stefan */

#endif /* GAW_H */
