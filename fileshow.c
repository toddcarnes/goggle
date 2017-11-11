/*
 * File: fileshow.c
 * Author: Douglas Selph
 * Maintained by: Robin Powell
 * $Id: fileshow.c,v 1.4 1997/12/11 00:43:44 rlpowell Exp rlpowell $
 * Purpose:
 *
 *   Provide a io interface using 'curses'.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curses.h>
/* ??? why does curses redefines this: */
#undef strchr
#include <ctype.h>
#include <assert.h>
#include "common.h"
#include "screen.h"

#include "main.h"
#include "util.h"
#include "errno.h"
#include "fileshow.h"

int FileDisplay (void);
static int FileApplySearchString (char *cp_search);
static int FileUpdateDisplay (void);
static int file_next_line (FILE *fp, char *cp_line, int i_size, char *cp_comment);
static int FilePlaceSideBars (int i_row);
static int extract_comment (char *cp_line, char *cp_comment);
static int update_read_vars (char *cp_line);
static int FileResetWorld (int i_newy, int i_newx);
static int FileSetCursor (int i_y, int i_x);

/**
 **  Local Structures and Defines
 **/
static struct file_show_data
{
    int cur;

    struct previous_files
    {
	char *name;		/* file to read in */
	char *title;	/* title to place at the top of the window */
	int id;		/* extra id identifying this file */
	struct line_buf
	{
	    char *buf;	/* actual line */
	    char *comment;	/* comment portion of line (holds simulator data) */
	    int length;	/* allocated space for line */
	} *lines;		/* array of saved lines */
	int numlines_alloced;
	int numlines_seen;
	int numlines_infile;
	int maxcol;
	long endpos;	/* lseek position of point just after what we can see */
	Position cursor, min, max, offset;
    } f[MAX_TMPS];
} gFile;

#define CUR_FILE gFile.f[gFile.cur]

extern char g_msg[MAX_MSG];
extern int g_num_col;
extern int old_num_col;
extern int g_insert_mode;
extern int g_cur_display;

/**
 ** Functions:
 **/

int FileInit (void)
{
    int i;

    for (i = 0; i < MAX_TMPS; i++)
    {
	gFile.f[i].cursor.r = 1;
	gFile.f[i].cursor.c = 1;
	gFile.f[i].offset.r = 0;
	gFile.f[i].offset.c = 0;
	gFile.f[i].lines = 0;
	gFile.f[i].numlines_alloced = 0;
	gFile.f[i].numlines_seen = 0;
	gFile.f[i].numlines_infile = 0;
	gFile.f[i].maxcol = 0;
	gFile.f[i].endpos = 0;
	gFile.f[i].min.r = 0;
	gFile.f[i].min.c = 0;
	gFile.f[i].max.r = 0;
	gFile.f[i].max.c = 0;
	gFile.f[i].name = 0;
	gFile.f[i].title = 0;
	gFile.f[i].id = 0;
    }
    gFile.cur = 0;

    return DONE;
}

int FileDelete (void)
{
    if (!CUR_FILE.name)
    {
	sprintf(g_msg, "No file currently displayed.");
	message_print(g_msg);
	return DONE;
    }
    zrealloc_cpy(&(CUR_FILE.name), 0);
    if (FileNextFile(1) != OKAY)
	FileDisplay();	/* will be erased automatically */
    return DONE;
}

/* Go to the specified file buffer */
int FileToFile (int i_which)
{
    if (i_which >= 0 && i_which < MAX_TMPS)
	if (gFile.f[i_which].name)
	{
	    gFile.cur = i_which;
	    return FileDisplay();
	}
    sprintf(g_msg, "There is nothing in file buffer %d", i_which);
    message_print(g_msg);

    return NOT_OKAY;
}

/* Move to the next file buffer */
int FileNextFile (int i_dir)
{
    FILE *fp;
    int i;

    for (i = 0; i < MAX_TMPS; i++)
    {
	if (i_dir > 0)
	    gFile.cur++;
	else
	    gFile.cur--;

	if (gFile.cur < 0)
	    gFile.cur = MAX_TMPS-1;
	else if (gFile.cur >= MAX_TMPS)
	    gFile.cur = 0;

	if (CUR_FILE.name)
	    if ((fp = fopen(CUR_FILE.name, "r")))
	    {
		fclose(fp);
		return FileDisplay();
	    }
    }
    return NOT_OKAY;
}

