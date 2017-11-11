/*
 * File: screen.h
 * Author: Douglas Selph
 * Date: Apr 1992
 * Purpose:
 *
 *   Defines that the screen module uses.
 */
#ifndef SCREEN_
#define SCREEN_

/* Window numbers */
#define MAP_NUM 0
#define FILE_NUM 1

/* Column number defs */
#define ALL_FILE 0
#define HALF_FILE 1
#define NO_FILE 2

/* , operator returns second argument result, so this way we get
 *   the return we want _and_ the assert happens in the right place
 */
#define MAPWIN          MapWin()
#define FILEWIN         FileWin()
#define DATAWIN_HEIGHT  ScreenDataWinHeight()
#define MAPWIN_WIDTH    MapWinWidth()
#define FILEWIN_WIDTH   FileWinWidth()

#define PR_NO_OPTIONS   0
#define PR_START_ROW    1

/*
 * The callback below will be called as so:
 *    (*callback)(i_chr, arg)
 */
typedef struct {
  int id;			/* id to identify this item */
  int top;			/* pulldown menu identifier */
  char *name;			/* name of item */
  char *accl;			/* multi-letter keycode accelerator */
  int (*callback)();		/* callback to call when executed */
  pointer arg;			/* passed argument to call */
  Boolean is_pulldown;		/* top level name for pulldown */
} itemData, *itemList;

#define GM_UNDEFINED   0
#define GM_STRING      1
#define GM_INTEGER     2
#define GM_FLOAT       3
#define GM_DOUBLE      4
#define GM_LOGICAL     5
#define GM_LABEL       6

typedef struct {
  int type;		/* GM_STRING, GM_INTEGER... */
  char *prompt;
  union values {
    char *sval;
    double dval;
    float fval;
    int ival;
    Boolean lval;
  } u;
} PromptObj;

int ScreenInit (void);
int ScreenResize (void);
WINDOW *MapWin( void );
WINDOW *FileWin( void );
int ScreenDataWinHeight (void);
int MapWinWidth (void);
int FileWinWidth (void);
int ScreenPlaceCursor (void);
void ScreenTerm (void);
void ScreenRedraw (void);
void ScreenMainLoop (void);
void ScreenSetDefaultCallback (int (*I_func)(), int arg);
void ScreenSetRedrawCallback (int (*I_func)(), int arg);
void ScreenCallRedraw (void);
int ScreenJumpWorld (int i_ymod, int i_xmod);
int ScreenCursorMove (int i_ymod, int i_xmod);
int ScreenCursorJump (int i_ymod, int i_xmod);

#endif
