/*
 * ss_sound.c: routines for SpiceStream that handle the capture from
 * the sound card : a 2 column data.
 *
 * Adapted from aplay.c amixer.c alsa-utils
 *
 * include LICENSE
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#define _GNU_SOURCE
#include <stdio.h>
#include <unistd.h>
// #include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <assert.h>

// #include <glib.h>

#include <msglog.h>
#include <bswap.h>
#include <sndparams.h>
 
#ifdef TRACE_MEM
#include <tracemem.h>
#endif
        


#ifdef HAVE_LIBASOUND
/* global data */

/* local prototypes */


int sound_close_soundCard(SoundParams *params)
{
   if (params->handle) {
      snd_pcm_close(params->handle);
      params->handle = NULL;
      free(params->audiobuf);
      snd_output_close(params->fdlog);
      snd_config_update_free_global();
   }
   return 0;
}

static int sound_set_params(SoundParams *params)
{
   snd_pcm_hw_params_t *hwparams;
   snd_pcm_sw_params_t *swparams;
   snd_pcm_uframes_t buffer_size;
   int err;
   size_t n;
   unsigned int rate;
//   int monotonic;
   snd_pcm_uframes_t start_threshold, stop_threshold;
   snd_pcm_hw_params_alloca(&hwparams);
   snd_pcm_sw_params_alloca(&swparams);
   
   err = snd_pcm_hw_params_any(params->handle, hwparams);
   if (err < 0) {
      msg_error(_("Broken configuration for this PCM: no configurations available"));
      return 1;
   }
   err = snd_pcm_hw_params_set_access(params->handle, hwparams,
					    SND_PCM_ACCESS_RW_INTERLEAVED);
   if (err < 0) {
      msg_error(_("Access type not available"));
      return 1;
   }
   err = snd_pcm_hw_params_set_format(params->handle, hwparams, params->format);
   if (err < 0) {
      msg_error(_("Sample format non available"));
      return 1;
   }
   err = snd_pcm_hw_params_set_channels( params->handle, hwparams,  params->channels);
   if (err < 0) {
      msg_error(_("Channels count non available"));
      return 1;
   }

   rate = params->rate;
   err = snd_pcm_hw_params_set_rate_near( params->handle, hwparams, &params->rate, 0);
   assert(err >= 0);
   if ((float) rate * 1.05 < params->rate || (float) rate * 0.95 > params->rate ) {
      const char *pcmname = snd_pcm_name(params->handle);
      msg_warning(_("%s : rate is not accurate (requested = %iHz, got = %iHz)"),
		  pcmname, rate, params->rate);
   }
   rate = params->rate;
   if (params->buffer_time == 0 && params->buffer_frames == 0) {
      err = snd_pcm_hw_params_get_buffer_time_max(hwparams, &params->buffer_time, 0);
      assert(err >= 0);
      if (params->buffer_time > 500000) {
	 params->buffer_time = 500000;
      }
   }
   if (params->period_time == 0 && params->period_frames == 0) {
      if (params->buffer_time > 0) {
	 params->period_time = params->buffer_time / 4;
      } else {
	 params->period_frames = params->buffer_frames / 4;
      }
   }
   if (params->period_time > 0) {
      err = snd_pcm_hw_params_set_period_time_near(params->handle, hwparams,
						   &params->period_time, 0);
   } else {
      err = snd_pcm_hw_params_set_period_size_near(params->handle, hwparams,
						   &params->period_frames, 0);
   }
   assert(err >= 0);
   if (params->buffer_time > 0) {
      err = snd_pcm_hw_params_set_buffer_time_near(params->handle, hwparams,
						   &params->buffer_time, 0);
   } else {
      err = snd_pcm_hw_params_set_buffer_size_near(params->handle, hwparams,
						   &params->buffer_frames);
   }
   assert(err >= 0);
//   monotonic = snd_pcm_hw_params_is_monotonic(hwparams);
   err = snd_pcm_hw_params(params->handle, hwparams);
   if (err < 0) {
      msg_error(_("Unable to install hw params:"));
      snd_pcm_hw_params_dump(hwparams, params->fdlog);
      return 1;
   }
   snd_pcm_hw_params_get_period_size(hwparams, &params->chunk_size, 0);
   snd_pcm_hw_params_get_buffer_size(hwparams, &buffer_size);
   if (params->chunk_size == buffer_size) {
      msg_error(_("Can't use period equal to buffer size (%lu == %lu)"),
	    params->chunk_size, buffer_size);
      return 1;
   }
   snd_pcm_sw_params_current(params->handle, swparams);
   if (params->avail_min < 0) {
      n = params->chunk_size;
   } else {
      n = (double) params->rate * params->avail_min / 1000000;
   }
   err = snd_pcm_sw_params_set_avail_min(params->handle, swparams, n);

	/* round up to closest transfer boundary */
   n = buffer_size;
   if (params->start_delay <= 0) {
      start_threshold = n + (double) params->rate * params->start_delay / 1000000;
   } else {
      start_threshold = (double) params->rate * params->start_delay / 1000000;
   }
   if (start_threshold < 1) {
      start_threshold = 1;
   }
   if (start_threshold > n) {
      start_threshold = n;
   }
   err = snd_pcm_sw_params_set_start_threshold(params->handle, swparams, start_threshold);
   assert(err >= 0);
   if (params->stop_delay <= 0) {
      stop_threshold = buffer_size + (double) params->rate * params->stop_delay / 1000000;
   } else {
      stop_threshold = (double) params->rate * params->stop_delay / 1000000;
   }
   err = snd_pcm_sw_params_set_stop_threshold(params->handle, swparams, stop_threshold);
   assert(err >= 0);

   if (snd_pcm_sw_params(params->handle, swparams) < 0) {
      msg_error(_("unable to install sw params:"));
      snd_pcm_sw_params_dump(swparams, params->fdlog);
      snd_pcm_dump(params->handle, params->fdlog);
      return 1;
   }

   params->bits_per_sample = snd_pcm_format_physical_width(params->format);
   params->bits_per_frame = params->bits_per_sample * params->channels;
   params->chunk_bytes = params->chunk_size * params->bits_per_frame / 8;
   params->audiobuf = realloc( params->audiobuf,  params->chunk_bytes);
   if ( params->audiobuf == NULL) {
      msg_error(_("not enough memory"));
      return 1;
   }
   params->count = params->numsamples /  params->chunk_size;

   params->buffer_frames = buffer_size;	/* for position test */
   return 0;
}

