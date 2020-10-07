/*
 * userprefs.c - User preferences and configuration functions
 * 
 * include LICENSE
 */
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <strcatdup.h>
#include <fsutil.h>
#include <stutil.h>
#include <msglog.h>
#include <appmem.h>
#include <incvar.h>
#include <strobj.h>
#include <duprintf.h>
#include <dliststr.h>

/* int prog_debug = 0; moved to main */


/*************************************************************/
 
#define THIS_IS_USERPREFS_FILE

#ifdef TRACE_MEM
#include <tracemem.h>
#endif

#include <userprefs.h>
 
        

/*************************************************************/ 

/*
 *  local prototupes
 */
void up_set_rcfile( UserPrefs *up, char *prog, char *rcFile);
static int up_file_parse(UserPrefs *up );
int up_rc_file_write( UserPrefs *up );
/*
 *** \brief Allocates memory for a new UserPrefs object.
 */

UserPrefs *up_new( char *prog, char *rcFile,  ConfigDescTable *pdesc )
{
   UserPrefs *up;

   up = app_new0(UserPrefs, 1);
   up_construct( up, prog, rcFile, pdesc );
   app_class_overload_destroy( (AppClass *) up, up_destroy );
   return up;
}

/** \brief Constructor for the UserPrefs object. */

void up_construct( UserPrefs *up, char *prog, char *rcFile, ConfigDescTable *pdesc )
{
   app_class_construct( (AppClass *) up );

   up_set_rcfile( up, prog, rcFile);
   if ( ! pdesc ){
      pdesc = confDesc;
   }
   up->pdesc = pdesc;
   up->prog = prog;

   up_init_defaults(up);
   up_file_parse( up );
   if ( up->newFile == 0 && mkcfVersion != up->version){
      /* make a new rc with all variables */
      char *name = app_strcatdup( up->rcFile, ".bak", NULL);
      rename( up->rcFile, name );
      app_free(name);
      up_rc_file_write(up);
      dlist_delete_all( up->incVars );
      up->incVars = NULL;
      up_file_parse( up );
   }
   prog_debug = up->prog_debug;
}

/** \brief Destructor for the UserPrefs object. */

void up_destroy(void *up)
{
   UserPrefs *this = (UserPrefs *) up;

   if (up == NULL) {
      return;
   }
   fdbuf_destroy(this->linebuf);
   app_free(this->rcDir);
   app_free(this->rcFile);

   dlist_delete_all( this->incVars );
   
   up_dyn_destroy(this);
   app_class_destroy( up );
}


/*
 * if not given rcfile
 *   use rcfile in current dir if file exists - rcDir is current dir
 *   use rcfile progrc in $HOME/.prog/        - rcDir is $HOME/.prog
 * if given rcfile
 *   if rcfile is relative, set arcfile relative to current directory 
 *   else rcfile = arcfile is absolute.
 *      if arcfile is directory - rcDir = arcfile, rcfile = progrc
 *      if arcfile is file - rcDir = dirname(arcfile), rcfile = basename(arcfile)
 */
void up_set_rcfile( UserPrefs *up, char *prog, char *rcFile)
{
   char *s;
   char *rc;
   char *cwd;
   struct stat st ;
   
   rc = app_strcatdup( prog, "rc", 0 );
   cwd = fsu_getcwd();
   
   if ( rcFile == NULL ){
      if (stat(rc , &st) < 0 ) {
         /* no rcfile in current dir */
         up->rcDir = app_strcatdup( fsu_home_dir(), "/.", prog, 0 );
      } else {
         up->rcDir = app_strdup(cwd);
      }
      up->rcFile = app_strcatdup( up->rcDir, "/", rc, 0 );
   } else {
      char *tmp;
      if ( *rcFile != '/' ) {
         /* relative file */ 
         tmp = app_strcatdup( cwd, "/", rcFile, 0 );
      } else {
         tmp = app_strdup( rcFile);
      }
      if (stat(tmp , &st) == 0 ) {
         if ( st.st_mode & S_IFDIR ){
            /* rcfile is a dir */
            up->rcDir = app_strdup(tmp);
            up->rcFile = app_strcatdup( up->rcDir, "/", rc, 0 );
         } else {
            up->rcFile = app_strdup( tmp );
            /* rcfile is a file */
            if ( ( s = strrchr(tmp , '/' ) ) ){
               *s = 0;
            }
            up->rcDir = app_strdup( tmp );
         }
      } else {
         msg_fatal(_("Can't stat '%s'."), tmp);
      }
      app_free(tmp);
   }
   app_free(rc);
   app_free(cwd);
}


