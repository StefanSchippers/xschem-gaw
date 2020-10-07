/*
 * fileformat.c - fileformat interface functions
 * 
 * include LICENSE
 */
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include <spicestream.h>
#include <fileformat.h>
 
#ifdef TRACE_MEM
#include <tracemem.h>
#endif
        

#define SS_NOT_USE_REGEX 0
#define SS_USE_REGEX 1

extern int sf_rdhdr_hsascii( SpiceStream *ss );
extern int sf_rdhdr_hsbin( SpiceStream *ss );
extern int sf_rdhdr_cazm( SpiceStream *ss );
extern int sf_rdhdr_s3raw( SpiceStream *ss );
extern int sf_rdhdr_s2raw( SpiceStream *ss );
extern int sf_rdhdr_ascii( SpiceStream *ss );
extern int sf_rdhdr_nsout( SpiceStream *ss );
extern int sf_rdhdr_wav( SpiceStream *ss );

extern int sf_readrow_ascii( SpiceStream *ss );
extern int sf_readrow_hspice( SpiceStream *ss );
extern int sf_readrow_nsout( SpiceStream *ss );
extern int sf_readrow_s3raw( SpiceStream *ss );
extern int sf_readrow_s2raw( SpiceStream *ss );
extern int sf_readrow_wav( SpiceStream *ss );

extern int sf_write_file_ascii( FILE *fd, WaveTable *wt, char *fmt );
extern int sf_write_file_nsout( FILE *fd, WaveTable *wt, char *fmt );
extern int sf_write_file_hsascii( FILE *fd, WaveTable *wt, char *fmt );
extern int sf_write_file_s3ascii( FILE *fd, WaveTable *wt, char *fmt );
extern int sf_write_file_s2raw( FILE *fd, WaveTable *wt, char *fmt );
extern int sf_write_file_wav( FILE *fd, WaveTable *wt, char *fmt );

char *hspivepat[] = {
  "tr[0-9]",
  "sw[0-9]",
  "ac[0-9]",
   NULL
};

char *cazmpat[] = {
  "BNW]",
   NULL
};

char *spice3pat[] = {
  "raw",
   NULL
};

char *spice2pat[] = {
  "rawspice",
   NULL
};

char *nsoutpat[] = {
  "out",
   NULL
};

char *wavpat[] = {
  "wav",
   NULL
};

char *asciipat[] = {
   "asc",
   "acs",
   "ascii",
   "dat",
   NULL
};

static FileFormat format_tab[] = {
   { "hsascii",  "\\.(tr|sw|ac)[0-9]$", NULL, 
         sf_rdhdr_hsascii, sf_readrow_hspice, sf_write_file_hsascii,
         hspivepat },
   { "hsbinary",  "\\.(tr|sw|ac)[0-9]$", NULL,
         sf_rdhdr_hsbin, sf_readrow_hspice, NULL,
         hspivepat },
   { "cazm",  "\\.[BNW]$",  NULL,
         sf_rdhdr_cazm, sf_readrow_ascii, NULL,
         cazmpat },
   { "spice3raw", "\\.raw$", NULL,
         sf_rdhdr_s3raw, sf_readrow_s3raw, sf_write_file_s3ascii,
         spice3pat },
   { "spice2raw",  "\\.rawspice$", NULL,
         sf_rdhdr_s2raw , sf_readrow_s2raw, sf_write_file_s2raw,
         spice2pat },
   { "nsout",  "\\.out$" , NULL,
         sf_rdhdr_nsout, sf_readrow_nsout, sf_write_file_nsout,
         nsoutpat },
   { "wav",  "\\.wav$" , NULL,
         sf_rdhdr_wav, sf_readrow_wav, sf_write_file_wav,
         wavpat },
   { "ascii",  "\\.(asc|acs|ascii|dat)$", NULL,
         sf_rdhdr_ascii, sf_readrow_ascii, sf_write_file_ascii,
         asciipat },
};
static const int NFormats = sizeof(format_tab) / sizeof(format_tab[0]);

void fileformat_cleanup(void)
{   
   int i;
   
   for (i = 0; i < NFormats; i++) {
      if ( format_tab[i].creg) {
         g_free(format_tab[i].creg);
      }
   }
}

static regex_t *regexp_compile(char *str)
{
   int err;
   char ebuf[128];
   regex_t *creg;

   creg = g_new(regex_t, 1);
   err = regcomp(creg, str, REG_NOSUB | REG_EXTENDED);
   if (err) {
      regerror(err, creg, ebuf, sizeof(ebuf));
      msg_fatal(_("internal error (in regexp %s):\n  %s"), str, ebuf );
   }
   return creg;
}



