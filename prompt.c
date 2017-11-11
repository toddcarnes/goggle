/*
 * File: prompt.c
 * Author: Douglas Selph
 * Maintainer: Robin Powell
 * $Id: prompt.c,v 1.3 1997/12/11 00:43:44 rlpowell Exp $
 */
#include <assert.h>
#include <ctype.h>
#include <string.h>
#include <curses.h>
/* #include <varargs.h> */
#include <stdlib.h>
#include <math.h>
#include "common.h"
#include "screen.h"

#include "util.h"
#include "main.h"
#include "prompt.h"

/**
 **  Local Structures and Defines
 **/
#define FIELD_LEN        20
#define MAX_MSGS         10

#define OK_WORD           "Ok "
#define CANCEL_WORD       "Cancel"

#define TRUE_WORD         "TRUE "
#define FALSE_WORD        "FALSE"

#define WITH_OK           1
#define WITH_CANCEL       2
#define IS_ONE_OF_MANY    4
#define WITH_SCROLL_ROW   8
#define OK_COL            2
#define CANCEL_COL        (OK_COL + strlen(OK_WORD) + 1)

extern char g_msg[MAX_MSG];
static unsigned char g_with_scroll_row;
static int g_actual_num;
/*
 *  Can be PR_NO_OPTIONS,
 *   PR_START_ROW 	-- set initial cursor placement.
 */
int g_prompt_flags = 0;
int g_prompt_start_row = 0;

static void ScreenDrawPromptWin (WINDOW *promptwin, PromptObj *sp_list,
	int i_num, int entercol);
static int Parse_Result (PromptObj *sp_list, int i_num, WINDOW *promptwin,
	int enterrow, int entercol);
static int ScreenMsg (char *mess, char *nothing);

/**
 ** Functions:
 **/

/*
 * Purpose:
 *
 *   Make a multiline prompt dialog.
 *    The prompt list passed will be placed in the
 *    dialog in the order given.  The i_field is the
 *    number of characters allowed in each field.
 *    Passed back is the upper-left corner of the first
 *    field.
 *
 *  WITH_OK or WITH_CANCEL will allow cause the window
 *   to have an extra row which has 'Ok' and 'Cancel' on it.
 *  IS_ONE_OF_MANY will cause this to bea special prompt
 *   dialog that is a selection from many choices.  cp_special
 *   will be used as the extra prompt message.
 */
static WINDOW *ScreenMakePromptWin (char **cpp_prompts, int i_num, int i_field, char *cp_special, int *ip_enterrow, int *ip_entercol, int i_flags)
{
    WINDOW *promptwin;
    char text[160];
    int numrows, numcols;
    int startrow, startcol;
    int maxlen, i;
    int row, col, num, len;
    Boolean with_ok, with_cancel, okcancel, oneofmany;

    with_ok = OPT_IS_SET(i_flags, WITH_OK);
    with_cancel = OPT_IS_SET(i_flags, WITH_CANCEL);
    okcancel = (with_ok || with_cancel);
    oneofmany = OPT_IS_SET(i_flags, IS_ONE_OF_MANY);
    g_with_scroll_row = 0;

    maxlen = strLongestLen(cpp_prompts, i_num);
    num = i_num;
    if (cp_special && ((len = strlen(cp_special)) > maxlen))
	maxlen = len;

    numrows = 2 + i_num + (okcancel ? 2 : 0) + (oneofmany ? 1 : 0);
    numcols = maxlen + i_field + 4 + (oneofmany ? 4 : 0);

    if (numrows >= LINES-2)
    {
	if (!g_with_scroll_row)
	    return NULL;

	/* note: extra '-1' is to account for additional prompt */
	num = LINES - 2 - (okcancel ? 2 : 0) - 1;
	g_actual_num = num;
	numrows = LINES;
	g_with_scroll_row = TRUE;
    }
    /* the no_err flag is really only for exceeding the #rows */
    if (numcols > COLS)
    {
	sprintf(g_msg,
		"Error: requested a %d column prompt dialog, when only %d columns on screen.", numcols, COLS);
	message_print(g_msg);
	return NULL;
    }
    startrow = LINES/2 - numrows/2 + 1;
    startcol = COLS/2 - numcols/2 - 1;

    if (startrow < 0)
    {
	num = LINES - 2 - (okcancel ? 2 : 0) - 1;
	numrows = LINES - 1;
	startrow = 0;
    }
    else
	num = i_num;

    promptwin = newwin(numrows, numcols, startrow, startcol);

    box(promptwin, '|', '-');

    if (oneofmany)
    {
	for (i = 0; i < num; i++)
	{
	    sprintf(text, "(%c) %s", (char) ('a' + i), cpp_prompts[i]);
	    mvwaddstr(promptwin, i+1, 2, text);
	}
	if (g_with_scroll_row)
	{
	    sprintf(text, "(%c) -- Next Set Of Prompts --", (char) ('a' + i));
	    g_with_scroll_row = ('a' + i);
	    i++;
	}
	if (cp_special)
	    mvwaddstr(promptwin, i+1, 2, cp_special);
    }
    else
	for (i = 0; i < num; i++)
	    mvwaddstr(promptwin, i+1, 2, cpp_prompts[i]);
    *ip_enterrow = 1;
    *ip_entercol = maxlen + 3;

    if (okcancel)
    {
	row = num+1;
	for (col = 1; col < numcols-2; col++)
	{
	    wmove(promptwin, num+1, col);
	    waddch(promptwin, '-');
	}
	row = num+2;
	text[0] = '\0';

	if (with_ok)
	    sprintf(text, "%s ", OK_WORD);

	if (with_cancel)
	    strcat(text, CANCEL_WORD);

	mvwaddstr(promptwin, row, 2, text);
    }
    wrefresh(promptwin);

    return promptwin;
}