int up_set_var(UserPrefs *up, ConfigDescTable *pdesc, char *tok)
{
   void *varptr = pdesc->varfunc(up);
   int *pi = (int *) varptr;
   int val ;

   if (*tok == '\0' && pdesc->vartype != TINI ) {
      return 0 ; /* keep default value */
   }
   
   switch(pdesc->vartype) {
    case TBOOL :
      if ( sscanf(tok, "%d" , &val) == 1) {
	 if (val != 0 ) {
	   val = 1 ;
	 }
	 *pi = val ;
	 return 0 ;
      }
      break ;
    case TDECI :
      if ( sscanf(tok, "%d" , &val) == 1) {
	 *pi = val ;
	 return 0 ;
      }
      break ;
    case THEXA :
      {
	 unsigned int uval;
	 
	 if ( app_strncasecmp( tok , "0x", 2) == 0 ) {
	    tok += 2;
	 }
 	 if ( sscanf( tok, "%x" , &uval) == 1) {
	    *pi = uval ;
	    return 0 ;
	 }
      }
      break ;
    case TPTR :
      {
	 char **pv = (char **) varptr;
	 app_free(*pv);
	 *pv = app_strdup( tok ) ;
	 return 0 ;
      }
    case TLIST :
      {
         DList **head = (DList **) varptr;
	 *head = dlist_str_set_ndata( *head, tok, up->curel);
	 return 0;
      }
    case TARYSTR :
      {
	 ArrayStr **ary = (ArrayStr **) varptr;
	 array_strPtr_replace_kill( *ary, tok, up->curel );
	 return 0 ;
      }
   }
   msg_warning(_("Variable '%s' ignored. Val '%s' at line %d"),
	       pdesc->extTok, tok, fdbuf_get_lineno(up->linebuf) ) ;
   return 1 ;
}

void up_set_inc_var(UserPrefs *up, ConfigDescTable *pdesc)
{
   if ( pdesc->vartype != TINI ) {
      unsigned int linenum = fdbuf_get_lineno(up->linebuf);
      IncVar *var = var_new( linenum, pdesc->varfunc(up), pdesc->vartype );
      up->incVars = dlist_add_tail( up->incVars, (AppClass *) var );
   }
}

ConfigDescTable *up_get_table_entry (  ConfigDescTable *pdesc, char *name )
{
   /* find name in table */
   while (pdesc->extTok){ 
      if (app_strcmp( name, pdesc->extTok) == 0) {
	 return pdesc;
      }
      pdesc++ ;
   }
   return NULL;
}
   
/*
 * comment : C comment style
 */
  
int up_skip_comment( char *line, int in_comment)
{
   char *s;
   char lastc = 0;
   int in_quote = 0;
   char *pcmt = NULL;
   
   if ( in_comment ){
      pcmt = line;
   }
 
   for ( s = line; *s ; s++ ) {
      if ( *s == '"' ) {
	 if ( in_quote == 0 ) {
	    in_quote = 1;
	 } else {
	    in_quote = 0;
	 }
      }
      if ( lastc == '/' && *s == '*' && in_quote == 0 ) {
	 in_comment = 1;
	 pcmt = s - 1;
      }
      if ( lastc == '*' && *s == '/' && in_quote == 0 ) {
	 if ( in_comment ) {
	    in_comment = 0;
	 }
      }
      lastc = *s;
   }
   if ( pcmt ) {
      *pcmt = 0;
   }
   return in_comment;
}


/*
 * Configuration file parse
 */