/*
 *  Show the indicated filename with the specified
 *  title.  The next available buffer is used by
 *  default.  First however the buffer with the
 *  same title and id is located, if it is found
 *  that buffer is reused.
 *
 *  cp_search will set the initial offset shown in the
 *  file so that the first line has the string of characters
 *  found in the cp_search string.  For example, if cp_search
 *  is '** KEYS **' then the first line shown will be the line
 *  containing '** KEYS ***' somewhere in it.
 */
int FileResetFilename (char *cp_filename, char *cp_title, int i_id, char *cp_search)
{
    int i;
    /*
     *  If we can locate title and id then re-used buffer.
     */
    for (i = 0; i < MAX_TMPS; i++)
    {
	if (gFile.f[i].name &&
		((gFile.f[i].title == cp_title) ||
		 (gFile.f[i].title && cp_title && !strcmp(gFile.f[i].title, cp_title))) &&
		(gFile.f[i].id == i_id))
	{
	    gFile.cur = i;
	    break;
	}
    }
    /*
     * We were unable to re-use a buffer, so look
     * for an unused one.
     */
    if (i == MAX_TMPS)
    {
	for (i = 0; i < MAX_TMPS; i++)
	{
	    if (++gFile.cur >= MAX_TMPS)
		gFile.cur = 0;

	    if (!CUR_FILE.name)
		break;
	}
	/*
	 * We were unable to find a used file, so reset
	 * to the next file after the currently set one
	 */
	if (i == MAX_TMPS)
	    if (++gFile.cur >= MAX_TMPS)
		gFile.cur = 0;
    }
    CUR_FILE.offset.r = 0;
    CUR_FILE.offset.c = 0;
    CUR_FILE.cursor.r = 1;
    CUR_FILE.cursor.c = 1;
    CUR_FILE.numlines_infile = 0;
    CUR_FILE.maxcol = 0;
    CUR_FILE.endpos = 0;
    CUR_FILE.min.r = 1;
    CUR_FILE.min.c = 1;
    CUR_FILE.max.r = DATAWIN_HEIGHT-2;
    /*
     *  I don't know why, but the dec platforms
     *  choke if we write too close to the edge
     *  of the window.
     */
#ifdef DEC
    CUR_FILE.max.c = FILEWIN_WIDTH-3;
#else
    CUR_FILE.max.c = FILEWIN_WIDTH-2;
#endif
    CUR_FILE.id = i_id;

    zrealloc_cpy(&(CUR_FILE.name), cp_filename);
    zrealloc_cpy(&(CUR_FILE.title), cp_title);

    FileApplySearchString(cp_search);

    if (FileDisplay() == DONE)
    {
	g_cur_display = DISPLAY_FILE;
	old_num_col = g_num_col;
	if( g_num_col == NO_FILE )
	{
	    g_num_col = ALL_FILE;
	    ScreenResize();
	}
    }
    else
    {
	g_cur_display = DISPLAY_MAP;
	if( g_num_col == ALL_FILE )
	{
	    g_num_col = NO_FILE;
	    ScreenResize();
	}
    }

    ScreenPlaceCursor();

    return DONE;
}

/*
 *  Sets CUR_FILE.offset.r according to cp_search.
 */
static int FileApplySearchString (char *cp_search)
{
    FILE *fp;
    char line[256];
    int row;

    if (!cp_search || !CUR_FILE.name)
	return DONE;

    if ((fp = fopen(CUR_FILE.name, "r")) == NULL)
	return IS_ERR;

    for (row = 0; fgets(line, sizeof(line), fp); row++)
    {
	if (strLocate(line, cp_search) >= 0)
	{
	    CUR_FILE.offset.r = row-1;
	    break;
	}
    }
    if (CUR_FILE.offset.r == 0)
    {
	sprintf(g_msg, "Search string '%s' not found in '%s'",
		cp_search, CUR_FILE.name);
	message_print(g_msg);
    }
    fclose(fp);

    return DONE;
}

int FileChkRedisplay (char *cp_file)
{
    if (CUR_FILE.name)
	if (!strcmp(cp_file, CUR_FILE.name))
	    FileRedisplay();

    return DONE;
}

int FileRedisplay (void)
{
    FileUpdateDisplay();
    return DONE;
}

