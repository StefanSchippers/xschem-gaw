#ifndef UTIL_H
#define UTIL_H

/*
 * util.c - utility functions
 * 
 * include LICENSE
 */

typedef struct _NameValue NameValue;

struct _NameValue {
   char *name;
   int value;
};

int uti_find_in_table( char *s, char *table[], int size);
char *uti_find_in_table_str( int type, char *table[], int size);

int uti_nv_find_in_table( char *s, NameValue table[], int size);
char *uti_nv_find_in_table_str( int type, NameValue table[], int size);

#endif /* UTIL_H */
