#ifndef GAWCURSOR_H
#define GAWCURSOR_H

typedef struct _MeasureBtn MeasureBtn;
typedef struct _AWCursor AWCursor;


/*
 * Global structures
 */

struct _MeasureBtn {
   AWCursor *csp;      /* pointer to associate cursor */
   int measurefunc;
   WaveVar *var;           // note: might be NULL
   
   GtkWidget *button;
   GtkWidget *label;
};


/*
 * Cursor - structure describing a vertical bar cursor
 */
struct _AWCursor {
   GtkWidget *button;  /* display x measure button */
   GtkWidget *label;   /* display x measure label  */
   int shown;          /* vertical bar cursor */
   int zoom;           /* set if coming from zoom */
   int x;              /* x pixel value */
   double xval;        /* data corresponding x val */
   GdkRGBA *color;     /* color for cursor in drawing area */
};

/*
 *  prototypes
 */
void cu_clear_cursors(UserData *ud);
void cu_update_xcursor(WavePanel *wp, AWCursor *csp, int x, int redraw);
void cu_display_xcursor(WavePanel *wp, gint button, int x, int redraw);

void cu_cursor_clear_cursor0_gaction (GSimpleAction *action, GVariant *param, gpointer user_data );
void cu_cursor_clear_cursor1_gaction (GSimpleAction *action, GVariant *param, gpointer user_data );
void cu_cursor_clear_cursors_gaction (GSimpleAction *action, GVariant *param, gpointer user_data );
void cu_cursor_set_cursor0_gaction (GSimpleAction *action, GVariant *param, gpointer user_data );
void cu_cursor_set_cursor1_gaction (GSimpleAction *action, GVariant *param, gpointer user_data );
void cu_cursor_set_cursors_gaction (GSimpleAction *action, GVariant *param, gpointer user_data );

#endif /* GAWCURSOR_H */