char *fileformat_get_next_name( int *i, int option )
{
   int idx = ++(*i);
   
   if ( idx >= 0 && idx < NFormats ){
      if ( (option == FILE_WRITE_OP && format_tab[*i].wrFilefunc) ||
	   (option == FILE_READ_OP && format_tab[*i].rdRowfunc) ||
	   option == 0 ){
	 return format_tab[*i].name;
      }
   } else {
      *i = -1;
   }
   if ( idx == NFormats && option == FILE_READ_OP ){
      return "auto";
   }
   return NULL;
}

int fileformat_get_index(char *format )
{
   int i;
   
   for ( i = 0 ; i < NFormats; i++) {
      if ( app_strcmp(format, format_tab[i].name) == 0) {
         return i;
      }
   }
   return -1;
}

ReadRowFP fileformat_get_read_func(int index)
{
   if ( index < 0 ) {
      return NULL;
   }
   return format_tab[index].rdRowfunc;
}

ReadRowFP fileformat_get_readhdr_func(int index)
{
   if ( index < 0 ) {
      return NULL;
   }
   return format_tab[index].rdHeaderfunc;
}

WriteFileFP fileformat_get_write_func(int index)
{
   if ( index < 0 ) {
      return NULL;
   }
   return format_tab[index].wrFilefunc;
}

char **fileformat_get_patterns(int index)
{
   if ( index < 0 ) {
      return NULL;
   }
   return format_tab[index].globlist;
}

int fileformat_try_format(  SpiceStream *ss, int index )
{
   int ret = (format_tab[index].rdHeaderfunc)(ss);

   if ( ret <= 0) {
      fdbuf_rewind(ss->linebuf);
   }
   return ret;
}

int fileformat_find_format( SpiceStream *ss, char *filename, int use_regex)
{   
   int i;
   int ret;
   
   for (i = 0; i < NFormats; i++) {
      if ( ! format_tab[i].creg) {
         format_tab[i].creg = regexp_compile(format_tab[i].fnrexp);
      }
      if (use_regex == 0 || regexec(format_tab[i].creg, filename, 0, NULL, 0) == 0) {
         ret = fileformat_try_format( ss, i);
         if (ret == 1) {
            msg_dbg("%s: read with format \"%s\"", filename, format_tab[i].name);
            return i;
         }
      }
   }
   return -1;
}

/*
 * return >= 0 : index of format if successfull read of the header
 *   or -1 if error
 */
int fileformat_read_header( SpiceStream *ss)
{
   int i;
   char *format = ss->format;

   if ( format && *format == 0 ) {
      format = NULL;
   }
   
   if ( format && app_strcmp(format, "auto" ) ){
      if ( app_strcmp(format, "hspice") == 0 ) {
	 i = fileformat_get_index("hsaccii");
	 if ( i >= 0 && fileformat_try_format( ss, i ) == 1 ){
	    return i;
	 }
	 i = fileformat_get_index("hsbinary");
	 if ( i >= 0 && fileformat_try_format( ss, i ) == 1 ){
	    return i;
	 }
      } else {
	 i = fileformat_get_index(format);
	 if ( i >= 0 && fileformat_try_format( ss, i ) == 1 ){
	    return i;
	 } else {
	    msg_error(_("%s:\n is not a  \"%s\" Format."),
		       ss->filename, format);
	 }
      }
      i = -1;
   } else {
      /* try to guess format from filename */
      i =  fileformat_find_format( ss, ss->filename, SS_USE_REGEX );
      if ( i < 0) {
         msg_dbg("%s: couldn't guess a format from filename suffix.", ss->filename);
         i =  fileformat_find_format( ss, ss->filename, SS_NOT_USE_REGEX );
	 if ( i < 0 ) {
	    msg_error(_("%s: couldn't read with any format"), ss->filename);
	 }
      }
   }
   return i;
}
   
int fileformat_file_write( WaveTable *wt, char *filename, char *format, char *fmt)
{
   int i;
   int ret;
   
   if ( format == NULL ) { 
      format = "ascii";
   }
   if ((i = fileformat_get_index(format)) < 0 ) {
      msg_error( _("Can't find format '%s'"), format );
      return -1;
   }
   WriteFileFP write_func = fileformat_get_write_func(i);
   
   if ( ! write_func ) {
      msg_error(_("Write function not yet implemented for format '%s'"), format );
      return -1;
   }
   FILE *fd = fopen(filename, "w");

   if ( fd == NULL ) {
      msg_error(_("Can't open file '%s': %s"), filename,  strerror (errno));
      return -1;
   }

   ret = write_func ( fd, wt, fmt);
      
   fclose(fd);
   return ret;
}
