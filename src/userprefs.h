#ifndef USERPREFS_H
#define USERPREFS_H

/*
 * userprefs.h - Gaw User preferences and configuration functions
 *     
 * 
 * include LICENSE
 */

/*
 * WARNING : This file use the same name for each project
 *           but its contents is project dependant
 * There are 3 parts in configuration system :
 *   - the C code of configuration functions in userprefs.c file independant of
 *     the project.
 *   - the private C code included between #ifdef THIS_IS_USERPREFS_FILE #endif
 *     in the  userprefs.h file that is application dependant.
 *   - the public code exported to other files of the application before and after the
 *     #ifdef THIS_IS_USERPREFS_FILE #endif in the userprefs.h file.
 *     THIS_IS_USERPREFS_FILE is THIS_IS_USERPREFS_FILE by default.
 */

/*
 * To add a new variable:
 *      1 - add the variable at the end of struct _UserPrefs .
 *          Do not forget to put a comment.
 *      2 - If you need a default value different of 0,
 *          do an assignation for this variable in
 *          the function up_init_defaults(UserPrefs *up).
 *      3 - run mkcf : mkcf userprefs.h
 *          it will do the hard job for you.
 */

#include <fdbuf.h>
#include <dliststr.h>
#include <arraystr.h>
// #include <dliststr.h>

typedef struct _UserPrefs UserPrefs;
typedef struct _ConfigDescTable ConfigDescTable;
// typedef void *(*VarAddr_FP)( UserPrefs *up );
typedef void *(*VarAddr_FP)( ); ; /* do not check arguments */

/* Description Table */
struct _ConfigDescTable {
   char *extTok;              /* ptr on file name token : TOKEN  */
   VarAddr_FP varfunc;        /* function returning the addres of variable */
   int  vartype;              /* variable type                   */
   char *pcomment;            /* a comment for the variable      */
} ;

    
struct _UserPrefs {
   AppClass parent;
   int newFile;             /* a new rcfile was created */
   char *rcDir;             /* ressource directory $HOME/.prog         */
   char *rcFile;            /* preferences file name                    */
   char *prog;              /* program name */
   ConfigDescTable *pdesc;  /* pointer to the config description table  */
   FDBuf *linebuf;          /* dynamic buffer for read the file         */
   DList *incVars;          /* list of included variables from rcfile   */
   int curel;               /* current element index                    */
   int curnum;              /* current element index                    */
//   AppClass *curptr;        /* current ponter to objet                  */

/* mkcf User Variables prefix up */
   int version;             /* userprefs version managed by mkcf */
   int prog_debug;          /* global debug flag  */
   int verbose;             /* level of verbosity */
   int  npanels;  /* Number of panels to be displayed by default */
   int  min_win_width;  /* Window minimum width */
   int  min_win_height;  /* Window minimum height */
   int  max_ps_y;  /* lower y coord for panel_scrolled on screen */
   int  gridXSpacing;  /* grid X line spacing */
   int  gridYSpacing;  /* grid Y line spacing */
   int  panelHeight;  /* nominal height for panel */
   int  panelWidth;  /* nominal width for panel */
   int  minPanelHeight;  /* minimun height for panel */
   int  minPanelWidth;  /* minimum width for panel */
   int  allowResize;    /* allow resize the main window */
   int  drawAlgo;       /* index of the drawing algorithm */
   int  toolBarStyle;  /* icons, text or both */
   char *panelBgColor;  /* RGB color for panel background 0-0xff, 0-0xff, 0-0xff */
   int  lmtableWidth;  /* nominal width for buttons and masurements table */
   int  showGrid;  /* Should we show the grid in panel */
   char *gridColor;  /* RGB color for grid 0-0xff, 0-0xff, 0-0xff */
   char *hlColor;  /* RGB color for highlight  0-0xff, 0-0xff, 0-0xff */
   char *srangeColor;  /* RGB color for select range  0-0xff, 0-0xff, 0-0xff */
   ArrayStr *cursorsColor;  /* [0, 1, 2] cursors 0, 1, diff, colors  0xffffff */
   ArrayStr *buttonBgColor;  /* [state] RGB button bg color[state] 0-0xff, 0-0xff, 0-0xff */
   char *lboxfgColor;  /* RGB color for list box button foreground color */
   char *lboxbgColor;  /* RGB color for list box button background color */
   char *wavebgColor;  /* RGB color for wave button back ground color */
   int  showXLabels;  /* Should we show X labels */
   int  showYLabels;  /* Should we show Y labels */
   int  showMoreYLabels;  /* choice between old mode with 2 labels or new mode with more */
   int  scientific;  /* Should we use scientific conversion mode */
   int  setLogX;  /* Should we set log X scale */
   int  setLogY;  /* Should we set log Y scale */
   int  showYDiff;  /* Should we show Y diff measure buttons */
   char *imgFmt;  /* image default format: jpeg, png, tiff,... */
   char *imgBaseName;  /* Base name for saving image */
   int  jpegQuality;  /* jpeg quality for saving image */
   int  pngCompression;  /* png compression for saving image */
   int  exportTimeout;  /* time to wait in millisecond before saving image to file */
   int  listenPort;  /* port used for listen in gaw server */
   char *dataFileFormat;  /* preferred format for data file */
   char *lastDataFile;  /* last used file name */
   int rate;  /* sampling rate */
   int duration;  /* duration length of capture in seconds */
   int pcm_format;  /* value of pcm format : SND_PCM_FORMAT_S16_LE ... */
   int input;  /* index of the mixer input */
   char *cardidstr;  /* index of the card '-' for default, '0' ... */
   char *helpCmd;  /* command called for display help */
   char *userGuide;  /* Gaw user guide page */
   char *webSite;  /* gaw main page web site */
   char *text_bg_color;  /* background color for drawing text */
   char *text_fg_color;  /* foreround color for drawing text */
   char *text_font;      /* user default font for drawing text */
   char *angle;          /* degree: default angle for drawing text */
   char *date_fmt; /* a strptime, strftime format for time_t conv */
   char *diffdate_fmt;  /* a printf format to exprees a diff date */
   int xconvert;   /* index of the X conversion */
};


