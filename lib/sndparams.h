#ifndef SS_SOUND_H
#define SS_SOUND_H
/*
 * ss_sound.h: definition for SpiceStream that handle the capture from
 * the sound card : a 2 column data.
 * 
 * include LICENSE
 */


typedef struct _SoundParams SoundParams;

#ifdef HAVE_LIBASOUND

#include <alsa/asoundlib.h>


#define SS_DEFAULT_FORMAT		SND_PCM_FORMAT_U8
#define SS_DEFAULT_SPEED 		8000

enum _VuMeterInfo {
        VUMETER_NONE,
        VUMETER_MONO,
        VUMETER_STEREO
};


struct _SoundParams {
   /* requested options */
   unsigned int rate;           /* sampling rate */
   snd_pcm_format_t format;     /* stream format */
   char *indexstr;              /* string index for card */
   char *device_name;           /* ASCII identifier of the PCM handle */
   int firstcard;      /* first card number */
   unsigned int input;          /* input number on card */
   unsigned int channels;
   char *control_name;          /* mixer control name */
   /* length options */
   unsigned int numsamples;     /* number of samples per channel to capture */ 
   double duration;             /* durationt is # seconds */
   int start_delay;             /* delay for automatic PCM start is # microseconds */
   int stop_delay;              /* delay for automatic PCM stop is # microseconds from xrun */
   /* internal options */
   snd_pcm_stream_t stream;     /* Wanted stream */
   int open_mode;               /* Open mode (see SND_PCM_NONBLOCK, SND_PCM_ASYNC) */
   snd_output_t *fdlog;         /* log desc for snd */
   unsigned int buffer_time;    /* buffer duration is # microseconds */
   unsigned int period_time ;   /* distance between interrupts is # microseconds */
   snd_pcm_uframes_t buffer_frames;
   snd_pcm_uframes_t period_frames; 
   int avail_min;                /* min available space for wakeup is # microseconds */
   /* internal variables */
   snd_pcm_t *handle;
   snd_pcm_uframes_t chunk_size; /* chunk size in frames */
   size_t chunk_bytes;           /* audiobuf size in bytes  */
   size_t bits_per_sample;       /* bits_per_sample depend on format */
   size_t bits_per_frame;        /* bits_per_sample * channels */
   size_t count;                 /* requested count chunk to read */
   int vumeter;
   u_char *audiobuf;             /* the read buffer */
   u_char *bufp;                 /* pointer on the buffer */
   size_t nleft;                 /* bytes remaining in buffer  */
};

int sound_init_soundCard(SoundParams *params);
int sound_getval_sound(SoundParams *params, double *val);
int sound_close_soundCard(SoundParams *params);

snd_mixer_t *sound_open_mixer(char *device_name);
int sound_mixer_set( char *device_name, char *mixer_control, char *item);

#else  /* HAVE_LIBASOUND */
struct _SoundParams {
};
#endif /* HAVE_LIBASOUND */

#endif /* SS_SOUND_H */
