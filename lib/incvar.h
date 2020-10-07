#ifndef INCVAR_H
#define INCVAR_H

/*
 * incvar.h - info for rewriting variables
 * 
 * include LICENSE
 */

enum _VarTypeInfo {
   TBOOL = 1,
      TDECI,
      THEXA,
      TPTR,         /* pointer to malloced string */   
      TARYSTR,      /* array of pointer to string */
      TARY,         /* array of pointer to object */
      TLIST,        /* string with elements separated with ',' */
      TCMT,         /* comment type */
      TINI,         /* initialize entry */
};

#include <appclass.h>

/*
 * line coding field 
 *   line number   : 19 bit
 *   file type     : 4 
 *   file number   : 8 
 *  count modified : 1
*/

enum _lineFieldInfo {
   LF_TYPESHIFT  = 19,
   LF_FNUMSHIFT  = 23,
   LF_LINEMSK    = (1 << LF_TYPESHIFT) - 1,
   LF_TYPEMSK    = ((1 << LF_FNUMSHIFT) - 1) & ~LF_LINEMSK,
   LF_MODIFIED   = (unsigned int) (1 << 31),
   LF_FNUMMSK    = (LF_MODIFIED - 1) & ~(LF_TYPEMSK | LF_LINEMSK) ,
};


typedef struct _IncVar IncVar;

struct _IncVar {
   AppClass parent;
   unsigned int linenum;     /* line number where the rule is */
   void *varptr;             /* address of the variable       */
   int vartype;              /* type of the variable  e       */
};

/*
 * prototypes
 */

IncVar *var_new( unsigned int linenum,  void *varptr, int vartype );
void var_construct( IncVar *var, unsigned int linenum, void *varptr, int vartype);
void var_destroy(void *var);

char *var_mkstr(void *varptr, int vartype);
char *var_get_str(IncVar *var);

#endif /* INCVAR_H */