int sound_init_soundCard(SoundParams *params)
{
   int err;
   snd_pcm_info_t *info;
   
   err = snd_output_stdio_attach(&params->fdlog, stderr, 0);
   assert(err >= 0);

   err = snd_pcm_open(&params->handle, params->device_name, params->stream, params->open_mode);
   if (err < 0) {
      msg_error(_("audio open error: %s"), snd_strerror(err));
      return 1;
   }

   snd_pcm_info_alloca(&info);
   if ((err = snd_pcm_info(params->handle, info)) < 0) {
      msg_error(_("info error: %s"), snd_strerror(err));
      return 1;
   }

   params->chunk_size = 1024;

   params->audiobuf = (u_char *)malloc(params->chunk_size);
   if (params->audiobuf == NULL) {
      msg_error(_("not enough memory"));
      return 1;
   }
   return sound_set_params(params);
}


/* peak handler */
static void sound_compute_max_peak(SoundParams *params, size_t count)
{
   signed int val, max, perc[2], max_peak[2];
   static	int	run = 0;
   size_t ocount = count;
   int	format_little_endian = snd_pcm_format_little_endian(params->format);
   int ichans, c;
   int verbose = 2;

   if (params->vumeter == VUMETER_STEREO) {
      ichans = 2;
   } else {
      ichans = 1;
   }

   memset(max_peak, 0, sizeof(max_peak));
   switch (params->bits_per_sample) {
    case 8: {
       signed char *valp = (signed char *) params->audiobuf;
       signed char mask = snd_pcm_format_silence(params->format);
       c = 0;
       while (count-- > 0) {
	  val = *valp++ ^ mask;
	  val = abs(val);
	  if (max_peak[c] < val)
	     max_peak[c] = val;
	  if (params->vumeter == VUMETER_STEREO)
	     c = !c;
       }
       break;
    }
    case 16: {
       signed short *valp = (signed short *)params->audiobuf;
       signed short mask = snd_pcm_format_silence_16(params->format);
       signed short sval;
       
       count /= 2;
       c = 0;
       while (count-- > 0) {
	  if (format_little_endian)
	     sval = le16_to_cpu(*valp);
	  else
	     sval = be16_to_cpu(*valp);
	  sval = abs(sval) ^ mask;
	  if (max_peak[c] < sval)
	     max_peak[c] = sval;
	  valp++;
	  if (params->vumeter == VUMETER_STEREO)
	     c = !c;
       }
       break;
    }
    case 24: {
       unsigned char *valp = params->audiobuf;
       signed int mask = snd_pcm_format_silence_32(params->format);
       
       count /= 3;
       c = 0;
       while (count-- > 0) {
	  if (format_little_endian) {
	     val = valp[0] | (valp[1]<<8) | (valp[2]<<16);
	  } else {
	     val = (valp[0]<<16) | (valp[1]<<8) | valp[2];
	  }
	  /* Correct signed bit in 32-bit value */
	  if (val & (1<<(params->bits_per_sample-1))) {
	     val |= 0xff<<24;	/* Negate upper bits too */
	  }
	  val = abs(val) ^ mask;
	  if (max_peak[c] < val)
	     max_peak[c] = val;
	  valp += 3;
	  if (params->vumeter == VUMETER_STEREO)
	     c = !c;
       }
       break;
    }
    case 32: {
       signed int *valp = (signed int *) params->audiobuf;
       signed int mask = snd_pcm_format_silence_32(params->format);
       
       count /= 4;
       c = 0;
       while (count-- > 0) {
	  if (format_little_endian)
	     val = le32_to_cpu(*valp);
	  else
	     val = be32_to_cpu(*valp);
	  val = abs(val) ^ mask;
	  if (max_peak[c] < val)
	     max_peak[c] = val;
	  valp++;
	  if (params->vumeter == VUMETER_STEREO)
	     c = !c;
       }
       break;
    }
    default:
      if (run == 0) {
	 msg_error(_("Unsupported bit size %d.\n"),
		 (int) params->bits_per_sample);
	 run = 1;
      }
      return;
   }
   max = 1 << (params->bits_per_sample-1);
   if (max <= 0) {
      max = 0x7fffffff;
   }

   for (c = 0; c < ichans; c++) {
      if (params->bits_per_sample > 16) {
	 perc[c] = max_peak[c] / (max / 100);
      } else {
	 perc[c] = max_peak[c] * 100 / max;
      }
   }

   if ( verbose <= 2) {
      static int maxperc[2];
      static time_t t=0;
      const time_t tt=time(NULL);
      if (tt > t) {
	 t = tt;
	 maxperc[0] = 0;
	 maxperc[1] = 0;
      }
      for (c = 0; c < ichans; c++) {
	 if (perc[c] > maxperc[c]) {
	    maxperc[c] = perc[c];
	 }
      }

      putchar('\r');
//      print_vu_meter(perc, maxperc);
      fflush(stdout);
   } else if(verbose == 3) {
      printf("Max peak (%li samples): 0x%08x ", (long) ocount, max_peak[0]);
      for (val = 0; val < 20; val++)
	 if (val <= perc[0] / 5) {
	    putchar('#');
	 } else {
	    putchar(' ');
	 }
      printf(" %i%%\n", perc[0]);
      fflush(stdout);
   }
}