/*
 * The following part is included in userprefs.c
 */
#ifdef THIS_IS_USERPREFS_FILE

static int mkcfVersion = 16;

void *up_addr_up_version( UserPrefs *up )
{
   return &up->version;
}

void *up_addr_up_prog_debug( UserPrefs *up )
{
   return &up->prog_debug;
}

void *up_addr_up_verbose( UserPrefs *up )
{
   return &up->verbose;
}

void *up_addr_up_npanels( UserPrefs *up )
{
   return &up->npanels;
}

void *up_addr_up_min_win_width( UserPrefs *up )
{
   return &up->min_win_width;
}

void *up_addr_up_min_win_height( UserPrefs *up )
{
   return &up->min_win_height;
}

void *up_addr_up_max_ps_y( UserPrefs *up )
{
   return &up->max_ps_y;
}

void *up_addr_up_gridXSpacing( UserPrefs *up )
{
   return &up->gridXSpacing;
}

void *up_addr_up_gridYSpacing( UserPrefs *up )
{
   return &up->gridYSpacing;
}

void *up_addr_up_panelHeight( UserPrefs *up )
{
   return &up->panelHeight;
}

void *up_addr_up_panelWidth( UserPrefs *up )
{
   return &up->panelWidth;
}

void *up_addr_up_minPanelHeight( UserPrefs *up )
{
   return &up->minPanelHeight;
}

void *up_addr_up_minPanelWidth( UserPrefs *up )
{
   return &up->minPanelWidth;
}

void *up_addr_up_allowResize( UserPrefs *up )
{
   return &up->allowResize;
}

