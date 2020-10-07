#ifndef GAWDND_H
#define GAWDND_H

typedef struct _DnDSrcData DnDSrcData;
typedef struct _DnDDestData DnDDestData;

/*
 * structure sent as drag-and-drop-data for selecting waveforms.
 */
struct _DnDSrcData {
   int magic;
   WaveVar *var;
   gpointer data;     /* more data */
};

enum _DnDDestActionInfo {
   DND_PANEL = 1,
   DND_DELETE_BUT,
   DND_ADD_BUT,
   DND_EVENT_BOX,
};
      
struct _DnDDestData {
   UserData *ud;
   WavePanel *wp;
   int action;      
};

/*
 * Global functions
 */
void ad_dnd_target_event (GtkWidget *widget, GdkDragContext *context,
		     gint  x, gint y,
		     GtkSelectionData   *data,
		     guint info, guint time, gpointer d);
void ad_set_drag_dest(GtkWidget *widget, UserData *ud, WavePanel *wp, int action);
void ad_dnd_setup_source(GtkWidget *button, DataFile *wdata, WaveVar *var, VisibleWave *vw);

#endif /* GAWDND_H */
