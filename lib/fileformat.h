#ifndef FILEFORMAT_H
#define FILEFORMAT_H

/*
 * fileformat.h - fileformat protocol interface
 * 
 * include LICENSE
 */

#include <sys/types.h>
#include <regex.h>

#define FILE_READ_OP        1
#define FILE_WRITE_OP       2

typedef struct _FileFormat FileFormat;

struct _FileFormat {
   char *name;              /* format name */
   char *fnrexp;            /* regex to match the file extension name */
   regex_t *creg;          /* compiled form of regexp */
   ReadHeaderFP rdHeaderfunc;
   ReadRowFP rdRowfunc;
   WriteFileFP wrFilefunc;
   char **globlist;          /* shell style glob patterm list */
};

/*
 * prototypes
 */

char *fileformat_get_next_name( int *i, int option);
int fileformat_get_index(char *format);
ReadHeaderFP fileformat_get_readhdr_func(int index);
ReadRowFP fileformat_get_read_func(int index);
WriteFileFP fileformat_get_write_func(int index);
int fileformat_read_header( SpiceStream *ss);
int fileformat_file_write( WaveTable *wt, char *filename, char *format, char *fmt);
char **fileformat_get_patterns(int index);

void fileformat_cleanup(void);

#endif /* FILEFORMAT_H */