void *up_addr_up_drawAlgo( UserPrefs *up )
{
   return &up->drawAlgo;
}

void *up_addr_up_toolBarStyle( UserPrefs *up )
{
   return &up->toolBarStyle;
}

void *up_addr_up_panelBgColor( UserPrefs *up )
{
   return &up->panelBgColor;
}

void *up_addr_up_lmtableWidth( UserPrefs *up )
{
   return &up->lmtableWidth;
}

void *up_addr_up_showGrid( UserPrefs *up )
{
   return &up->showGrid;
}

void *up_addr_up_gridColor( UserPrefs *up )
{
   return &up->gridColor;
}

void *up_addr_up_hlColor( UserPrefs *up )
{
   return &up->hlColor;
}

void *up_addr_up_srangeColor( UserPrefs *up )
{
   return &up->srangeColor;
}

void *up_addr_up_cursorsColor( UserPrefs *up )
{
   return &up->cursorsColor;
}

void *up_addr_up_buttonBgColor( UserPrefs *up )
{
   return &up->buttonBgColor;
}

void *up_addr_up_lboxfgColor( UserPrefs *up )
{
   return &up->lboxfgColor;
}

void *up_addr_up_lboxbgColor( UserPrefs *up )
{
   return &up->lboxbgColor;
}

void *up_addr_up_wavebgColor( UserPrefs *up )
{
   return &up->wavebgColor;
}

void *up_addr_up_showXLabels( UserPrefs *up )
{
   return &up->showXLabels;
}

void *up_addr_up_showYLabels( UserPrefs *up )
{
   return &up->showYLabels;
}

void *up_addr_up_showMoreYLabels( UserPrefs *up )
{
   return &up->showMoreYLabels;
}

void *up_addr_up_scientific( UserPrefs *up )
{
   return &up->scientific;
}

void *up_addr_up_setLogX( UserPrefs *up )
{
   return &up->setLogX;
}

void *up_addr_up_setLogY( UserPrefs *up )
{
   return &up->setLogY;
}

void *up_addr_up_showYDiff( UserPrefs *up )
{
   return &up->showYDiff;
}

void *up_addr_up_imgFmt( UserPrefs *up )
{
   return &up->imgFmt;
}

void *up_addr_up_imgBaseName( UserPrefs *up )
{
   return &up->imgBaseName;
}

void *up_addr_up_jpegQuality( UserPrefs *up )
{
   return &up->jpegQuality;
}

void *up_addr_up_pngCompression( UserPrefs *up )
{
   return &up->pngCompression;
}

void *up_addr_up_exportTimeout( UserPrefs *up )
{
   return &up->exportTimeout;
}

void *up_addr_up_listenPort( UserPrefs *up )
{
   return &up->listenPort;
}

void *up_addr_up_dataFileFormat( UserPrefs *up )
{
   return &up->dataFileFormat;
}

void *up_addr_up_lastDataFile( UserPrefs *up )
{
   return &up->lastDataFile;
}

void *up_addr_up_rate( UserPrefs *up )
{
   return &up->rate;
}

void *up_addr_up_duration( UserPrefs *up )
{
   return &up->duration;
}

void *up_addr_up_pcm_format( UserPrefs *up )
{
   return &up->pcm_format;
}

void *up_addr_up_input( UserPrefs *up )
{
   return &up->input;
}

void *up_addr_up_cardidstr( UserPrefs *up )
{
   return &up->cardidstr;
}

void *up_addr_up_helpCmd( UserPrefs *up )
{
   return &up->helpCmd;
}

void *up_addr_up_userGuide( UserPrefs *up )
{
   return &up->userGuide;
}

void *up_addr_up_webSite( UserPrefs *up )
{
   return &up->webSite;
}

void *up_addr_up_text_bg_color( UserPrefs *up )
{
   return &up->text_bg_color;
}

void *up_addr_up_text_fg_color( UserPrefs *up )
{
   return &up->text_fg_color;
}

