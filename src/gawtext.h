#ifndef GAWTEXT_H
#define GAWTEXT_H

/*
 * gawtext.h - gawtext protocol interface
 * 
 * include LICENSE
 */

#include <appclass.h>

typedef struct _GawText GawText;

struct _GawText {
   AppClass parent;
   GtkWidget **win;       /* pointer to top window */
   GtkWidget *entry;      /* text entry widget */
   GtkWidget *da;         /* deawing area */
   gchar *fontname;       /* font name */
   void *user_data;       /* pointer to user data for callbacks */
   PangoFontDescription *font_desc; /* pango font description */
   GdkRGBA *bg_color;     /* color for text background */
   GdkRGBA *fg_color;     /* color for text foreground */
   gint x;                 /* x position of the text */
   gint y;                 /* y position of the text */
   gint cx;                /* drag cursor x position */
   gint cy;                /* drag cursor y position */
   gint width;             /* width of the text */
   gint height;            /* height of the text */
   gint maxwidth;          /* width of drawingarea */
   gint maxheight;         /* height of drawingarea */
   double bbx1;            /* x1 bounding box around text */
   double bby1;            /* y1 bounding box around text */
   double bbx2;            /* x2 bounding box around text */
   double bby2;            /* y2 bounding box around text */
   gchar *text;            /* the text to be displayed */
   double angle;           /* angle (in radians) */
   int edit;               /* indicate called for edit */
};

/*
 * prototypes
 */
GawText *gawtext_new( UserData *ud, gchar *fontname, gchar *bg_color,
                      gchar *fg_color, gchar *angle);
void gawtext_construct( GawText *gtext, UserData *ud, gchar *fontname,
                      gchar *bg_color, gchar *fg_color, gchar *angle);
void gawtext_destroy(void *gtext);

GawText *gawtext_dup( GawText *gtext );
void gawtext_draw_text( GawText *gtext, WavePanel *wp);
void gawtext_dialog (GawText *gtext,  UserData *ud);
void gawtext_update_pos( GawText *gtext, int x, int y);
gboolean gawtext_inside_text( AppClass *gtext, int x, int y);
void gawtext_new_gaction (GSimpleAction *action, GVariant *param, gpointer user_data);
void gawtext_pop_edit_gaction (GSimpleAction *action, GVariant *param, gpointer user_data );
void gawtext_pop_delete_gaction (GSimpleAction *action, GVariant *param, gpointer user_data );
void gawtext_pop_delete_all_text_gaction (GSimpleAction *action, GVariant *param, gpointer user_data );

#endif /* GAWTEXT_H */
