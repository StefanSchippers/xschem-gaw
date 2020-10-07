/*
 * sndstream.c - sound interface functions
 * 
 * include LICENSE
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <stdio.h>
#include <string.h>
#include <unistd.h>
// #include <stdlib.h>
#include <float.h>

#include <sndstream.h>
 
#ifdef TRACE_MEM
#include <tracemem.h>
#endif
        

#ifdef HAVE_LIBASOUND

/*
 *** \brief Allocates memory for a new SoundStream object.
 */

SoundStream *sound_new( SoundParams *params, WaveTable *wt )
{
   SoundStream *snd;

   snd =  app_new0(SoundStream, 1);
   sound_construct( snd, params, wt );
   app_class_overload_destroy( (AppClass *) snd, sound_destroy );
   return snd;
}

/** \brief Constructor for the SoundStream object. */

void sound_construct( SoundStream *snd, SoundParams *params, WaveTable *wt )
{
   app_class_construct( (AppClass *) snd );
   
   snd->sndParams = params ;
   snd->wt = wt;
   wavetable_set_funcs( wt, (AppClass *) snd, sound_rdhr, sound_read_rows,
			sound_destroy );
   snd->ncols = snd->sndParams->channels + 1;
}

/** \brief Destructor for the SoundStream object. */

void sound_destroy(void *snd)
{
//   SoundStream *this = (SoundStream *) snd;

   if (snd == NULL) {
      return;
   }

   app_class_destroy( snd );
}


/*
 * Initialize stream - sound pcm format
 * return   1 OK
 *          0 EOF (not used)
 *         -1 error
 */

int sound_rdhr( AppClass *ss )
{
   SoundStream *snd = (SoundStream *) ss;
   char signam[16];
   int i;

   if ( sound_init_soundCard(snd->sndParams) ) {
      msg_error(_("Can't initialize sound card"));
      return -1;
   }
   snd->wds = wavetable_get_cur_dataset(snd->wt); /* curent wds */
   
   dataset_var_add( snd->wds, "time", TIME, 1 );

   for ( i = 0; i < snd->ncols - 1 ; i++) {
      sprintf( signam, _("channel_%d"), i );
      dataset_var_add( snd->wds, signam, VOLTAGE, 1 );
   }

   return 1;
}

/*
 * Read row of values from sound card : n channels.
 *
 * Returns:
 *	1 on success.  also fills in *ivar scalar and *dvars vector
 *	0 on EOF
 *	-1 on error  (may change some ivar/dvar values)
 */
int sound_readrow(SoundStream *snd)
{
   int i;
   double val;

   dataset_val_add( snd->wds, (double) snd->nrows + 1);

   for (i = 0; i < snd->ncols - 1; i++) {
      if ( sound_getval_sound(snd->sndParams, &val) != 1) {
	 if (i > 0 ) {
	    msg_warning(_("EOF or error reading data field %d in row %d; file is incomplete."),
			i, snd->nrows);
	 }
         return 0;
      }
//      msg_dbg ( "sound val = %f", val); 
      dataset_val_add( snd->wds, val);
   }
   snd->nrows++;
   return 1;
}


/*
 * row is read in temp storage ss->rowData because
 * we do not known if we have multiple set in the file
 * indicated by a decreseing order of iv
 */
int sound_read_rows( AppClass *sss )
{
   SoundStream *snd = (SoundStream *) sss;
   int ret;
   int row = 0;

   while ((ret = sound_readrow(snd)) > 0) {
      row++;
   }
   msg_dbg("Read %d rows %d", row, snd->nrows );
   return ret;
}

#else  /* HAVE_LIBASOUND */

SoundStream *sound_new( SoundParams *params, WaveTable *wt )
{
   return NULL;
}
   
int sound_rdhr( AppClass *ss )
{
   return 0;
}

int sound_readrow(SoundStream *snd)
{
   return 0;
}

#endif /* HAVE_LIBASOUND */