void *up_addr_up_text_font( UserPrefs *up )
{
   return &up->text_font;
}

void *up_addr_up_angle( UserPrefs *up )
{
   return &up->angle;
}

void *up_addr_up_date_fmt( UserPrefs *up )
{
   return &up->date_fmt;
}

void *up_addr_up_diffdate_fmt( UserPrefs *up )
{
   return &up->diffdate_fmt;
}

void *up_addr_up_xconvert( UserPrefs *up )
{
   return &up->xconvert;
}

ConfigDescTable confDesc[] = {  /* UserPrefs - by mkcf */
{ "", 0, TCMT,
     "User Preferences"       },
{ "up_version", up_addr_up_version, TDECI,
     "userprefs version managed by mkcf"       },
{ "up_prog_debug", up_addr_up_prog_debug, TDECI,
     "global debug flag"       },
{ "up_verbose", up_addr_up_verbose, TDECI,
     "level of verbosity"       },
{ "up_npanels", up_addr_up_npanels, TDECI,
     "Number of panels to be displayed by default"       },
{ "up_min_win_width", up_addr_up_min_win_width, TDECI,
     "Window minimum width"       },
{ "up_min_win_height", up_addr_up_min_win_height, TDECI,
     "Window minimum height"       },
{ "up_max_ps_y", up_addr_up_max_ps_y, TDECI,
     "lower y coord for panel_scrolled on screen"       },
{ "up_gridXSpacing", up_addr_up_gridXSpacing, TDECI,
     "grid X line spacing"       },
{ "up_gridYSpacing", up_addr_up_gridYSpacing, TDECI,
     "grid Y line spacing"       },
{ "up_panelHeight", up_addr_up_panelHeight, TDECI,
     "nominal height for panel"       },
{ "up_panelWidth", up_addr_up_panelWidth, TDECI,
     "nominal width for panel"       },
{ "up_minPanelHeight", up_addr_up_minPanelHeight, TDECI,
     "minimun height for panel"       },
{ "up_minPanelWidth", up_addr_up_minPanelWidth, TDECI,
     "minimuùm width for panel"       },
{ "up_toolBarStyle", up_addr_up_toolBarStyle, TDECI,
     "icons, text or both"       },
{ "up_panelBgColor", up_addr_up_panelBgColor, TPTR,
     "RGB color for panel background 0-0xff, 0-0xff, 0-0xff"       },
{ "up_lmtableWidth", up_addr_up_lmtableWidth, TDECI,
     "nominal width for buttons and masurements table"       },
{ "up_showGrid", up_addr_up_showGrid, TDECI,
     "Should we show yhe grid in panel"       },
{ "up_gridColor", up_addr_up_gridColor, TPTR,
     "RGB color for grid 0-0xff, 0-0xff, 0-0xff"       },
{ "up_hlColor", up_addr_up_hlColor, TPTR,
     "RGB color for highlight  0-0xff, 0-0xff, 0-0xff"       },
{ "up_srangeColor", up_addr_up_srangeColor, TPTR,
     "RGB color for select range  0-0xff, 0-0xff, 0-0xff"       },
{ "up_cursorsColor", up_addr_up_cursorsColor, TARYSTR,
     "[0, 1, 2] cursors 0, 1, diff, colors  0xffffff"       },
{ "up_buttonBgColor", up_addr_up_buttonBgColor, TARYSTR,
     "[state] RGB button bg color[state] 0-0xff, 0-0xff, 0-0xff"       },
{ "up_showXLabels", up_addr_up_showXLabels, TDECI,
     "Should we show X labels"       },
{ "up_showYLabels", up_addr_up_showYLabels, TDECI,
     "Should we show Y labels"       },
{ "up_showMoreYLabels", up_addr_up_showMoreYLabels, TDECI,
     "choice between old mode with 2 labels or new mode with more"       },
{ "up_scientific", up_addr_up_scientific, TDECI,
     "Should we use scientific conversion mode"       },
{ "up_setLogX", up_addr_up_setLogX, TDECI,
     "Should we set log X scale"       },
{ "up_setLogY", up_addr_up_setLogY, TDECI,
     "Should we set log Y scale"       },
{ "up_showYDiff", up_addr_up_showYDiff, TDECI,
     "Should we show Y diff measure buttons"       },
{ "up_imgFmt", up_addr_up_imgFmt, TPTR,
     "image default format: jpeg, png, tiff,..."       },
{ "up_imgBaseName", up_addr_up_imgBaseName, TPTR,
     "Base name for saving image"       },
{ "up_jpegQuality", up_addr_up_jpegQuality, TDECI,
     "jpeg quality for saving image"       },
{ "up_pngCompression", up_addr_up_pngCompression, TDECI,
     "png compression for saving image"       },
{ "up_exportTimeout", up_addr_up_exportTimeout, TDECI,
     "time to wait in millisecond before saving image to file"       },
{ "up_listenPort", up_addr_up_listenPort, TDECI,
     "port used for listen in gaw server"       },
{ "up_dataFileFormat", up_addr_up_dataFileFormat, TPTR,
     "preferred format for data file"       },
{ "up_lastDataFile", up_addr_up_lastDataFile, TPTR,
     "last used file name"       },
{ "up_rate", up_addr_up_rate, TDECI,
     "sampling rate"       },
{ "up_duration", up_addr_up_duration, TDECI,
     "duration length of capture in seconds"       },
{ "up_pcm_format", up_addr_up_pcm_format, TDECI,
     "value of pcm format : SND_PCM_FORMAT_S16_LE ..."       },
{ "up_input", up_addr_up_input, TDECI,
     "index of the mixer input"       },
{ "up_cardidstr", up_addr_up_cardidstr, TPTR,
     "index of the card '-' for default, '0' ..."       },
{ "up_helpCmd", up_addr_up_helpCmd, TPTR,
     "command called for display help"       },
{ "up_userGuide", up_addr_up_userGuide, TPTR,
     "Gaw user guide page"       },
{ "up_webSite", up_addr_up_webSite, TPTR,
     "gaw main page web site"       },
{ "up_text_bg_color", up_addr_up_text_bg_color, TPTR,
     "background color for drawing text"       },
{ "up_text_fg_color", up_addr_up_text_fg_color, TPTR,
     "foreround color for drawing text"       },
{ "up_text_font", up_addr_up_text_font, TPTR,
     "user default font for drawing text"       },
{ "up_angle", up_addr_up_angle, TPTR,
     "degree: default angle for drawing text"       },
{ "up_allowResize", up_addr_up_allowResize, TDECI,
     "allow resize the main window"       },
{ "up_drawAlgo", up_addr_up_drawAlgo, TDECI,
     "index of the drawing algorithm"       },
{ "up_lboxbgColor", up_addr_up_lboxbgColor, TPTR,
     "RGB color for list box button back ground color"       },
{ "up_wavebgColor", up_addr_up_wavebgColor, TPTR,
     "RGB color for wave button back ground color"       },
{ "up_lboxfgColor", up_addr_up_lboxfgColor, TPTR,
     "RGB color for list box button foreground color"       },
{ "up_xconvert", up_addr_up_xconvert, TDECI,
     "index of the X conversion"       },
{ "up_date_fmt", up_addr_up_date_fmt, TPTR,
     "a strptime, strftime format for time_t conv"       },
{ "up_diffdate_fmt", up_addr_up_diffdate_fmt, TPTR,
     "a printf format to exprees a diff date"       },
    {  0, 0, 0, 0, },
};