int FileDisplay (void)
{
    FILE *fp;
    char line[256];
    char comment[64];
    int r, numsaved, i;

    werase(FILEWIN);

    if (CUR_FILE.name == 0)
    {
	wrefresh(FILEWIN);
	return IS_ERR;
    }
    if ((fp = fopen(CUR_FILE.name, "r")) == NULL)
    {
	errnoMsg(line, CUR_FILE.name);
	message_print(line);
	wrefresh(FILEWIN);
	return IS_ERR;
    }
    CUR_FILE.min.r = 1;
    CUR_FILE.min.c = 1;
    CUR_FILE.max.r = DATAWIN_HEIGHT-2;
    /*
     *  I don't know why, but the dec platforms
     *  choke if we write too close to the edge
     *  of the window.
     */
#ifdef DEC
    CUR_FILE.max.c = FILEWIN_WIDTH-3;
#else
    CUR_FILE.max.c = FILEWIN_WIDTH-2;
#endif

    if (CUR_FILE.offset.c < 0)
	CUR_FILE.offset.c = 0;

    CUR_FILE.numlines_infile = 0;
    CUR_FILE.maxcol = 0;
    /*
     *  Skip lines until we have reached the offset.
     */
    r = CUR_FILE.offset.r;

    while (r-- > 0)
	if (file_next_line(fp, line, sizeof(line), comment) != OKAY)
	    break;

    numsaved = CUR_FILE.max.r - CUR_FILE.min.r + 1;
    /*
     *  Free previous save buffer.
     */
    if (CUR_FILE.lines && (numsaved > CUR_FILE.numlines_alloced))
    {
	for (i = 0; i < CUR_FILE.numlines_alloced; i++)
	{
	    if (CUR_FILE.lines[i].buf)
		free(CUR_FILE.lines[i].buf);
	    if (CUR_FILE.lines[i].comment)
		free(CUR_FILE.lines[i].comment);
	}
	free(CUR_FILE.lines);

	CUR_FILE.lines = 0;
    }
    /*
     *  Allocate new buffer.
     */
    if (!CUR_FILE.lines)
    {
	CUR_FILE.numlines_alloced = numsaved;
	CUR_FILE.lines = (struct line_buf *)
	    CALLOC(CUR_FILE.numlines_alloced, sizeof(struct line_buf));
    }
    CUR_FILE.numlines_seen = 0;

    for (r = CUR_FILE.min.r, i = 0; r <= CUR_FILE.max.r; r++, i++)
    {
	if (file_next_line(fp, line, sizeof(line), comment) != OKAY)
	    break;

	/* save line */
	zrealloc_cpy(&(CUR_FILE.lines[i].buf), line);
	/* save comment */
	if (strlen(comment) > 0)
	    zrealloc_cpy(&(CUR_FILE.lines[i].comment), comment);
	else
	    zrealloc_cpy(&(CUR_FILE.lines[i].comment), NULL);

	/* remember allocated length of string */
	CUR_FILE.lines[i].length = strlen(CUR_FILE.lines[i].buf);

	/* display line */
	if (CUR_FILE.offset.c < strlen(line))
	{
	    line[CUR_FILE.offset.c+FILEWIN_WIDTH-2] = '\0';
	    mvwaddstr(FILEWIN, r, CUR_FILE.min.c, line + CUR_FILE.offset.c);
	}
	CUR_FILE.numlines_seen++;
    }
    /* free rest of lines if not yet freed */
    for (; i < CUR_FILE.numlines_alloced; i++)
    {
	if (CUR_FILE.lines[i].buf)
	{
	    free(CUR_FILE.lines[i].buf);
	    CUR_FILE.lines[i].buf = 0;
	    CUR_FILE.lines[i].length = 0;
	}
	if (CUR_FILE.lines[i].comment)
	{
	    free(CUR_FILE.lines[i].comment);
	    CUR_FILE.lines[i].comment = 0;
	}
    }
    /* remember this location */
    CUR_FILE.endpos = ftell(fp);

    FilePlaceSideBars(r);

    /* need to continue counting the number of lines in the file */
    while (file_next_line(fp, line, sizeof(line), comment) == OKAY)
	;

    fclose(fp);

    ScreenCallRedraw();

    wmove(FILEWIN, CUR_FILE.cursor.r, CUR_FILE.cursor.c);
    wrefresh(FILEWIN);

    return DONE;
}

/*
 *  Place top title bar, bottom bar, and side bars.
 *   if row is <= CUR_FILE.max.r then a bottom
 *   bar will be placed, otherwise it will not.
 */