int up_rc_read_file(UserPrefs *up )
{
   int in_comment = 0;
   char *line;
   char *tok;
   char *sn;
   ConfigDescTable *pdesc ;
   ConfigDescTable *curTable = up->pdesc;
	
   /* open file */
   up->linebuf = fdbuf_new ( up->rcFile, "r", 128);
   if ( up->linebuf->status ) {
      msg_fatal(_("Can't open file '%s': %s\n"), up->rcFile, strerror (errno));
   }
   fdbuf_set_flags(up->linebuf, FDB_JOIN_LINES | FDB_STRIP_CR);

   while( (line = fdbuf_get_line(up->linebuf)) != NULL) {
      /* fprintf(stderr, "%s", line );*/
      
      /* skip blank at beginning */
      line += strspn( line, " \t");
      in_comment = up_skip_comment( line, in_comment);

      if ( *line == 0 ){
         continue;
      }
 		
      sn = line; /* sn string next */
      tok = stu_token_next(&sn, " =]", " =[{}" );
      if ( ! tok ) {
	 /* no token in line */
	 continue ;
      }
      while ( ( pdesc = up_get_table_entry ( curTable, tok)) == NULL ) {
	 /* token not found in table */
	 if ( curTable == up->pdesc ) {
	    msg_fatal(_("Bad configuration file at line %d :\n Variable '%s' not found in preferences table.\n"
			"May-be remove the file '%s' and run %s again"),
		      fdbuf_get_lineno( up->linebuf), tok , up->rcFile,
                     up->prog );
	 }
	 curTable = up->pdesc; /* back to UserPrefs */
      }

      /*
       * as a general rule string are surrounded with "
       */
      up->curel = 0; /* list element number */
      char *endstr = "\",";
      char *skipstr = "\" ,";
      if ( pdesc->vartype == TPTR || pdesc->vartype == TARYSTR ) {
         endstr = "\"";
      }
      while ((tok = stu_token_next( &sn, endstr, skipstr )) ) {
	 stu_rm_blank_at_end(tok);
	 up_set_var(up, pdesc, tok ) ;
	 up_set_inc_var(up, pdesc);
	 up->curel++;
      }
      if ( pdesc->vartype == TINI ) {
	 if ( app_strcmp(pdesc->extTok,"UserPrefs") == 0 ){
	    curTable = up->pdesc;
	 } else {
	    curTable = (ConfigDescTable *) pdesc->varfunc(up, 1);
	 }
      } else if ( up->curel == 0 ){
	 /* no info after = no call to set var to keep the default */
	 up_set_inc_var(up, pdesc);
      }
   }
   return 0 ;
}

void up_print_long_line(FILE *ofd, char *line)
{
   if ( strlen(line) > 75 ) {
      char *p = line;
      int pos = 0;
      while ( *p ) {
	 fputc( *p, ofd );
	 pos++;
	 if (pos > 70 && *p == ',') {
	    fputs( " \\\n        ",  ofd );
	    pos = 8;
	 }
	 p++;
      }
   } else {
      fputs( line, ofd);
   }
   fputc( '\n', ofd );
}

/*
 * Write descriptor to file with default parameters
 */
void up_rc_desc_write( UserPrefs *up, FILE *ofd, ConfigDescTable *pdesc,
		       int init )
{
   ConfigDescTable *newDesc ;
   char *line;
   
   for( ; pdesc->extTok ;  pdesc++ ) {
      if ( pdesc->pcomment ) {
	 if ( pdesc->vartype == TCMT ) {
	    fprintf( ofd, "\n    /* %s */\n\n", pdesc->pcomment);
	    continue;
	 } else {
            if ( pdesc->vartype == TINI ) {
               fprintf( ofd, "\n");
            }
	    fprintf( ofd, "    /* %s */\n", pdesc->pcomment);
	 }
      }
      if ( pdesc->vartype == TINI ) {
         up->curnum = -1;
         /*
          * loop thru the preinited instances of a sub Desc
          * curnum is the instance number
          * curptr is a pointer an the data structure set by up_init_.*
          */
	 do { /* write the inited struct to file */
	    up->curnum++;
	    fprintf( ofd, "\n[%s] {\n", pdesc->extTok);
	    newDesc = (ConfigDescTable *) pdesc->varfunc(up, 0);
	    up_rc_desc_write( up, ofd, newDesc, up->curnum );
	    fprintf( ofd, "}\n\n");
	 } while (0); /* just 1 instance at write time */
	 continue;
      }
      if ( init >= 0 ) {
	 char *str = var_mkstr(pdesc->varfunc(up), pdesc->vartype);
	 char *str2 = str;
	 if ( str == NULL) {
	    str2 = "";
	 }
	 line = app_strdup_printf("%s = %s", pdesc->extTok, str2);
	 app_free(str);
      } else {
	 line = app_strdup_printf("/* %s = */", pdesc->extTok);
      }
      up_print_long_line(ofd, line);
      app_free(line);   
   }
}
/*
 * Write configuration file if it does not exist
 * with default parameters
 */