/*
 * Purpose:
 *
 *   Prompt the user for a (or lots) of values.
 *
 *   If not NULL I_callback will be called each time
 *   a row change occurs (through return or tab).
 *   It will be called like this:
 *	(*I_callback)(sp_list, i_num, i_row)
 *
 *	i_row is the current row # the cursor is on.
 *
 * Returns:
 *
 *   DONE if got the value.
 *   ABORT if user canceled.
 *   BAD_VALUE if bad value entered.
 */
int ScreenGetMany (PromptObj *sp_list, int i_num, int (*I_callback)())
{
    WINDOW *promptwin;
    char **prompts;
    char savetext[80];
    char chr, testch;
    int entercol, enterrow, menurow;
    int curcol, currow;
    int err = DONE;
    int type, redraw;
    int set_row = OPT_IS_SET(g_prompt_flags, PR_START_ROW);
    int i;

    /* reset */
    g_prompt_flags = PR_NO_OPTIONS;

    prompts = (char **) CALLOC(i_num, sizeof(char *));
    for (i = 0; i < i_num; i++)
	if (sp_list[i].type == GM_LABEL)
	    prompts[i] = " ";
	else
	    prompts[i] = sp_list[i].prompt;
    promptwin = ScreenMakePromptWin(prompts, i_num, FIELD_LEN, 0,
	    &enterrow, &entercol, WITH_OK + WITH_CANCEL);

    free(prompts);
    if (promptwin == NULL)
	return NOT_OKAY;

    ScreenDrawPromptWin(promptwin, sp_list, i_num, entercol);
    /*
     * ESCAPE = abort
     * return = enter value and move to next field.
     * tab = move to next field and abort current value.
     */
    menurow = enterrow + i_num + 1;

    if (set_row)
    {
	if ((g_prompt_start_row >= menurow) || (g_prompt_start_row < 0))
	{
	    currow = menurow;
	    curcol = OK_COL;
	}
	else
	{
	    currow = g_prompt_start_row;
	    curcol = entercol;
	}
    }
    else
    {
	currow = enterrow;
	curcol = entercol;
    }
    my_mvwinstr(promptwin, currow, curcol, FIELD_LEN, savetext);

    while (1)
    {
	/*
	 *  Get type of current row
	 */
	if ((currow >= enterrow) && (currow < enterrow + i_num))
	    type = sp_list[currow - enterrow].type;
	else
	    type = GM_UNDEFINED;
	/*
	 *  If current row is a label, then do auto next.
	 */
	if (type == GM_LABEL)
	    chr = '\n';
	else
	{
	    wmove(promptwin, currow, curcol);
	    wrefresh(promptwin);
	    chr = wgetch(promptwin);
	}
	if (chr == ESCAPE)	/* ESCAPE -- CANCEL */
	{
	    err = ABORT;
	    break;
	}
	else if (chr == CONTROL('d'))	/* OK */
	{
	    err = DONE;
	    break;
	}
	else if (((chr == NEXT_FIELD) || (chr == '\n')) ||
		((IsBackSpace(chr)) && (curcol == entercol)))
	{
	    if (currow == menurow)
	    {
		if (chr == NEXT_FIELD)
		{
		    currow = enterrow;
		    curcol = entercol;
		}
		else
		{
		    if (curcol >= CANCEL_COL)
			err = ABORT;
		    break;
		}
	    }
	    else if ((IsBackSpace(chr)) && (curcol == entercol))
	    {
		currow--;
		curcol = entercol;

		if (currow < enterrow)
		{
		    currow = menurow;
		    curcol = OK_COL;
		}
		wmove(promptwin, currow, curcol);
		wrefresh(promptwin);
	    }
	    else
	    {
		/* abort current field */
		if (chr == NEXT_FIELD)
		    mvwaddstr(promptwin, currow, entercol, savetext);

		currow++;
		curcol = entercol;

		if (currow >= enterrow + i_num)
		{
		    currow = menurow;
		    curcol = OK_COL;
		}
	    }
	    /* save next field */
	    my_mvwinstr(promptwin, currow, entercol, FIELD_LEN, savetext);

	    /* call callback */
	    if (I_callback)
	    {
		if (Parse_Result(sp_list, i_num, promptwin, enterrow, entercol)
			== DONE)
		{
		    redraw = FALSE;

		    (*I_callback)(sp_list, i_num, currow, &redraw);

		    if (redraw)
			ScreenDrawPromptWin(promptwin, sp_list, i_num, entercol);
		}
		touchwin(promptwin);
		wrefresh(promptwin);
	    }
	}
	else if (currow == menurow)
	{
	    if (IsBackSpace(chr))
	    {
		currow -= 2;
		curcol = entercol;
	    }
	    else if (curcol < CANCEL_COL)
		curcol = CANCEL_COL;
	    else
		curcol = OK_COL;
	}
	else if (type == GM_LOGICAL)
	{
	    if ((chr == 't') || (chr == 'T'))
		mvwaddstr(promptwin, currow, curcol, TRUE_WORD);
	    else if ((chr == 'f') || (chr == 'F'))
		mvwaddstr(promptwin, currow, curcol, FALSE_WORD);
	    else if (chr == ' ')	/* toggle */
	    {
		testch = mvwinch(promptwin, currow, entercol);

		if (testch == TRUE_WORD[0])
		    mvwaddstr(promptwin, currow, curcol, FALSE_WORD);
		else
		    mvwaddstr(promptwin, currow, curcol, TRUE_WORD);
	    }
	}
	else if (IsBackSpace(chr))
	{
	    if (curcol > entercol)
	    {
		mvwaddch(promptwin, currow, curcol, ' ');
		curcol--;
		mvwaddch(promptwin, currow, curcol, ' ');
	    }
	}
	else if (isprint(chr))
	{
	    if (curcol == entercol)
	    {
		/* auto erase of line */
		for (i = entercol; i < entercol + FIELD_LEN - 1; i++)
		    mvwaddch(promptwin, currow, i, ' ');
	    }
	    if (curcol < entercol + FIELD_LEN)
	    {
		mvwaddch(promptwin, currow, curcol, chr);
		curcol++;
	    }
	}
    }
    /*
     *  Parse result.
     */
    if (err == DONE)
	err = Parse_Result(sp_list, i_num, promptwin, enterrow, entercol);
    delwin(promptwin);

    ScreenRedraw();

    return err;
}