static int FilePlaceSideBars (int i_row)
{
    char line[256];
    char name[128];
    int r, c, len, max;

    /*
     *  Get name we are going to display.
     *  Make sure this name is not too long.
     */
    if (CUR_FILE.title)
	strcpy(name, CUR_FILE.title);
    else
    {
	max = FILEWIN_WIDTH - 16;
	if ((len = strlen(CUR_FILE.name)) > max)
	    sprintf(name, "File: ..%s", CUR_FILE.name+len-max);
	else
	    sprintf(name, "File: %s", CUR_FILE.name);
    }
    /*
     *  Place Top Bar
     */
    for (c = 0; c < FILEWIN_WIDTH; c++)
	mvwaddch(FILEWIN, 0, c, '*');

    if (CUR_FILE.offset.r > 0)
	sprintf(line, "     %s (+%d)    ", name, CUR_FILE.offset.r);
    else
	sprintf(line, " %s ", name);
    mvwaddstr(FILEWIN, 0, FILEWIN_WIDTH/2 - strlen(line)/2 - 1, line);

    sprintf(line, "%d", gFile.cur);
    mvwaddstr(FILEWIN, 0, FILEWIN_WIDTH - strlen(line)/2 - 2, line);
    /*
     *  Place bottom bar
     */
    if (i_row <= CUR_FILE.max.r)
    {
	CUR_FILE.max.r = i_row-1;

	for (c = 0; c < FILEWIN_WIDTH; c++)
	    mvwaddch(FILEWIN, i_row, c, '*');
    }
    /*
     *  Place side bars
     */
    if (CUR_FILE.offset.c == 0)
	for (r = 0; r <= CUR_FILE.max.r+1; r++)
	    mvwaddch(FILEWIN, r, 0, '*');

    if (CUR_FILE.max.c - CUR_FILE.min.c + CUR_FILE.offset.c >= CUR_FILE.maxcol)
	for (r = 0; r <= CUR_FILE.max.r+1; r++)
	    mvwaddch(FILEWIN, r, CUR_FILE.max.c+1, '*');

    return DONE;
}

/*
 *  Get the next line in the file.
 *   Everything past '##' in the retrieved line treated
 *   as a comment.  This comment will be placed in the
 *   separate buffer cp_comment.  This was originally
 *   added to get additional information about the line
 *   to use for the battle simulator.
 */
static int file_next_line (FILE *fp, char *cp_line, int i_size, char *cp_comment)
{
    if (fgets(cp_line, i_size, fp) == 0)
	return IS_ERR;

    strRmReturn(cp_line);
    strExpandTab(cp_line);

    extract_comment(cp_line, cp_comment);

    update_read_vars(cp_line);

    return OKAY;
}

static int update_read_vars (char *cp_line)
{
    int len;

    CUR_FILE.numlines_infile++;

    if ((len = strlen(cp_line)) > CUR_FILE.maxcol)
	CUR_FILE.maxcol = len;

    return OKAY;
}

static int extract_comment (char *cp_line, char *cp_comment)
{
    char *s;

    *cp_comment = '\0';

    if ((s = strchr(cp_line, '#')))
	if (*(s+1) == '#')
	{
	    strcpy(cp_comment, s+2);
	    *s = '\0';
	}

    return DONE;
}


int FilePlaceCursor (void)
{
    wmove(FILEWIN, CUR_FILE.cursor.r, CUR_FILE.cursor.c);
    wrefresh(FILEWIN);
    return DONE;
}

/*
 *  Move the upper-left into the grid the following
 *  amount
 */
static int FileMoveWorld (int i_ymod, int i_xmod)
{
    return FileResetWorld(
	    CUR_FILE.offset.r + i_ymod,
	    CUR_FILE.offset.c + i_xmod);
}

/*
 *  Set the upper-left into the grid to following amount
 */
static int FileResetWorld (int i_newy, int i_newx)
{
    int r, c, w, h;

    r = i_newy;
    c = i_newx;
    w = CUR_FILE.max.c - CUR_FILE.min.c + 1;
    h = DATAWIN_HEIGHT-2;

    if (r < 0)
	r = 0;
    else if (r > CUR_FILE.numlines_infile - h + 1)
	r = CUR_FILE.numlines_infile - h + 1;

    if (c < 0)
	c = 0;
    else if (c > CUR_FILE.maxcol - w + 1)
	c = CUR_FILE.maxcol - w + 1;

    if (r != CUR_FILE.offset.r)
    {
	CUR_FILE.offset.r = r;
	CUR_FILE.offset.c = c;
	FileDisplay();
	return DONE;
    }
    /* Does not require a re-read */
    else if (c != CUR_FILE.offset.c)
    {
	CUR_FILE.offset.c = c;
	FileUpdateDisplay();
	return DONE;
    }
    return NOTHING;
}

