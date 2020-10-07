/*
 * ss_wav.c: routines for SpiceStream that handle the '.wav' files 
 * 
 * include LICENSE
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
// #include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <float.h>

#include <spicestream.h>
#include <ss_wav.h>
 
#ifdef TRACE_MEM
#include <tracemem.h>
#endif
        


/*
 * Read spice-type file header - ".wav" format
 *
 * return  1 ok
 *         0 EOF
 *        -1 Error
 */
int sf_rdhdr_wav( SpiceStream *ss )
{
   int i;
   char *line;
   WaveHeader *hdr;
   WaveChunkHeader *cf;
   WaveFmtBody *fmt;
   WaveChunkHeader *cd;
   int rate;
   char signam[16];
   size_t length;

   if ( fdbuf_read(ss->linebuf, sizeof(WaveHeader) ) != sizeof(WaveHeader) ) {
      return 0;
   }
   line = ((DBuf *) ss->linebuf)->s;
   hdr = (WaveHeader *) line;
   if (hdr->magic != WAV_RIFF || hdr->type != WAV_WAVE ) {
      msg_dbg("not a wav file (bad magic number)");
      return -1;
   }

   if ( fdbuf_read(ss->linebuf, sizeof(WaveChunkHeader) ) != sizeof(WaveChunkHeader) ) {
      return 0;
   }
   line = ((DBuf *) ss->linebuf)->s;
   cf = (WaveChunkHeader *) line;
   
   if (cf->type != WAV_FMT ) {
      msg_dbg("not a wav file (expecting a WAV_FMT record)");
      return -1;
   }

   if ( fdbuf_read(ss->linebuf, sizeof(WaveFmtBody) ) != sizeof(WaveFmtBody) ) {
      return 0;
   }
   line = ((DBuf *) ss->linebuf)->s;
   fmt = (WaveFmtBody *) line;
   
   ss->ndv = le16_to_cpu(fmt->channels);
   rate = le32_to_cpu(fmt->sample_fq);
   ss->bps = le16_to_cpu(fmt->bit_p_spl); /* storing bits per sample in bps */
   
   /* Data chunk header */
   if ( fdbuf_read(ss->linebuf, sizeof(WaveChunkHeader) ) != sizeof(WaveChunkHeader) ) {
      return 0;
   }
   line = ((DBuf *) ss->linebuf)->s;
   cd = (WaveChunkHeader *) line;
   if (cd->type != WAV_DATA ) {
      msg_dbg("not a wav file (expecting a WAV_DATA record)");
      return -1;
   }
   length = le32_to_cpu(cd->length);
   
   /* independant variable */
   dataset_var_add ( ss->wds, "time", TIME, 1 );
   ss->ncols = 1;

   for (i = 0; i < ss->ndv; i++) {
      sprintf( signam, _("channel_%d"), i );
      dataset_var_add( ss->wds, signam, VOLTAGE, 1 );
      ss->ncols++;
   }
   ss->expected_vals = length * 8 / ss->bps ;
   ss->nrows = ss->expected_vals / ss->ndv;
   ss->read_rows = 0;

   sprintf( signam, _("wav@%d"), rate );
   dataset_dup_setname(ss->wds, signam);
   msg_dbg("Wav \"%s\" expecting %d vals, %d rows",  signam,
	    ss->expected_vals, ss->nrows );
   msg_dbg("done with header at offset=0x%lx", (long) fdbuf_tello(ss->linebuf) );
   
   return 1;
}


/*
 * Read row of values from a spice2 rawfile
 */
int sf_readrow_wav(SpiceStream *ss )
{
   int i;
   int ret;
   double val;
   char *line;
   int bits = ss->bps ;
   int bytes = bits / 8 ;

   /* independent var */
   dataset_val_add( ss->wds, (double) ss->read_rows + 1);
	
   /* dependent vars */
   for (i = 0; i < ss->ndv; i++) {
      if ( ( ret = fdbuf_read(ss->linebuf, bytes )) != bytes ) {
	 msg_error(_("unexpected EOF at dvar %d"), i);
	 return -1;
      }
      line = ((DBuf *) ss->linebuf)->s;
      switch (bits) {
       case 8: {
	  u_char *uval = (u_char *) line;
	  val = (double) *uval;
	  break;
       }
       case 16: {
	  short *sval = (short *) line;
	  val = (double) le16_to_cpu(*sval);
	  break;
       }
       case 32: {
	  int *ival = (int *) line;
	  val = (double) le32_to_cpu(*ival);
	  break;
       }
       default:
	 val = 0;
      }
      dataset_val_add( ss->wds, val );
   }
   ss->read_rows++;
   if ( ss->read_rows >= ss->nrows ) {
      return -2; 
   }

   return 1;
}