static void ScreenDrawPromptWin (WINDOW *promptwin, PromptObj *sp_list,
	int i_num, int entercol)
{
    char text[120];
    int i;

    /*
     *  Fill prompt window with current values
     */
    for (i = 0; i < i_num; i++)
    {
	switch (sp_list[i].type)
	{
	    case GM_STRING :
		strncpy(text, sp_list[i].u.sval, FIELD_LEN);
		text[FIELD_LEN+1] = '\0';
		break;
	    case GM_DOUBLE :
		sprintf(text, "%.2g", sp_list[i].u.dval);
		break;
	    case GM_FLOAT :
		sprintf(text, "%.2f", sp_list[i].u.fval);
		break;
	    case GM_INTEGER :
		sprintf(text, "%d", sp_list[i].u.ival);
		break;
	    case GM_LOGICAL :
		if (sp_list[i].u.lval)
		    strcpy(text, TRUE_WORD);
		else
		    strcpy(text, FALSE_WORD);
		break;
	    case GM_LABEL :
		strcpy(text, sp_list[i].prompt);
		break;
	    default :
		assert(0);
	}
	if (sp_list[i].type == GM_LABEL)
	    mvwaddstr(promptwin, i+1, 2, text);
	else
	    mvwaddstr(promptwin, i+1, entercol, text);
    }
    wrefresh(promptwin);
}

