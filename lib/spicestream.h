#ifndef SPICESTREAM_H
#define SPICESTREAM_H

/*
 * spicestream.h - spicestream protocol interface
 * 
 * include LICENSE
 */

#include <fdbuf.h>
#include <wavetable.h>
#include <wavevar.h>

/* values for flags field */
enum _SSFlagsInfo {
   SSF_ESWAP  = (1 << 0),    /* need to swap binary data */
   SSF_PUSHBACK = (1 << 1),  /* the stream work in push back mode  */
   SSF_NORDHDR = (1 << 2),   /* do not read header again */
   SSF_CPVARS = (1 << 3),    /* need to recreate vars    */
   SSF_PHDR = (1 << 4),      /* read partial header at second time  */
};

typedef struct _SpiceStream SpiceStream;

typedef int  (*ReadHeaderFP)  ( SpiceStream *ss );
typedef int  (*ReadRowFP)  ( SpiceStream *ss );
typedef int  (*ReadSweepFP)  ( SpiceStream *ss );
typedef int  (*ReadGetVal)  ( SpiceStream *ss, double *val );

typedef int  (*WriteFileFP)  ( FILE *fd, WaveTable *wt, char *fmt );

struct _SpiceStream {
   AppClass parent;
   WaveTable *wt;
   char *filename;       /* pointer copy of linebuf filename */
   char *format;         /* reaquested format */
   FDBuf *linebuf;
   DBuf *dbuf;           /* for text manipulation in hspice */
   FILE *fd;             /* file desc for writting */
   int nrows;            /* number of rows   */
   int ncols;            /* number of cols   */
   int curcol;           /* cur col   */
   WDataSet *wds;        /* dataset  */
   ReadHeaderFP readHdrFunc; /* read header function */
   ReadRowFP readRowFunc; /* read row function */
   int status;           /* status of operation */
   double *rowData;      /* temp array of double data for one row */
   int ntables;          /* number of tables in tables */
   int numVars;          /* number of added variables */
   int need_update;
   /* some more variables for hspice */
   int nsweepparam;
   int expected_vals;    /* number of float in the data block */
   int read_sweepparam;
   int read_vals;        /* number of float read              */
   int read_tables;
   int read_rows;
   int flags;
   ReadSweepFP rdSweepFunc;
   ReadGetVal rdValFunc;
   int ndv;
   /* some more variables for nsout */
   double time_resolution;
   double current_resolution;
   double voltage_resolution;
   int minindex;
   int maxindex;
   double *datrow; /* temporary data row indexed by ns indices */
   int *nsindexes; /* indexed by dvar, contains ns index number */
   /* some more variables for spice3 */
   int currow;
   /* some more variables for .wav */
   int bps;       /* bit per sample */
};


extern int sf_rdhr_sound(SpiceStream *ss );
int sf_readrow_sound(SpiceStream *ss);

/*
 * prototypes
 */
SpiceStream *spicestream_new( char *filename, char *format, WaveTable *wt );
void spicestream_construct( SpiceStream *ss, char *filename, char *format, WaveTable *wt);
void spicestream_destroy(void *ss);

void spicestream_val_add( SpiceStream *ss, double val);
void spicestream_var_add( SpiceStream *ss, char *varName, int type, int ncols );
int spicestream_read_hdr( AppClass *ss );
int spicestream_read_rows( AppClass *ss );

int spicestream_file_write( char *filename, char *format, WDataSet *wds, char *fmt);

#endif /* SPICESTREAM_H */