/* write a WAVE-header */
static int sf_write_wav_header(FILE *fd, WDataSet *wds, int bits, int fmt_float, int rate )
{
   WaveHeader hdr;
   WaveFmtBody fmt;
   WaveChunkHeader chunkf;
   WaveChunkHeader chunkd;
   u_int tmp;
   u_short tmp2;

   hdr.magic = WAV_RIFF;
   /* len start after length field of WaveHeader */
   tmp =  sizeof(WaveHeader) + sizeof(WaveChunkHeader) + sizeof(WaveFmtBody) 
      + sizeof(WaveChunkHeader) - 8;
   hdr.length = cpu_to_le16(tmp);
   hdr.type = WAV_WAVE;

   chunkf.type = WAV_FMT;
   chunkf.length = cpu_to_le32(sizeof(WaveFmtBody));

   if ( fmt_float ) {
      fmt.format = cpu_to_le16(WAV_FMT_IEEE_FLOAT);
   } else {
      fmt.format = cpu_to_le16(WAV_FMT_PCM);
   }
   fmt.channels =  cpu_to_le16(wds->ncols - 1);
   fmt.sample_fq = cpu_to_le32(rate);

   tmp2 = (wds->ncols - 1) * bits / 8;
   fmt.byte_p_spl = cpu_to_le16(tmp2);
   tmp = (u_int) tmp2 * rate;

   fmt.byte_p_sec = cpu_to_le32(tmp);
   fmt.bit_p_spl = cpu_to_le16(bits);

   chunkd.type = WAV_DATA;
   chunkd.length = 0;

   if (fwrite( &hdr, sizeof(WaveHeader), 1, fd) != 1 ||
       fwrite( &chunkf, sizeof(WaveChunkHeader), 1,  fd) != 1 ||
       fwrite( &fmt, sizeof(WaveFmtBody), 1, fd) != 1 ||
       fwrite( &chunkd, sizeof(WaveChunkHeader), 1, fd) != 1 ) {
      msg_error(_("Wav write error"));
      return -1;
   }
   return 0;
}

static void sf_write_wav_end(FILE *fd, size_t nWritten)
{
   WaveChunkHeader cd;
   off_t length_seek;
   off_t filelen;
   u_int rifflen;
	
   filelen = nWritten + 2 * sizeof(WaveChunkHeader) + sizeof(WaveFmtBody) + 4;
   rifflen = filelen > 0x7fffffff ? cpu_to_le32(0x7fffffff) : cpu_to_le32(filelen);
   fseeko(fd, 4, SEEK_SET) ;
   fwrite( &rifflen, 4, 1, fd);

   length_seek = sizeof(WaveHeader) + sizeof(WaveChunkHeader) +
		      sizeof(WaveFmtBody);
   cd.type = WAV_DATA;
   cd.length = nWritten > 0x7fffffff ? cpu_to_le32(0x7fffffff) : cpu_to_le32(nWritten);
   fseeko(fd, length_seek, SEEK_SET);
   fwrite( &cd, sizeof(WaveChunkHeader), 1, fd);
}

void sf_write_file_wav( FILE *fd, WaveTable *wt, char *fmt)
{
   int i;
   int j;
   int k = 0;
   WDataSet *wds;
   int fmt_float;
   int bits;
   size_t nWritten;
   int rate;
   
   while ( (wds = wavetable_get_dataset( wt, k++)) ){
      fmt_float = 0;
      nWritten = 0;

      bits = wt->bits;
      rate = wt->rate;
      if ( wt->bits > 32 ) {
	 bits &= 0x2f;
	 fmt_float = 1;
      }
      if (sf_write_wav_header( fd, wds, bits, fmt_float, rate ) ) {
	 return;
      }
      
      for ( i = 0 ; i < wds->nrows ; i++ ){
	 for ( j = 0 ; j < wds->ncols ; j++ ){
	    if ( j == 0 ) {
	       continue;
	    }
	    double val = dataset_val_get( wds, i, j );
	    switch (bits) {
	     case 8: {
		u_char uval = (u_char) val;
		fwrite( &uval, sizeof(u_char), 1, fd);
		nWritten += sizeof(u_char);
		break;
	     }
	     case 16: {
		short sval = cpu_to_le16( (short) val);
		fwrite( &sval, sizeof(short), 1, fd);
		nWritten += sizeof(short);
		break;
	     }
	     case 32: {
		int ival = cpu_to_le32( (int) val);
		fwrite( &ival, sizeof(int), 1, fd);
		nWritten += sizeof(int);
		break;
	     }
	    }
	 }
	 
      }
      sf_write_wav_end ( fd, nWritten ) ;
   }
}