static int Parse_Result (PromptObj *sp_list, int i_num, WINDOW *promptwin,
	int enterrow, int entercol)
{
    char text[120];
    int err = DONE;
    int reset;
    int redraw;
    int i;

    for (i = 0; i < i_num; i++)
    {
	my_mvwinstr(promptwin, enterrow+i, entercol, FIELD_LEN, text);
	strStripSpaces(text);

	reset = FALSE;
	redraw = FALSE;

	switch (sp_list[i].type)
	{
	    case GM_STRING :
		strcpy(sp_list[i].u.sval, text);
		break;
	    case GM_DOUBLE :
		if (strIsDouble(text))
		    sp_list[i].u.dval = atof(text);
		else
		{
		    sprintf(g_msg, "Bad Float: %s", text);
		    ScreenMsg(g_msg, 0);
		    strcpy(text, "1.0");
		    reset = TRUE;
		    redraw = TRUE;
		    err = BAD_VALUE;
		}
		break;
	    case GM_FLOAT :
		if (strIsDouble(text))
		    sp_list[i].u.fval = (float) atof(text);
		else
		{
		    sprintf(g_msg, "Bad Float: %s", text);
		    ScreenMsg(g_msg, 0);
		    strcpy(text, "1.0");
		    reset = TRUE;
		    redraw = TRUE;
		    err = BAD_VALUE;
		}
		break;
	    case GM_INTEGER :
		if (strIsInt(text))
		    sp_list[i].u.ival = atoi(text);
		else
		{
		    sprintf(g_msg, "Bad Integer: %s", text);
		    ScreenMsg(g_msg, 0);
		    strcpy(text, "1");
		    reset = TRUE;
		    redraw = TRUE;
		    err = BAD_VALUE;
		}
		break;
	    case GM_LOGICAL :
		if (text[0] == TRUE_WORD[0])
		    sp_list[i].u.lval = TRUE;
		else
		    sp_list[i].u.lval = FALSE;
		break;
	    case GM_LABEL :
		break;
	    default :
		assert(0);
	}
	if (reset)
	{
	    wmove(promptwin, enterrow+i, entercol);
	    wclrtoeol(promptwin);
	    mvwaddstr(promptwin, enterrow+i, entercol, text);
	    box(promptwin, '|', '-');
	    wrefresh(promptwin);
	}
	if (redraw)
	{
	    touchwin(promptwin);
	    wrefresh(promptwin);
	}
    }
    return err;
}

/*
 * Purpose:
 *
 *   Display the passed message(s).
 *
 * Returns:
 *
 *   DONE if got the value.
 *   ABORT if user canceled.
 *   BAD_VALUE if bad value entered.
 */
