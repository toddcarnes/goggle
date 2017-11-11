/*
 * File: menu.c
 * Author: Douglas Selph
 * Maintained by: Robin Powell
 * $Id: menu.c,v 1.3 1997/12/11 00:43:44 rlpowell Exp $
 */
#include <stdio.h>
#include <string.h>
#include <curses.h>
#include "common.h"
#include "screen.h"
#include "menu.h"

/**
 **  Local Structures and Defines
 **/

#define PREV		30
#define NEXT		31
#define SELECTED	32

#define MENU_ROW 0

static struct menu_global
{
    WINDOW *menubar;
    itemList list;
    char activate[5];	/* current string to implement accelerator strings */
} Menu;

void ScreenShowMenuBar (void);
static int ScreenAcclChk (char *cp_accl);
static int ScreenActivatePulldown (int i_which);
static int ScreenProcessPulldown (WINDOW *sp_pulldown, int i_numrows, int *ip_row);
static int ScreenShowPulldown (int i_which, WINDOW **sp_pulldown, int *ip_numrows);


/**
 ** Functions:
 **/

int ScreenMenuInit (itemList sp_itemlist)
{
    Menu.list = sp_itemlist;
    ScreenShowMenuBar();
    Menu.activate[0] = '\0';
    return DONE;
}

void ScreenShowMenuBar (void)
{
    int i;
    char text[150];

    strcpy(text, " ");

    for (i = 0; Menu.list[i].name != 0; i++)
	if (Menu.list[i].is_pulldown)
	{
	    strcat(text, Menu.list[i].name);
	    strcat(text, " ");
	}
    Menu.menubar = newwin(1, COLS, 0, 0);
    mvwaddstr(Menu.menubar, 0, 0, text);

    wrefresh(Menu.menubar);
}

int ScreenChkPulldown (int i_chr)
{
    int i;
    int ret = FALSE;
    int anyflag = FALSE;
    int err;
    int pull_list[20];
    int maxpull;
    int cur;

    if (i_chr == (int) '\t') /* automatic pulldown activate */
    {
	ScreenAcclClear();
	anyflag = TRUE;
    }
    /*
     *  Calculate the # of pulldowns we have (maxpull)
     *  as well as their item list position (pull_list).
     */
    maxpull = 0;

    for (i = 0; Menu.list[i].name != 0; i++)
	if (Menu.list[i].is_pulldown)
	    pull_list[maxpull++] = i;

    for (i = 0; i < maxpull; i++)
    {
	cur = pull_list[i];

	/* pulldown activated? */
	if (ScreenAcclChk(Menu.list[cur].accl) || anyflag)
	{
	    ret = TRUE;

	    err = ScreenActivatePulldown(cur);

	    if (err == PREV)
	    {
		if (i == 0)
		    return ABORT;

		i -= 2;
	    }
	    else if (err != NEXT)
		break;

	    anyflag = TRUE;

	    ScreenAcclClear();
	}
    }
    return ret;
}

/*
 *  Return TRUE if something was cleared.
 *  Return FALSE if nothing to clear.
 */
int ScreenAcclClear (void)
{
    Menu.activate[0] = '\0';
    return DONE;
}

/* Returns the size of the resultant acceleration string */
int ScreenAcclNewChr (int i_chr)
{
    int len;

    if ((len = strlen(Menu.activate)) >= sizeof(Menu.activate)-1)
    {
	ScreenAcclClear();
	len = 0;
    }
    Menu.activate[len++] = i_chr;
    Menu.activate[len++] = '\0';

    return strlen(Menu.activate);
}

/*
 *  Returns TRUE if the passed acceleration string
 *  matches what is currently in our little buffer.
 */
static int ScreenAcclChk (char *cp_accl)
{
    return cp_accl && !strcmp(cp_accl, Menu.activate);
}

/*
 * Returns TRUE if the current activate string could
 * match an accelerator code if given more inputs.
 */
int ScreenAcclPossible (void)
{
    int i, len;

    if ((len = strlen(Menu.activate)) > 0)
	for (i = 0; Menu.list[i].name != 0; i++)
	    if (Menu.list[i].accl)
		if (!strncmp(Menu.list[i].accl, Menu.activate, len))
		    return TRUE;

    return FALSE;
}

