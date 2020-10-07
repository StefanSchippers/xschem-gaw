#ifndef STUTIL_H
#define STUTIL_H

/*
 * stutil.c - string utility functions
 * 
 * include LICENSE
 */

char *stu_token_next_sep(char **sn, char *endstr, char *skipstr, char *sep);
char *stu_token_next(char **sn, char *endstr, char *skipstr);
char *stu_str_next(char **sn, int *size);
void stu_rm_blank_at_end(char *s);
char *stu_rm_blank_at_start(char *s);
char *stu_rm_blank(char *s);
char *stu_unquote(char *str );
char *stu_toupper(char * p);
char * stu_ext_get(char *path);

#endif /* STUTIL_H */