#if 0
static int ScreenMsg(va_alist)
    va_dcl
    /* int ScreenMsg (...) */
{
    va_list ap;
    WINDOW *promptwin;
    int entercol, enterrow, menurow, num;
    char *msgs[MAX_MSGS], *msg;
    char chr;

    num = 0;

    va_start(ap);

    while (1)
    {
	msg = va_arg(ap, char *);
	if (msg == 0)
	    break;
	num++;
	assert(num <= MAX_MSGS);
	msgs[num-1] = msg;
    }
    va_end(ap);

    promptwin = ScreenMakePromptWin(msgs, num, 0, 0,
	    &enterrow, &entercol, WITH_OK);

    if (promptwin == NULL)
	return NOT_OKAY;

    menurow = enterrow + num + 1;
    wmove(promptwin, menurow, OK_COL);
    wrefresh(promptwin);

    while (1)
    {
	chr = getch();

	if (chr == '\n')
	    break;
    }
    delwin(promptwin);

    noecho();

    ScreenRedraw();

    return DONE;
}
#else
static int ScreenMsg (char *mess, char *nothing)
{
    WINDOW *promptwin;
    int entercol, enterrow, menurow, num;
    char *msgs[MAX_MSGS];
    char chr;

    assert(nothing == 0);

    msgs[0] = mess;
    num = 1;

    promptwin = ScreenMakePromptWin(msgs, num, 0, 0,
	    &enterrow, &entercol, WITH_OK);

    if (promptwin == NULL)
	return NOT_OKAY;

    menurow = enterrow + num + 1;
    wmove(promptwin, menurow, OK_COL);
    wrefresh(promptwin);

    while (1)
    {
	chr = getch();

	if (chr == '\n')
	    break;
    }
    delwin(promptwin);

    noecho();

    ScreenRedraw();

    return DONE;
}
#endif

/*
 * Curses functions that should exist
 */
int my_mvwinstr (WINDOW *win, int y, int x, int num, char *str)
    /* num	number of characters to retrieve */
{
    int i;

    for (i = 0; i < num; i++)
    {
	wmove(win,y,x+i);
	str[i] = winch(win);
    }
    str[i] = '\0';
    return num;
}

/*
 * Purpose:
 *
 *   Prompt the user to select an item from a list.
 *   Note: cp_special is the special prompt for
 *    this window to placed at the bottom indicating
 *    to the user what to do.
 *
 * Returns:
 *
 *   >= 0 if got the value.
 *   ABORT if user canceled.
 *   BAD_VALUE if bad value entered.
 */
int ScreenGetOneOfMany (char **cpp_prompts, int i_num, char *cp_special)
{
    WINDOW *promptwin;
    char chr, botchr;
    char text[160];
    char **prompts;
    int entercol, enterrow, num;
    int err = DONE;
    int i;

    prompts = cpp_prompts;
    num = i_num;

TRY_AGAIN :

    promptwin = ScreenMakePromptWin(prompts, num, 0, cp_special,
	    &enterrow, &entercol,
	    IS_ONE_OF_MANY | WITH_SCROLL_ROW);

    /* if NULL then we we have too many columns for the screen */
    if (promptwin == NULL)
	return 0;  /* ??? was NULL */

    botchr = 'a' + num;

    wrefresh(promptwin);
    /*
     * ESCAPE = abort
     * letter = selects choice.
     */
    while (1)
    {
	chr = getch();

	if (chr == ESCAPE)	/* ESCAPE -- CANCEL */
	{
	    err = ABORT;
	    break;
	}
	else if (g_with_scroll_row && (chr == g_with_scroll_row))
	{
	    delwin(promptwin);
	    touchwin(stdscr);
	    if (prompts == cpp_prompts)
	    {
		prompts = cpp_prompts + g_actual_num;
		num -= g_actual_num;
		assert(num > 0);
	    }
	    else
	    {
		prompts = cpp_prompts;
		num = i_num;
	    }
	    goto TRY_AGAIN;
	}
	else if (chr < 'a' || chr > botchr)
	{
	    sprintf(text, "Please enter in a letter from 'a' to '%c' or ESCAPE to abort",
		    botchr);
	    ScreenMsg(text, 0);
	    touchwin(promptwin);
	    wrefresh(promptwin);
	}
	else
	{
	    i = chr - 'a';
	    err = i;
	    break;
	}
    }
    delwin(promptwin);

    ScreenRedraw();

    return err;
}