/*  Offset the cursor inside the grid */
int FileCursorMove (int i_ymod, int i_xmod)
{
    return FileSetCursor(CUR_FILE.cursor.r + i_ymod,
	    CUR_FILE.cursor.c + i_xmod);
}

/*  Move the cursor inside the grid */
static int FileSetCursor (int i_y, int i_x)
{
    int r, c, d;

    r = i_y;
    c = i_x;

    if (r < CUR_FILE.min.r)
    {
	d = CUR_FILE.min.r - r;
	FileMoveWorld(-d, 0);
	r = CUR_FILE.min.r;
    }
    else if (r > CUR_FILE.max.r)
    {
	d = r - CUR_FILE.max.r;
	FileMoveWorld(d, 0);
	r = CUR_FILE.max.r;
    }
    if (c < CUR_FILE.min.c)
    {
	d = CUR_FILE.min.c - c;
	FileMoveWorld(0, -d);
	c = CUR_FILE.min.c;
    }
    else if (c > CUR_FILE.max.c)
    {
	d = c - CUR_FILE.max.c;
	FileMoveWorld(0, d);
	c = CUR_FILE.max.c;
    }
    wmove(FILEWIN, r, c);
    wrefresh(FILEWIN);

    CUR_FILE.cursor.r = r;
    CUR_FILE.cursor.c = c;

    return DONE;
}

int FileJumpWorld (int i_ymod, int i_xmod)
{
    int r, c;

    r = DATAWIN_HEIGHT/2;
    c = FILEWIN_WIDTH/2;

    return FileMoveWorld(r * i_ymod, c * i_xmod);
}

/*  Jump the cursor inside the grid */
int FileCursorJump (int i_ymod, int i_xmod)
{
    int r, c;

    r = CUR_FILE.cursor.r;
    c = CUR_FILE.cursor.c;

    if ((i_xmod == 0) && (i_ymod == 0))
    {
	r = DATAWIN_HEIGHT/2;
	c = FILEWIN_WIDTH/2;
    }
    else if (i_xmod > 0)
	c = CUR_FILE.max.c;
    else if (i_xmod < 0)
	c = CUR_FILE.min.c;
    else if (i_ymod > 0)
	r = CUR_FILE.max.r;
    else if (i_ymod < 0)
	r = CUR_FILE.min.r;
    if ((r != CUR_FILE.cursor.r) || (c != CUR_FILE.cursor.c))
    {
	CUR_FILE.cursor.r = r;
	CUR_FILE.cursor.c = c;
	FilePlaceCursor();
	return DONE;
    }
    return NOTHING;
}

/*
 *  Update the screen with the lines saved in our buffer.
 */
static int FileUpdateDisplay (void)
{
    int r, i;
    char line[256];

    werase(FILEWIN);
    touchwin(FILEWIN);

    if (CUR_FILE.numlines_alloced > 0)
    {
	CUR_FILE.max.r = DATAWIN_HEIGHT-2;
	CUR_FILE.maxcol = 0;
	CUR_FILE.numlines_seen = 0;

	for (r = CUR_FILE.min.r, i = 0;
		(r <= CUR_FILE.max.r) && (i < CUR_FILE.numlines_alloced) &&
		(CUR_FILE.lines[i].buf); r++, i++)
	{
	    strcpy(line, CUR_FILE.lines[i].buf);

	    update_read_vars(line);

	    /* display line */
	    if (CUR_FILE.offset.c < strlen(line))
	    {
		line[CUR_FILE.offset.c+FILEWIN_WIDTH-2] = '\0';
		mvwaddstr(FILEWIN, r, CUR_FILE.min.c, line + CUR_FILE.offset.c);
	    }
	    CUR_FILE.numlines_seen++;
	}
	FilePlaceSideBars(r);
    }
    wmove(FILEWIN, CUR_FILE.cursor.r, CUR_FILE.cursor.c);
    wrefresh(FILEWIN);

    return DONE;
}

/*
 *  Retrieve the comment part of the line
 *  in which the cursor is currently on.
 *  The 'comment' is unseen to the user.
 *  It was initially added to contain additional
 *  simulator information about the file.
 */
int FileRetrieveCommentAtCursor (char *cp_comment)
{
    int i;

    i = CUR_FILE.cursor.r - CUR_FILE.min.r;

    if (CUR_FILE.lines[i].comment)
	strcpy(cp_comment, CUR_FILE.lines[i].comment);
    else
    {
	*cp_comment = 0;
	return NOT_OKAY;
    }
    return OKAY;
}
