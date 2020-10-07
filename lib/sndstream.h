#ifndef SNDSTREAM_H
#define SNDSTREAM_H

/*
 * sndstream.h - sound protocol interface
 * 
 * include LICENSE
 */

#include <wavetable.h>
#include <sndparams.h>

typedef struct _SoundStream SoundStream;

struct _SoundStream {
   AppClass parent;
   SoundParams *sndParams;
   WDataSet *wds;
   WaveTable *wt;
   int ncols;
   int nrows;
};

/*
 * prototypes
 */
SoundStream *sound_new( SoundParams *params, WaveTable *wt );
void sound_construct( SoundStream *snd, SoundParams *params, WaveTable *wt );
void sound_destroy(void *snd);

int sound_rdhr( AppClass *ss );
int sound_readrow(SoundStream *snd);
int sound_read_rows( AppClass *sss );

#endif /* SNDSTREAM_H */
