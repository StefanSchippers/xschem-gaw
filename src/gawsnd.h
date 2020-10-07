#ifndef GAWSND_H
#define GAWSND_H
 
/*
 * gawsnd.h - Gtk analog waveform viewer
 * sound configuration and capture.
 * 
 * include LICENSE
 */

#include <appclass.h>

typedef struct _GawSndData GawSndData;

struct _GawSndData {
   AppClass parent;
   UserData *ud;
   GtkWidget *w_card;       /* combo for card */
   GtkWidget *w_rate;       /* combo for rate */
   gint n_rate;             /* number of item in rate combo */
   GtkWidget *w_format;     /* combo for format */
   gint n_format;           /* number of item in format combo */
   GtkWidget *w_duration;   /* entry for duration */
   GtkWidget *w_size;       /* entry for siz */
   SoundParams *sparams;    /* sound params for sound interface */
};

/*
 * prototypes
 */
GawSndData *as_sound_new( UserData *ud );
void as_sound_construct( GawSndData *snd, UserData *ud );
void as_sound_destroy(void *snd);

void as_sound_win_create(GawSndData *snd);

#endif /* GAWSND_H */