void up_dyn_destroy(UserPrefs *up)
{
    app_free(up->panelBgColor);
    app_free(up->gridColor);
    app_free(up->hlColor);
    app_free(up->srangeColor);
    array_destroy(up->cursorsColor);
    array_destroy(up->buttonBgColor);
    app_free(up->lboxfgColor);
    app_free(up->lboxbgColor);
    app_free(up->wavebgColor);
    app_free(up->imgFmt);
    app_free(up->imgBaseName);
    app_free(up->dataFileFormat);
    app_free(up->lastDataFile);
    app_free(up->cardidstr);
    app_free(up->helpCmd);
    app_free(up->userGuide);
    app_free(up->webSite);
    app_free(up->text_bg_color);
    app_free(up->text_fg_color);
    app_free(up->text_font);
    app_free(up->angle);
    app_free(up->date_fmt);
    app_free(up->diffdate_fmt);
}

/* mkcf end of stuff generated by mkcf (do not remove) */



void up_init_defaults(UserPrefs *up)
{
   int i;
   
   prog_debug = 0;
   up->npanels = 2;
   up->min_win_width = 80;
   up->min_win_height = 50;
   up->gridXSpacing = 20;
   up->gridYSpacing = 20;
   up->panelHeight = 100;
   up->panelWidth = 400;
   up->minPanelHeight = 50;
   up->minPanelWidth = 100;
   up->toolBarStyle = 2;
   app_dup_str( &up->panelBgColor, "rgba(0,0,0,1)");   /* black */
   app_dup_str( &up->gridColor, "#662828");      /* light red */
   app_dup_str( &up->hlColor, "#ffffff");      /*  white */
   app_dup_str( &up->srangeColor, "#ffffff");  /*  white */
   app_dup_str( &up->srangeColor, "#ffffff");  /*  white */
   app_dup_str( &up->wavebgColor, "#000000");  /*  black */
   app_dup_str( &up->lboxfgColor, "#ffffff");  /*  white */
   app_dup_str( &up->lboxbgColor, "#000000");  /*  black */

   up->cursorsColor = array_strPtr_new( 4 );
   array_strPtr_add(up->cursorsColor, "#ffffff" );  /* cursor 0  white */
   array_strPtr_add(up->cursorsColor, "#ffff00" );  /* cursor 1  yellow */
   array_strPtr_add(up->cursorsColor, "" );  /* diff cursor */

   up->buttonBgColor = array_strPtr_new( 4 );
   for ( i = 0 ; i < 3 ; i++ ) {
      array_strPtr_add(up->buttonBgColor, "" );
   }

   up->lmtableWidth = 280;
   up->showXLabels = 1;
   up->showYLabels = 1;
   up->imgFmt = app_strdup("png");
   up->imgBaseName = app_strdup("panel");
   up->jpegQuality = 100;
   up->exportTimeout = 1500;  /* ms */
   up->cardidstr = app_strdup("-");
   up->input = 2;    /* correspond to 'Line' on my machine */
   up->rate = 48000;
   up->duration = 10;
   up->helpCmd = app_strdup("firefox %s");
   up->userGuide = app_strdup("http://www.rvq.fr/linux/gawman.php");
   up->webSite = app_strdup("http://www.rvq.fr/linux/gaw.php");
   up->text_bg_color = app_strdup("rgb(239,41,41)");
   up->text_fg_color = app_strdup("rgb(243,243,243)");
   up->text_font = app_strdup("Sans 10");
   up->angle = app_strdup("0");
   up->date_fmt = app_strdup("%d-%m-%Y %H:%M:%S");
   up->diffdate_fmt = app_strdup("%dd %dh %dm %ds");
}

#endif /* THIS_IS_USERPREFS_FILE */

extern int prog_debug;

/*
 * prototypes
 */
UserPrefs *up_new( char *prog, char *rcFile, ConfigDescTable *pdesc );
void up_construct( UserPrefs *up, char *prog, char *rcFile, ConfigDescTable *pdesc );
void up_destroy(void *up);

void up_init_defaults(UserPrefs *up);
int up_rc_read_file(UserPrefs *up );
int up_rc_rewrite( UserPrefs *up );

#endif /* USERPREFS_H */

