/*
 * test 
 */

#include <stdio.h>
#include <string.h>
#include <libgen.h>

#include <sndstream.h>
#include <wavetable.h>
#include <spicestream.h>
#include <fileformat.h>
 
#ifdef TRACE_MEM
#include <tracemem.h>
#endif
        

void usage(char *prog)
{
   fprintf(stderr, _("%s  [options] <source file>\n"), prog);
   fprintf(stderr, _("                : test and convert spice formats\n"));
   fprintf(stderr, _(" [-rf fmt]      : specify spice format for input file\n"));
   fprintf(stderr, _(" [-wf fmt]      : specify spice format for outfile file\n"));
   fprintf(stderr, _(" [-o file]      : specify output file\n"));
   fprintf(stderr, _(" [-pf fmt]      : specify printf format in output file\n"));
   fprintf(stderr, _(" [-rl]          : list available spice input format\n"));
   fprintf(stderr, _(" [-wl]          : list available spice output format\n"));
   
   exit(1);
}

void list_formats(char *label, int option)
{
   int i = -1;
   char *name;

   fprintf(stdout, "%s: ", label);
   do {
      name = fileformat_get_next_name( &i, option);
      if ( option == FILE_READ_OP && i == -1 ){
         name = "auto";
      } else if ( ! name ){
         continue;
      }
      fprintf(stdout, " %s", name);
   } while ( i != -1 );
   fprintf(stdout, "\n");
}


int main (int   argc, char *argv[])
{
   int i ;
   char *rformat = NULL;
   char *wformat = NULL;
   char *filename = NULL;
   char *output = "myfile";
   SoundParams *sparams = NULL;
   WaveTable *wt;
   SoundStream *snd;
   SpiceStream *ss;
   char *fmt = NULL ; 
   int ret ;
   char *prog;

   prog = basename(argv[0]);
   msg_initlog(prog, MSG_F_NO_DATE, NULL, NULL );
   
   for (i = 1 ; i < argc ; i++) {
      if (*argv[i] == '-') {
         if (strcmp(argv[i], "-d") == 0) {
            prog_debug++ ;
	 } else if (strcmp(argv[i], "-h") == 0) {
	    usage(prog);
	 } else if (strcmp(argv[i], "-rl") == 0) {
	    list_formats(_("Spice input formats"), FILE_READ_OP );
	    return 0;
	 } else if (strcmp(argv[i], "-wl") == 0) {
	    list_formats(_("Spice output formats"), FILE_WRITE_OP );
	    return 0;
	 } else if (strcmp(argv[i], "-rf") == 0) {
	    rformat = argv[++i];
	 } else if (strcmp(argv[i], "-wf") == 0) {
	    wformat = argv[++i];
	 } else if (strcmp(argv[i], "-o") == 0) {
	    output = argv[++i];
	 } else if (strcmp(argv[i], "-pf") == 0) {
	    fmt = argv[++i];
	 }
      } else {
	 filename = argv[i];
      }
   }

   if ( ! filename ) {
      usage(prog);
   }
   wt = wavetable_new( NULL, "test");
   if ( sparams ) {
      snd = sound_new( sparams, wt);
      (void) snd; /* disable warning */
   } else {
      ss = spicestream_new( filename, rformat, wt);
      if (ss->status) {
	 ret = ss->status;
	 goto end;
      }
   }
   ret = wavetable_fill_tables( wt, filename);
   if ( ret < 0 ) {
      goto end;
   }
    /* fmt is a format for double in printf */
   fileformat_file_write( wt, output, wformat, fmt );

end:
   wavetable_destroy(wt);
   return ret;
}

			 
