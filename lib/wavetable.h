#ifndef WAVETABLE_H
#define WAVETABLE_H

/*
 * wavetable.h - wavetable protocol interface
 * 
 * include LICENSE
 */

/*
 * Wave Table - structure to hold the datasets
 */
typedef struct _WaveTable WaveTable;

#include <dataset.h>
#include <wavevar.h>

typedef int  (*ReadVarsFP)  ( AppClass *src );
typedef int  (*ReadDataFP)  ( AppClass *src );
typedef void  (*StreamDestroyFP) ( void *src );


struct _WaveTable {
   AppClass parent;
   GPtrArray *tables;    /* array of WDataSet  */
   int ntables;          /* number of tables in tables */
   AppClass *datafile;       /* back pointer to DataFile   */
   char *tblname;
   GPtrArray *swvars;    /* pointer to array of sweep var info */
   int nswvars;          /* number of var in array  nswvars */
   AppClass *dataSrc;    /* pointer to filling class   */
   StreamDestroyFP streamDestroyFunc; /* function to destroy stream */
   ReadVarsFP rdVarsFunc;
   ReadDataFP rdDataFunc;
   int bits;             /* sample size for writing .vaw file */
   int rate;             /* sample rate for writing .wav files */
};

/*
 * prototypes
 */
WaveTable *wavetable_new( AppClass *userdata, char *tblname );
void wavetable_construct( WaveTable *wt, AppClass *userdata, char *tblname );
void wavetable_destroy(void *wt);

WDataSet *wavetable_add( WaveTable *wt);
WDataSet *wavetable_get_dataset( WaveTable *wt, int idx);
WDataSet *wavetable_get_cur_dataset( WaveTable *wt);
void wavetable_remove( WaveTable *wt, int index);
void wavetable_set_funcs ( WaveTable *wt, AppClass *src, ReadVarsFP hdrfunc,
			   ReadDataFP rdfunc, StreamDestroyFP destroyfunc );
void wavetable_set_datafile ( WaveTable *wt, void *datafile );
void *wavetable_get_datafile ( WaveTable *wt );
char *wavetable_get_tblname ( WaveTable *wt );
int wavetable_fill_tables( WaveTable *wt, char *filename);
int wavetable_get_ntables ( WaveTable *wt );
void wavetable_foreach_wavevar(WaveTable *wt, GFunc func, gpointer *p);
WaveVar *wavetable_get_var_for_name(WaveTable *wt,  char *varName, int tblno );
void wavetable_swvar_add(WaveTable *wt, char *varName, int type, int ncols);
int wavetable_is_multisweep( WaveTable *wt);
char *wavetable_swvar_name_get(WaveTable *wt, int index);

#endif /* WAVETABLE_H */
