/*
 * File: msg.c
 * Author: Douglas Selph
 * Maintained by: Robin Powell
 * $Id: msg.c,v 1.3 1997/12/11 00:43:44 rlpowell Exp $
 * Purpose:
 *
 *   Provide a io interface using 'curses'.
 */
#include <stdio.h>
#include <curses.h>
#include <unistd.h>
#include "common.h"
#include "screen.h"

#include "util.h"
#include "fileshow.h"
#include "msg.h"

/**
 **  Local Structures and Defines
 **/
#define MSGWIN_LO_ROW  (LINES-1)
#define MSGWIN_LO_COL  0
#define MSGWIN_WIDTH   COLS
#define MSGWIN_HEIGHT  1

static struct screen_data
{
    WINDOW *win;
} gMsg;

extern char g_msg[MAX_MSG];

static int MsgSave (char *cp_msg);

/**
 ** Functions:
 **/
/* static int read_next_line(); ??? never used */

/*
 *  Intialize the screen
 */
int MsgInit (void)
{
    unlink(MSG_FILE);
    gMsg.win = newwin(MSGWIN_HEIGHT, MSGWIN_WIDTH,
	    MSGWIN_LO_ROW, MSGWIN_LO_COL);
    return DONE;
}

int MsgDisplay (char *cp_msg)
{
    MsgSave(cp_msg);

    werase(gMsg.win);
    mvwaddstr(gMsg.win, 0, 0, cp_msg);
    wrefresh(gMsg.win);

    ScreenPlaceCursor();

    return DONE;
}

int MsgRedisplay (void)
{
    touchwin(gMsg.win);
    wrefresh(gMsg.win);
    return DONE;
}

static int MsgSave (char *cp_msg)
{
    FILE *fp;

    if ((fp = fopen(MSG_FILE, "a")) == NULL)
	return NOT_OKAY;

    strRmReturn(cp_msg);
    fprintf(fp, "%s\n", cp_msg);
    fclose(fp);

    FileChkRedisplay(MSG_FILE);

    return DONE;
}
