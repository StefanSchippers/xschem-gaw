#ifndef GAWWAVE_H
#define GAWWAVE_H

/*
 * gawwave.h - VisibleWave -- a waveform shown in a panel.
 * 
 * include LICENSE
 */

#include <appclass.h>

typedef struct _VisibleWave VisibleWave;


struct _VisibleWave {
   AppClass parent;
   WaveVar *var;
   DataFile *wdata;
   WavePanel *wp;
   int colorn;
   int logAble;           /* the weve can be viewved in logarithm form */
   GdkRGBA *color;        /* color for the wave */
   GtkWidget *button;
   GtkWidget *label;
   MeasureBtn *mbtn[3];
   GtkWidget *buttonpopup;   /* button popup menu */
};

/*
 * prototypes
 */
VisibleWave *wave_new( WaveVar *var, DataFile *wdata );
void wave_construct( VisibleWave *vw, WaveVar *var, DataFile *wdata );
void wave_destroy(void *vw);

void wave_color_set (VisibleWave *vw);
void wave_attach(VisibleWave *vw, WavePanel *wp, GdkRGBA *color);
void wave_detach(VisibleWave *vw);
void wave_vw_buttons_create(VisibleWave *vw );
void wave_vw_buttons_delete(VisibleWave *vw);

#endif /* GAWWAVE_H */