int up_rc_file_write( UserPrefs *up )
{
   /* write the file */
   msg_infol( 5, "Writing File %s", up->rcFile);

   FILE *ofd = fopen( up->rcFile, "w");
   if ( ofd == NULL ){
      msg_fatal( _("Can't open file '%s' - %s"), up->rcFile, strerror(errno) );
   }
   up->version = mkcfVersion;
   up_rc_desc_write( up, ofd, up->pdesc, 1 );
   
   fclose(ofd);
   return 0;
}

/*
 *   use filename to read config file
 */
int up_rc_read_conf( UserPrefs *up )
{
   struct stat st ;

   if (stat(up->rcDir, &st) < 0 ) {
      mkdir(up->rcDir, 0755);
   }
   if (stat(up->rcFile, &st) < 0 ) {
      up_rc_file_write(up);
      up->newFile = 1;
      msg_info(_("created config file '%s'"), up->rcFile );
      /* read the file to set the variable list for rewrite */
   }
   return up_rc_read_file(up);
}

static int up_file_parse(UserPrefs *up )
{
   int ret;
   
   if ( up->rcFile == NULL ){
      msg_fatal("Rcfile not defined.");
   }
   ret = up_rc_read_conf(up) ;
   if ( ret ){
      msg_fatal(_("rcparse returned %d - Stopping"), ret);
   }
   return ret;
}

/*
 * return next rc line
 */

int up_find_next_line ( AppClass *n1, AppClass *n2 )
{
   IncVar *var = (IncVar *) n1;
   int *plineno = (int *) n2;
   
   if ( (var->linenum & LF_LINEMSK) > *plineno ) {
      return 0;
   }
   return 1;
}

void up_rc_file_rewrite(UserPrefs *up, FILE *ifd, FILE *ofd)
{
   char *linebuf = app_new(char, BUFSIZ);
   unsigned int curline = 0;
   unsigned int nextline = 0;
   IncVar *var = 0;
   int need_next = 1;
 
   while ( 1 ) {
      if ( need_next ){
	 var = (IncVar *) dlist_lookup( up->incVars, (AppClass *) &curline,
					up_find_next_line );
	 if ( var ) {
	    nextline = var->linenum & LF_LINEMSK;
	 } else {
	    nextline = LF_LINEMSK;
	 }
	 need_next = 0;
      }
      if ( fgets( linebuf, BUFSIZ, ifd) == NULL ) {
	 break;
      }
      curline++;
      
      if ( curline == nextline ){
	 need_next = 1;
         /* find = at end of line */
         char *s = strrchr( linebuf, '=');
         if ( ! s) {
            msg_errorl(2, "No rule at line %d '%s' in %s", curline, linebuf,
                       up->rcFile );
            fputs(linebuf, ofd);
            continue;
         }
         *(++s) = 0;
	 char *line = app_strdup(linebuf);
         char *varstr = var_get_str(var);
         if ( varstr ){
	    line = app_strappend (line, varstr, " ");
         }
         /* find a comment at end of line */
         s = app_strstr( ++s, "/*" );
         if ( s) {
	    line = app_strappend (line,  app_strdup(s), " ");
	 }
	 up_print_long_line(ofd, line);
	 app_free(line);
      } else {
         fputs(linebuf, ofd);
      }
   }
   app_free(linebuf);
}


/*
 * rewrite values in configuration file
 */
int up_rc_rewrite( UserPrefs *up )
{
   up->prog_debug = prog_debug;
   /* rewrite the file */
   msg_infol( 5, "Rewriting File %s", up->rcFile);
   char *name = app_strcatdup(  up->rcFile, ".XXXXXX", NULL);

   FILE *ifd =  fopen( up->rcFile, "r");
   if ( ifd == NULL ){
      msg_fatal( _("Can't open file '%s' - %s"),  up->rcFile, strerror(errno) );
   }
   FILE *ofd =  fdopen( mkstemp(name), "w");
   if ( ofd == NULL ){
      msg_fatal( _("Can't open file '%s' - %s"), name, strerror(errno) );
   }
   up_rc_file_rewrite(up, ifd, ofd);
   fclose(ofd);
   fclose(ifd);
   rename( name, up->rcFile );
   app_free(name);
      
   return 0;
}
