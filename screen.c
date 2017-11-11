/*
 * File: screen.c
 * Author: Douglas Selph
 * Maintainer: Robin Powell
 * $Id: screen.c,v 1.9 1998/06/19 20:58:00 rlpowell Exp rlpowell $
 * Purpose:
 *
 *   Provide a io interface using 'curses'.
 */
#include <stdio.h>
#include <curses.h>
#include <assert.h>
#include <unistd.h>
#include "common.h"
#include "screen.h"

#include "fileshow.h"
#include "planetshow.h"
#include "msg.h"
#include "menu.h"

/**
 **  Local Structures and Defines
 **/
static struct screen_data
{
    struct default_def
    {
	int (*func)();
	int arg;
    } def, redraw;

    struct flags
    {
	Boolean inited;
    } flag;
} gScr;

static int ScreenProcessAccl (int i_chr);

/**
 ** Functions:
 **/

/* static int read_next_line(); ??? never used */
extern int g_cur_display;
extern char g_msg[MAX_MSG];
WINDOW *g_data_win[2];
int g_data_height;
int g_data_width[2];
int g_insert_mode;
int g_num_col;
int old_num_col;
int g_starting_cols;

/*
 *  Intialize the screen
 */
int ScreenInit (void)
{
    int row, col;

    initscr();
    cbreak();
    noecho();

    /*
     * Set up variables relating to screen control:
     * initial row, initial col, screen height,
     * initial widths for file and map windows,
     * and initial number of columns.
     */
    gScr.flag.inited = TRUE;
    row = 1;
    col = 0;
    g_data_height = LINES-2;
    g_num_col = ALL_FILE;
   
    g_data_width[MAP_NUM] = 0;
    g_data_width[FILE_NUM] = COLS-g_data_width[MAP_NUM];

    /* Must always have file window */
    g_data_win[FILE_NUM] = newwin(g_data_height, g_data_width[FILE_NUM], 
	    row, col+g_data_width[MAP_NUM]);
    assert(g_data_win[FILE_NUM]);

    FileInit();

    /* Create windows */
    if( g_num_col != ALL_FILE )
    {
	g_data_win[MAP_NUM] = newwin(g_data_height, g_data_width[MAP_NUM], 
		row, col);
	assert(g_data_win[MAP_NUM]);
	
	
	if( g_num_col == NO_FILE )
	{
	    g_cur_display = DISPLAY_MAP;
	}
    }

    
    return(DONE);
}

int ScreenResize (void)
{
    int row, col;

    if (g_data_win[MAP_NUM])
	delwin(g_data_win[MAP_NUM]);
    if (g_data_win[FILE_NUM])
	delwin(g_data_win[FILE_NUM]);

    g_data_win[MAP_NUM] = 0;
    g_data_win[FILE_NUM] = 0;

    row = 1;
    col = 0;

    g_num_col = g_num_col % 3;	/* 0, 1 or 2 only */

    switch (g_num_col)
    {
	case NO_FILE :
	    g_cur_display = DISPLAY_MAP;
	    g_data_width[MAP_NUM] = COLS;
	    break;
	case HALF_FILE :
	    g_cur_display = DISPLAY_FILE;
	    g_data_width[MAP_NUM] = COLS/2;
	    break;
	case ALL_FILE :
	    g_cur_display = DISPLAY_FILE;
	    g_data_width[MAP_NUM] = 0;
	    break;
    }
    g_data_width[FILE_NUM] = COLS-g_data_width[MAP_NUM];

    if( g_num_col != ALL_FILE )
    {
	g_data_win[MAP_NUM] = newwin(g_data_height, g_data_width[MAP_NUM], 
		row, col);
	assert(g_data_win[MAP_NUM]);
    }

    /* Must always have a file window for info calls to display to,
     * Even if we don't show it.
     */
    g_data_win[FILE_NUM] = newwin(g_data_height, g_data_width[FILE_NUM], 
	    row, col+g_data_width[MAP_NUM]);

    if( g_num_col != NO_FILE )
    {
	assert(g_data_win[FILE_NUM]);
    }


    if (old_num_col != g_num_col)
        PlanetMapResized();

    ScreenRedraw();

    return DONE;
}