/*
 *  read function
 *   return number of bytes in the buffer
 */

static int sound_next_chunk(SoundParams *params) 
{
   ssize_t ret;

   if ( params->count-- <= 0 ){
	 return 0;
   }
   /* capture */
   size_t frames = params->chunk_bytes * 8 / params->bits_per_frame;
   ret = snd_pcm_readi( params->handle, params->audiobuf, frames);
   if ( ret < 0 ) {
      msg_error(_("Error in snd_pcm_readi - %s"), snd_strerror(ret));
      return ret;
   } else if (ret > 0 &&  params->vumeter ) {
      sound_compute_max_peak(params, ret);
   }
   if ( ret != frames ) {
      msg_error(_("Error in pcm_read - frames read %ld requested %ld"), ret, frames);
      return -1;
   }
   return  params->chunk_bytes;
}


int sound_getval_sound(SoundParams *params, double *dval )
{
   if ( params->nleft <= 0 ){
      if ( (params->nleft = sound_next_chunk(params)) <= 0 ) {
	 sound_close_soundCard(params);
	 return 0;
      }
      params->bufp = params->audiobuf;
   }
   switch ( params->format ) {
    case SND_PCM_FORMAT_S8 : {
      char cval = (char) *params->bufp++;
      *dval = (double) cval;
      params->nleft -= 1;
      return 1;
    }
    case SND_PCM_FORMAT_U8 : {
      u_char uval = *params->bufp++;
      *dval = (double) uval;
      params->nleft -= 1;
      return 1;
    }
    case SND_PCM_FORMAT_S16_LE: {
       short *sval = (short *) params->bufp;
       *dval = (double) le16_to_cpu(*sval);
       params->bufp += 2;
       params->nleft -= 2;
       return 1;
    }
    case SND_PCM_FORMAT_S16_BE: {
       short *sval = (short *) params->bufp;
       *dval = (double) be16_to_cpu(*sval);
       params->bufp += 2;
       params->nleft -= 2;
       return 1;
    }
    case SND_PCM_FORMAT_S32_LE: {
      int *ival = (int *) params->bufp;
      *dval = (double) le32_to_cpu(*ival) ;
      params->bufp += 4;
      params->nleft -= 4;
      return 1;
    }
    case SND_PCM_FORMAT_S32_BE: {
      int *ival = (int *) params->bufp;
      *dval = (double) be32_to_cpu(*ival) ;
      params->bufp += 4;
      params->nleft -= 4;
      return 1;
    }
    default:
      return 0;
   }
   return 0;
}


