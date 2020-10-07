#ifndef DATAFILE_H
#define DATAFILE_H

/*
 * datafile.h - datafile protocol interface
 * 
 * include LICENSE
 */

enum _DatafileTypeInfo {
   DATAFILE_FILE = 1,
      DATAFILE_SOUND,
};

typedef struct _DataFile DataFile;

#include <appclass.h>
#include <gaw.h>


struct _DataFile {
   AppClass parent;
   void  *ud;   /* back pointer to userData */
   WaveTable *wt;          /* pointer to table object */
   WaveTable *old_wt;      /* for clean up purpose */
   gchar *filename;  
   gchar *format;          /* format of filename */
   SoundParams *sparams;   /* sound params  */
   int method;             /* file or sound */
   GtkWidget *wlist_win;   /* window with scrolling variable list */
   GtkWidget *wlist_vbox;  /* vertical container */
   GtkWidget *wlist_box;   /* scrolled box containing DnD variable items */
   int ftag;        /* short tag used to help identify which file is which */
   guint merge_id;  /* merge id in variable list menu */
   int ndv;
   GdkPixbuf *drag_icon;
   GSimpleActionGroup *group;  /* the vl menu action group */
   GtkWidget *vlmenu;         /*  vl menu  */
   GtkWidget *lbpopmenu;      /*  list button pop menu  */
};

/*
 * prototypes
 */
DataFile *datafile_new(  void *ud, char *name );
void datafile_construct( DataFile *wdata, void *ud, char *name );
void datafile_destroy(void *wdata);

void datafile_dup_filename(DataFile *wdata, char *filename);
void datafile_dup_format(DataFile *wdata, char *format);
void datafile_set_file(DataFile *wdata,  char *filename, char *format);
void datafile_set_sound(DataFile *wdata,  SoundParams *sparams);
int datafile_load(DataFile *wdata);
int datafile_reload(DataFile *wdata);

void datafile_list_win_destroy(DataFile *wdata);
void datafile_create_list_win (DataFile *wdata);
void datafile_recreate_list_win (DataFile *wdata);
void  datafile_add_list_button(gpointer d, gpointer p);
void datafile_similar_vars_add (DataFile *wdata, WaveVar *var);

#endif /* DATAFILE_H */
