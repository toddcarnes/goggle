/*
 * File: common.h
 * Author: Douglas Selph
 * Maintained by: Robin Powell
 * $Id: common.h,v 1.2 1997/12/05 03:42:41 rlpowell Exp $
 * Purpose:
 *
 *   Common global variables and defines.
 */
#ifndef COMMON_DEF
#define COMMON_DEF

#include "platform.h"

#define CALLOC(N,S)     zcalloc(N,S,__FILE__,__LINE__)
#define MALLOC(S)       zmalloc(S,__FILE__,__LINE__)
#define REALLOC(P,S)    zrealloc(P,S,__FILE__,__LINE__)

#define HELP_USAGE      "README.usage"
#define HELP_CODE       "README.code"
#define HELP_SETUP      "README.setup"
/*#define HELP_FEATURES   "README.features" ??? not used */
#define HELP_BUGS       "README.bugs"
#define PLANET_FILE     "save.planets"
#define GROUP_FILE      "save.groups"
#define ROUTE_FILE      "save.routes"
#define SHIP_FILE       "save.ships"
#define VERSION_FILE    "save.version"
#define MSG_FILE        "save.msgs"
#define SAVE_ZOOM       "save.zooms"
#define TMP_FILE        "tmp.tmp"

#define MAX_TMPS        10
/* #define MAX_MSG_LINES   10  saved number of message lines ??? not used */

#define RIGHT_JUSTIFY    1
#define LEFT_JUSTIFY     0

#define JUMP_LEFT       CONTROL('h')
#define JUMP_RIGHT      CONTROL('l')
#define JUMP_UP         CONTROL('k')
#define JUMP_DOWN       CONTROL('j')
#define NEXT_FIELD      CONTROL('i')
#define REDRAW          CONTROL('r')
#define MOVE_LEFT       'h'
#define MOVE_RIGHT      'l'
#define MOVE_UP         'k'
#define MOVE_DOWN       'j'
#define SCROLL_LEFT     'H'
#define SCROLL_RIGHT    'L'
#define SCROLL_UP       'K'
#define SCROLL_DOWN     'J'
#define JUMP_CENTER     'C'
#define TOGGLE_SCREENS  ' '
#define ESCAPE          27

#define OPT_IS_SET(OPT,MASK) ((OPT & MASK) == MASK)

#define OKAY             0
#define DONE             0
#define FILEERR         -1
#define NO_MATCH        -2
#define IS_ERR          -3
#define NOT_OKAY        -4
#define NEW             -5
#define BAD_VALUE       -6
#define ABORT           -7
#define NOTHING         -8

#define QUERY            1
#define CLOSEST          0

#ifndef NULL
#define NULL             0
#endif

#ifndef TRUE
#define TRUE             1
#define FALSE            0
#endif

#define MAX_MSG        128
/* #define SQR(A)         ((A) * (A)) ??? not used */

#define DISPLAY_MAP      0
#define DISPLAY_FILE     1

/* #define INSERT           1 ??? not used */
#define REPLACE          0

/* typedef unsigned char byte; ??? not used */
typedef unsigned char Boolean;
/* typedef unsigned short byte2; ??? not used */

typedef struct {
  float max_width;
  int option;
} MapData;

typedef struct {
  int r, c;
} Position;

#define CBRT(V) pow(V,0.333333333333333)

#endif