/********************************/

snd_mixer_t *sound_open_mixer(char *device_name)
{
   int err;
   snd_mixer_t *handle;

   if ((err = snd_mixer_open(&handle, 0)) < 0) {
      msg_error(_("Mixer %s open error: %s"), device_name, snd_strerror(err));
      return NULL;
   }
   if ((err = snd_mixer_attach(handle, device_name)) < 0) {
      msg_error(_("Mixer attach %s error: %s"), device_name, snd_strerror(err));
      goto error;
   }
   if ((err = snd_mixer_selem_register(handle, NULL, NULL)) < 0) {
      msg_error(_("Mixer register error: %s"), snd_strerror(err));
      goto error;
   }
   err = snd_mixer_load(handle);
   if (err < 0) {
      msg_error(_("Mixer %s load error: %s"), device_name, snd_strerror(err));
      goto error;
   }
   return handle;

   error:
   snd_mixer_close(handle);
   return NULL;
}

static int sound_get_enum_item_index(snd_mixer_elem_t *elem, char **ptrp)
{
   char *ptr = *ptrp;
   int items, i, len;
   char name[40];
        
   items = snd_mixer_selem_get_enum_items(elem);
   if (items <= 0) {
      return -1;
   }
   for (i = 0; i < items; i++) {
      if (snd_mixer_selem_get_enum_item_name(elem, i, sizeof(name) - 1, name) < 0){
	 continue;
      }
      len = strlen(name);
      if (! strncmp(name, ptr, len)) {
	    return i;
      }
   }
   return -1;
}


int sound_mixer_set( char *device_name, char *mixer_control, char *item)
{
   snd_mixer_t *handle;
   snd_mixer_selem_id_t *sid;
   snd_mixer_elem_t *elem;
   snd_mixer_selem_id_alloca(&sid);
   int ret = -1;
   
   snd_mixer_selem_id_set_name(sid, mixer_control);
   
   if ( (handle = sound_open_mixer(device_name)) == NULL ){
      return -1;
   }
   elem = snd_mixer_find_selem(handle, sid);
   if ( ! elem) {
      msg_error(_("Unable to find simple control '%s', %i"),
		snd_mixer_selem_id_get_name(sid),
		snd_mixer_selem_id_get_index(sid));
      goto error;
   }
   if (snd_mixer_selem_is_enumerated(elem)) {
      int ival = sound_get_enum_item_index(elem, &item);
      if (ival < 0) {
	 msg_dbg("Unable to get item index for '%s'", item );
	 goto error;
      }
      if (snd_mixer_selem_set_enum_item(elem, 0, ival) >= 0) {
	 ret = 0;
      }
   }
   
   error:
   snd_mixer_close(handle);
   return ret;
}
	    
	    
#endif /* HAVE_LIBASOUND */
