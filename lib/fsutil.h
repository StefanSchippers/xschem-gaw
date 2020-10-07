#ifndef FSUTIL_H
#define FSUTIL_H

/*
 * fsutil.c - filesystem utility functions
 * 
 * include LICENSE
 */


char *fsu_home_dir (void);
char *fsu_getcwd(void);
char *fsu_absolute_name( char *dir, char *fname);

#endif /* FSUTIL_H */