int ScreenChkItem (int i_chr)
{
    int i;

    for (i = 0; Menu.list[i].name != 0; i++)
	if ((Menu.list[i].callback != 0) && ScreenAcclChk(Menu.list[i].accl))
	{
	    (*(Menu.list[i].callback))(Menu.list[i].id, i_chr, Menu.list[i].arg);
	    ScreenAcclClear();
	    return TRUE;
	}
    return FALSE;
}

static int ScreenActivatePulldown (int i_which)
{
    WINDOW *pulldown;
    int cur_top;
    int err;
    int numrows;
    int i, row, trow;

    err = ScreenShowPulldown(i_which, &pulldown, &numrows);

    if (err != DONE)
	return BAD_VALUE;

    err = ScreenProcessPulldown(pulldown, numrows, &row);

    delwin(pulldown);
    ScreenRedraw();

    if (err == SELECTED)
    {
	cur_top = Menu.list[i_which].top;
	trow = 0;

	for (i = 0; Menu.list[i].name != 0; i++)
	    if ((i != i_which) && (cur_top == Menu.list[i].top))
		if (++trow >= row)
		{
		    (*(Menu.list[i].callback))(Menu.list[i].id, 0, Menu.list[i].arg);
		    return DONE;
		}
    }
    return err;
}

static int ScreenProcessPulldown (WINDOW *sp_pulldown, int i_numrows, int *ip_row)
{
    char chr;

    *ip_row = 1;	/* Top item is row 1 */

    while (1)
    {
	chr = getch();
	/*
	 *  Accept <return>, j, or k, or 'h' or 'l'.
	 */
	if (chr == '\n')
	    break;
	if (chr == ESCAPE)
	    return ABORT;

	if ((chr == 'j') || (chr == ' '))
	{
	    if (*ip_row < i_numrows)
	    {
		(*ip_row)++;
		wmove(sp_pulldown, *ip_row, 1);
		wrefresh(sp_pulldown);
	    }
	    else if (chr == ' ')
	    {
		*ip_row = 1;
		wmove(sp_pulldown, *ip_row, 1);
		wrefresh(sp_pulldown);
	    }
	    else
		return ABORT;
	}
	else if (chr == 'k')
	{
	    if (*ip_row > 1)
	    {
		(*ip_row)--;
		wmove(sp_pulldown, *ip_row, 1);
		wrefresh(sp_pulldown);
	    }
	}
	else if ((chr == 'h') || (chr == CONTROL('h')))
	    return PREV;
	else if ((chr == 'l') || (chr == CONTROL('i')))
	    return NEXT;
    }
    return SELECTED;
}

static int ScreenShowPulldown (int i_which, WINDOW **sp_pulldown, int *ip_numrows)
{
    WINDOW *pulldown;
    char text[64];
    int count = 0;
    int cur_top = Menu.list[i_which].top;
    int len, maxlen;
    int startrow, startcol;
    int row;
    int i;

    startrow = MENU_ROW+1;
    startcol = 1;
    maxlen = 0;

    for (i = 0; Menu.list[i].name != 0; i++)
	if (i < i_which && Menu.list[i].is_pulldown)
	    startcol += strlen(Menu.list[i].name) + 1;
	else
	    if ((i != i_which) && (Menu.list[i].top == cur_top))
	    {
		len = strlen(Menu.list[i].name);

		if (Menu.list[i].accl)
		    len += strlen(Menu.list[i].accl);

		if (len > maxlen)
		    maxlen = len;

		count++;
	    }
    if (count == 0)
	return BAD_VALUE;

    *ip_numrows = count;
    *sp_pulldown =
	pulldown = newwin(count+2, maxlen+6, startrow, startcol);
    werase(pulldown);
    box(pulldown, '|', '-');
    row = 1;

    for (i = 0; Menu.list[i].name != 0; i++)
	if ((i != i_which) && (Menu.list[i].top == cur_top))
	{
	    if (Menu.list[i].accl)
		sprintf(text, "%s (%s)", Menu.list[i].name, Menu.list[i].accl);
	    else
		sprintf(text, "%s", Menu.list[i].name);

	    mvwaddstr(pulldown, row++, 1, text);
	}

    wmove(pulldown, 1, 1);
    wrefresh(pulldown);

    return DONE;
}