WINDOW *MapWin( void )
{
    return(g_data_win[MAP_NUM]);
}

WINDOW *FileWin( void )
{
    return(g_data_win[FILE_NUM]);
}

int ScreenDataWinHeight (void)
{
    return g_data_height;
}

int MapWinWidth (void)
{
    return g_data_width[MAP_NUM];
}

int FileWinWidth (void)
{
    return g_data_width[FILE_NUM];
}

int ScreenPlaceCursor (void)
{
    switch (g_cur_display)
    {
	case DISPLAY_MAP :
	    PlanetPlaceCursor();
	    break;
	case DISPLAY_FILE :
	    FilePlaceCursor();
	    break;
    }
    return DONE;
}

void ScreenTerm (void)
{
    if (!gScr.flag.inited)
	return;

    clear();
    refresh();
    endwin();
    printf("\n");
}

void ScreenRedraw (void)
{
    ScreenShowMenuBar();
    MsgRedisplay();
    if( g_num_col != NO_FILE )
	FileRedisplay();
    if( g_num_col != ALL_FILE )
	PlanetRedisplay();
    ScreenPlaceCursor();
}

/*
 * Purpose:
 *
 *   Examine the passed inputted character against
 *   all registered callbacks.
 */
void ScreenMainLoop (void)
{
    int chr;

    /* Some curses don't have ungetch.
	Fortunately, these curse also don't appear to blank the screen
	on the first getch call.
     */
#ifdef WRONG_CURSES
    ScreenProcessAccl(REDRAW);
    (*gScr.def.func)(0, REDRAW, gScr.def.arg);
#else
    ungetch( REDRAW );
#endif

    while (1)
    {
	chr = getch();

	if (ScreenProcessAccl(chr))
	    continue;

	if (gScr.def.func)
	    (*gScr.def.func)(0, chr, gScr.def.arg);
    }
}

static int ScreenProcessAccl (int i_chr)
{
    int num;

    num = ScreenAcclNewChr(i_chr);
    /*
     *  Did we get a pulldown menu?
     */
    if (ScreenChkPulldown(i_chr))
	return TRUE;

    if (ScreenChkItem(i_chr))
	return TRUE;

    if (!ScreenAcclPossible())
    {
	ScreenAcclClear();
	/*
	 * If there was something in the accleration
	 * string before we started this function try
	 * matching the above character again to see if
	 * we can match another acceleration.
	 */
	if (num > 1)
	    return ScreenProcessAccl(i_chr);
    }
    return FALSE;
}

void ScreenSetDefaultCallback (int (*I_func)(), int arg)
{
    gScr.def.func = I_func;
    gScr.def.arg = arg;
}

void ScreenSetRedrawCallback (int (*I_func)(), int arg)
{
    gScr.redraw.func = I_func;
    gScr.redraw.arg = arg;
}

void ScreenCallRedraw (void)
{
    if (*gScr.redraw.func != 0)
	(*gScr.redraw.func)(gScr.redraw.arg);
}

int ScreenJumpWorld (int i_ymod, int i_xmod)
{
    switch (g_cur_display)
    {
	case DISPLAY_MAP  : return PlanetJumpWorld(i_ymod, i_xmod);
	case DISPLAY_FILE : return FileJumpWorld(i_ymod, i_xmod);
    }
    return NOT_OKAY;
}


/*  Move the cursor inside the grid */
int ScreenCursorMove (int i_ymod, int i_xmod)
{
    switch (g_cur_display)
    {
	case DISPLAY_MAP  : return PlanetCursorMove(i_ymod, i_xmod);
	case DISPLAY_FILE : return FileCursorMove(i_ymod, i_xmod);
    }
    return NOT_OKAY;
}

/*  Move the cursor inside the grid */
int ScreenCursorJump (int i_ymod, int i_xmod)
{
    switch (g_cur_display)
    {
	case DISPLAY_MAP  : return PlanetCursorJump(i_ymod, i_xmod);
	case DISPLAY_FILE : return FileCursorJump(i_ymod, i_xmod);
    }
    return NOT_OKAY;
}
