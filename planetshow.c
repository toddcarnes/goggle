/*
 * File: planetshow.c
 * Author: Douglas Selph
 * Maintained by: Robin Powell
 * $Id: planetshow.c,v 1.5 1997/12/11 00:43:44 rlpowell Exp rlpowell $
 */
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <curses.h>
#include "common.h"
#include "data.h"
#include "planet.h"
#include "screen.h"

#include "errno.h"
#include "main.h"
#include "prompt.h"
#include "util.h"
#include "lex.h"
#include "planetshow.h"

/**
 **  Local Structures and Defines
 **/
#define ROUND(F) (((F) - ((int) (F))) >= 0.5 ? ((int) (F)) + 1.0 : ((int) (F)))
#define CLIP(V) ((V) < 0 ? 0 : ((V) > g_map.max_width ? g_map.max_width : (V)))
#define POS(x,y)	((x)+((y)*(COLS+1)))

extern char g_msg[MAX_MSG];
extern MapData g_map;
extern int g_num_col;
extern int old_num_col;
extern int g_num_planets;

#define MAX_ZOOM     3

#define CUR_STARTX  g.zoom[g.cur_zoom].startx
#define CUR_STARTY  g.zoom[g.cur_zoom].starty
#define CUR_WIDTH   g.zoom[g.cur_zoom].width

static struct global_show_planet
{
    float center_x;
    float center_y;
    float markx;
    float marky;
    int numrows, numcols;
    int cur_zoom;
    int top_zoom;
    /* amount of real world per character space */
    float deltax, deltay;
    Position cursor, min, max;

    struct zoom_stack
    {
	float startx, starty, width;
    } zoom[MAX_ZOOM];
} g;

struct sorted
{
    float dist;
    int p;
};

static int read_zooms (void);
static int fill_map (char *map, int y, int x, int p, int option);
static int PlanetNewCoords (float startx, float starty, float width);
static int PlanetSetCoords (float startx, float starty, float width);
static int PlanetAt (int row, int col);
static int PlanetAtQuery (int row, int col, int i_flag);
static long PlanetWorldXToLoc(float x);
static long PlanetWorldYToLoc(float x);
static float PlanetLocToWorldX (int c);
static float PlanetLocToWorldY (int r);

/**
 ** Functions:
 **/

void init_planet_show (void)
{
    g.zoom[0].startx = 0;
    g.zoom[0].starty = 0;
    g.zoom[0].width = g_map.max_width;
    g.cur_zoom = 0;
    g.top_zoom = 0;
    g.marky = -1;
    g.markx = -1;
    g.cursor.r = DATAWIN_HEIGHT/2;
    g.cursor.c = MAPWIN_WIDTH/2;

    read_zooms();
}

/*
 *  The map was just resized (g_num_col has changed).
 *  So reset the cursor accordingly to maintain
 *  current position.
 */
void PlanetMapResized (void)
{
    if (old_num_col == HALF_FILE)	/* from 1 to X */
    {
	g.cursor.c *= 2;
    }
    if (g_num_col == HALF_FILE)	/* from X to 1 */
    {
	g.cursor.c /= 2;
    }
}

int PlanetRedisplay (void)
{
    if (MAPWIN)
    {
	touchwin(MAPWIN);
	PlanetShow();
    }
    return DONE;
}

int PlanetShow (void)
{
    float x1, y1, x2, y2;
    float width;
    char *map;
    int x, y, p, i;
    int pass_two;
    int numrows, numcols;

    if (!MAPWIN)	/* map window not visible! */
	return NOT_OKAY;

    width = CUR_WIDTH;

    x1 = CUR_STARTX;
    y1 = CUR_STARTY;
    x2 = x1 + width;
    y2 = y1 + width;

    numrows = DATAWIN_HEIGHT-4;
    if (numrows > LINES)
	numrows = LINES;
    numcols = MAPWIN_WIDTH;
    g.numrows = numrows;
    g.numcols = numcols;
    g.min.r = 2;
    g.min.c = 0;
    g.max.r = g.min.r + numrows - 1;
    g.max.c = numcols-1;

    g.center_x = x1 + width/2;
    g.center_y = y1 + width/2;

    /* Allocate the map space */
    map = calloc( 1, POS( COLS, LINES ) );

    /* display axis labels */
    werase(MAPWIN);
    /*
     *  Fill in character map.
     */
    for (y = 0; y < LINES; y++)
    {
	for (x = 0; x < COLS; x++)
	    map[POS(x,y)] = ' ';

	map[POS(x,y)] = '\0';
    }
    /*
     *  Pass 1: Fill map up with basics planet symbols.
     *  Pass 2: Overlay name, resources or sizes onto map.
     */
    pass_two = 0;
AGAIN:

    for (p = 0; p < g_num_planets; p++)
	if (planet[p].set)
	    if ((planet[p].x >= x1) && (planet[p].x <= x2) &&
		    (planet[p].y >= y1) && (planet[p].y <= y2))
	    {
		x = PlanetWorldXToLoc(planet[p].x) - g.min.c;
		y = PlanetWorldYToLoc(planet[p].y) - g.min.r;

		if ((x >= 0) && (x < numcols) && (y >= 0) && (y < numrows))
		    if (planet[p].who == WHO_ME)
			if (pass_two)
			    /* fill in planet # */
			    fill_map(map, y, x, p, g_map.option);
			else
			    map[POS(x,y)] = '*';
		    else if ((planet[p].who == WHO_ALIEN) ||
			    (planet[p].who == WHO_FRIEND))
		    {
			if (map[POS(x,y)] != '*')
			    if (pass_two)
				/* fill in planet # */
				fill_map(map, y, x, p, g_map.option);
			    else
				map[POS(x,y)] = '+';
		    }
		    else if (map[POS(x,y)] != '*' && map[POS(x,y)] != '+')
			if (pass_two)
			    /* fill in planet # */
			    fill_map(map, y, x, p, g_map.option);
			else
			    map[POS(x,y)] = 'o';
	    }
    if (!pass_two)
    {
	pass_two = 1;
	goto AGAIN;
    }
    /*
     *  Display map to screen or file.
     */
    for (y = 0; y < numrows; y++)
    {
	map[POS(COLS,y)] = '\0';
	mvwaddstr(MAPWIN, y+2, 0, &map[POS(0,y)]);
    }
    /* display axis labels */
    sprintf(g_msg, "%4.1f,%4.1f", x1, y1);
    mvwaddstr(MAPWIN, 0, 0, g_msg);

    sprintf(g_msg, "%4.1f,%4.1f\n", x2, y2);
    mvwaddstr(MAPWIN, 0, numcols - strlen(g_msg), g_msg);

    sprintf(g_msg, "%4.1f,%4.1f", x1, y2);
    mvwaddstr(MAPWIN, g.max.r+2, 0, g_msg);
    sprintf(g_msg, "%4.1f,%4.1f\n", x2, y2);
    mvwaddstr(MAPWIN, g.max.r+2, numcols-strlen(g_msg), g_msg);

    /* display line */
    for (i = 0; i < numcols; i++)
    {
	mvwaddch(MAPWIN, 1, i, '-');
	mvwaddch(MAPWIN, g.max.r+1, i, '-');
    }
    /* add mark */
    if (g.markx >= 0 && g.marky >= 0)
    {
	int x, y;

	y = PlanetWorldYToLoc(g.marky);
	x = PlanetWorldXToLoc(g.markx);

	if (y >= g.min.r && y <= g.max.r &&
		x >= g.min.c && x <= g.max.c)
	    mvwaddch(MAPWIN, y, x, 'X');
    }
    wrefresh(MAPWIN);
    PlanetPlaceCursor();

    return DONE;
}

/*
 *  Dump the map screen to a file.
 */
int PlanetMapDump (char *cp_filename)
{
    FILE *fp;
    char text[256];
    int y, len;

    if ((fp = fopen(cp_filename, "w")) == NULL)
    {
	errnoMsg(g_msg, cp_filename);
	message_print(g_msg);
	return FILEERR;
    }
    len = g.numcols;

    for (y = 0; y <= g.max.r+2; y++)
    {
	my_mvwinstr(MAPWIN, y, 0, len, text);
	text[len] = '\0';
	fprintf(fp, "%s\n", text);
    }
    fclose(fp);
    return OKAY;
}

static int fill_map( char *map, int y, int x, int p, int option)
{
    double dist;
    char text[80];
    int j;

    switch (option)
    {
	case OPT_PLANET_SIZE :
	    if (planet[p].size == 0)
		return NOT_OKAY;
	    sprintf(text, "%d", (int) planet[p].size);
	    break;
	case OPT_PLANET_RES :
	    if (planet[p].res == 0)
		return NOT_OKAY;
	    if ((int) planet[p].res == planet[p].res)
		sprintf(text, "%d", (int) planet[p].res);
	    else
		sprintf(text, "%3.1f", planet[p].res);
	    break;
	    /* show distance from center point of display */
	case OPT_PLANET_DIST :
	    dist = planet_xy_dist(g.center_x, g.center_y, p);
	    sprintf(text, "%.1f", (float) dist);
	    break;
	default :
	    strcpy(text, get_planet_name(p));
	    break;
    }

    for (j = 1; j <= strlen(text); j++)
    {
	if( (x+j >= COLS) || (map[POS(x+j,y)] != ' ') ) 
	{
	    break;
	} else {
	    map[POS(x+j,y)] = text[j-1];
	}
    } 
    return DONE;
}

int PlanetPlaceCursor (void)
{
    if (!MAPWIN)
	return NOT_OKAY;	/* MAP window not visible */

    wmove(MAPWIN, g.cursor.r, g.cursor.c);
    wrefresh(MAPWIN);
    return DONE;
}

/*
 *  Move the upper-left into the grid the following
 *  amount.  i_ymod and i_xmod represent the number
 *  characters to move the world (each character
 *  represents a cerain amount of real world space).
 */
static int PlanetMoveWorld (int i_ymod, int i_xmod)
{
    float deltax, deltay;
    float startx, starty;
    float max;

    if (!MAPWIN)
	return NOT_OKAY;	/* MAP window not visible */

    deltax = CUR_WIDTH / g.numcols;
    deltay = CUR_WIDTH / g.numrows;

    startx = CUR_STARTX + (i_xmod * deltax);
    starty = CUR_STARTY + (i_ymod * deltay);

    max = g_map.max_width - CUR_WIDTH;

    if (startx < 0)
	startx = 0;
    else if (startx > max)
	startx = max;

    if (starty < 0)
	starty = 0;
    else if (starty > max)
	starty = max;

    if ((startx != CUR_STARTX) || (starty != CUR_STARTY))
    {
	CUR_STARTX = startx;
	CUR_STARTY = starty;
	PlanetShow();
	return DONE;
    }
    return NOTHING;
}

int PlanetJumpWorld (int i_ymod, int i_xmod)
{
    int r, c;

    r = g.numrows/2;
    c = g.numcols/2;

    return PlanetMoveWorld(r * i_ymod, c * i_xmod);
}

/*  Move the cursor inside the grid */
int PlanetCursorMove (int i_ymod, int i_xmod)
{
    int modx, mody;
    int r, c;

    r = g.cursor.r + i_ymod;
    c = g.cursor.c + i_xmod;
    modx = 0;
    mody = 0;

    if (r < g.min.r)
    {
	r = g.min.r;
	mody--;
    }
    else if (r > g.max.r)
    {
	r = g.max.r;
	mody++;
    }

    if (c < g.min.c)
    {
	c = g.min.c;
	modx--;
    }
    else if (c > g.max.c)
    {
	c = g.max.c;
	mody++;
    }

    if (modx || mody)
    {
	g.cursor.r = r;
	g.cursor.c = c;
	return PlanetMoveWorld(i_ymod, i_xmod);
    }
    else if ((r != g.cursor.r) || (c != g.cursor.c))
    {
	g.cursor.r = r;
	g.cursor.c = c;
	PlanetPlaceCursor();
	return DONE;
    }
    return NOTHING;
}

/* Jump the cursor inside the grid */
/* However, jump in the direction indicated but stop until
 *   edge reached or a planet.
 */
int PlanetCursorJump (int i_ymod, int i_xmod)
{
    int r, c;

    r = g.cursor.r;
    c = g.cursor.c;

    if ((i_xmod == 0) && (i_ymod == 0))
    {
	r = g.max.r/2;
	c = g.max.c/2;
    }
    else
    {
	if (PlanetAutoPosition(i_ymod, i_xmod) == OKAY)
	    return DONE;

	if (i_xmod > 0)
	    c = g.max.c;
	else if (i_xmod < 0)
	    c = g.min.c;
	else if (i_ymod > 0)
	    r = g.max.r;
	else if (i_ymod < 0)
	    r = g.min.r;
    }
    if ((r != g.cursor.r) || (c != g.cursor.c))
    {
	g.cursor.r = r;
	g.cursor.c = c;
	PlanetPlaceCursor();
	return DONE;
    }
    return NOTHING;
}

/* place a mark at the current cursor location */
int PlanetPlaceMark (void)
{
    if (!MAPWIN)
	return NOT_OKAY;	/* MAP window not visible */
    g.marky = PlanetLocToWorldY(g.cursor.r);
    g.markx = PlanetLocToWorldX(g.cursor.c);
    PlanetShow();
    return DONE;
}

/*
 * zoom in with the box being at the current cursor
 * location and the other corner at the mark.
 * Be sure to remember the stack of zooms.
 */
int PlanetDoZoom (void)
{
    float startx, starty, width, widthx, widthy;
    float cursorx, cursory;
    double fabs();

    if (!MAPWIN)
	return NOT_OKAY;	/* MAP window not visible */

    if ((g.markx < 0) || (g.marky < 0))
	if (g.cur_zoom < g.top_zoom)
	    g.cur_zoom++;
	else
	    return ABORT;
    else
    {
	cursorx = PlanetLocToWorldX(g.cursor.c);
	cursory = PlanetLocToWorldY(g.cursor.r);

	startx = (g.markx < cursorx ? g.markx : cursorx);
	starty = (g.marky < cursory ? g.marky : cursory);
	widthx = fabs((double) g.markx - cursorx);
	widthy = fabs((double) g.marky - cursory);
	width = (widthx > widthy ? widthx : widthy);

	PlanetNewCoords(startx, starty, width);
    }
    g.markx = -1;
    g.marky = -1;

    PlanetShow();

    return DONE;
}

/*
 *  Do new zoom operation
 */
static int PlanetNewCoords (float startx, float starty, float width)
{
    if (!MAPWIN)
	return NOT_OKAY;	/* MAP window not visible */

    if (++g.cur_zoom >= MAX_ZOOM)
	g.cur_zoom = MAX_ZOOM-1;
    if (g.cur_zoom > g.top_zoom)
	g.top_zoom = g.cur_zoom;

    return PlanetSetCoords(startx, starty, width);
}

/*
 *  Do new zoom operation
 */
static int PlanetSetCoords (float startx, float starty, float width)
{
    if (!MAPWIN)
	return NOT_OKAY;	/* MAP window not visible */

    startx = CLIP(startx);
    starty = CLIP(starty);
    width = (width > g_map.max_width ? g_map.max_width : width);

    if (startx + width > g_map.max_width)
	startx = g_map.max_width - width;
    if (starty + width > g_map.max_width)
	starty = g_map.max_width - width;

    CUR_STARTX = startx;
    CUR_STARTY = starty;
    CUR_WIDTH = width;

    return DONE;
}

/*
 * We have remembered the each zoom, so unzoom
 * to the previous zoom.
 */
int PlanetUnZoom (void)
{
    if (!MAPWIN)
	return NOT_OKAY;	/* MAP window not visible */

    if (g.cur_zoom > 0)
    {
	g.cur_zoom--;
	g.marky = -1;
	g.markx = -1;
	PlanetShow();
    }
    return DONE;
}

/*
 * Recenter the map around the cursor.
 */
int PlanetRecenterAtCursor (void)
{
    float posx, posy;
    float startx, starty;

    if (!MAPWIN)
	return NOT_OKAY;	/* MAP window not visible */

    posx = PlanetLocToWorldX(g.cursor.c);
    posy = PlanetLocToWorldY(g.cursor.r);

    startx = posx - CUR_WIDTH/2;
    starty = posy - CUR_WIDTH/2;

    PlanetSetCoords(startx, starty, CUR_WIDTH);

    g.cursor.r = PlanetWorldYToLoc(posy);
    g.cursor.c = PlanetWorldXToLoc(posx);

    PlanetShow();

    return DONE;
}

/*
 *  Query for a planet and a new radius
 *  to reposition around
 */
int PlanetReposition (void)
{
    PromptObj prompt[2];
    char planetname[128];
    float starty, startx, width;
    int id;

    if (!MAPWIN)
	return NOT_OKAY;	/* MAP window not visible */

    planetname[0] = '\0';

    prompt[0].type = GM_STRING;
    prompt[0].prompt = "Planet : ";
    prompt[0].u.sval = planetname;
    prompt[1].type = GM_FLOAT;
    prompt[1].prompt = "Radius : ";
    prompt[1].u.fval = CUR_WIDTH/2;

    if (ScreenGetMany(prompt, 2, 0) != DONE)
	return ABORT;

    if ((id = translate_planet(planetname)) < 0)
	return IS_ERR;

    width = prompt[1].u.fval*2;
    starty = get_planet_y(id) - width/2;
    startx = get_planet_x(id) - width/2;

    PlanetNewCoords(startx, starty, width);

    g.cursor.r = PlanetWorldYToLoc(get_planet_y(id));
    g.cursor.c = PlanetWorldXToLoc(get_planet_x(id));

    PlanetShow();

    return DONE;
}

/*
 *  Move cursor to planet closest to current cursor position
 *   and also on the passed row.
 *
 *    i_row and i_col both zero : moves to closest planet.
 *    i_row < 0 moves to closest planet on same column as cursor
 *	on a smaller row.
 *    i_row > 0 moves to closest planet on same column as cursor
 *	on a greater row.
 *    i_col < 0 moves to closest planet on same row as cursor
 *	on a smaller column.
 *    i_col > 0 moves to closest planet on same row as cursor
 *	on a greater column.
 */
int PlanetAutoPosition (int i_row, int i_col)
{
    int p2, selectp;
    float cursorx, cursory, dist, mindist;
    float minx, maxx, miny, maxy;
    float deltax, deltay;

    if (!MAPWIN)
	return NOT_OKAY;	/* MAP window not visible */

    cursorx = PlanetLocToWorldX(g.cursor.c);
    cursory = PlanetLocToWorldY(g.cursor.r);
    deltax = CUR_WIDTH / g.numcols / 2;
    deltay = CUR_WIDTH / g.numrows / 2;
    mindist = 9999.0;

    if ((i_row == 0) && (i_col == 0))
    {
	minx = CUR_STARTX;
	maxx = CUR_STARTX + CUR_WIDTH;
	miny = CUR_STARTY;
	maxy = CUR_STARTY + CUR_WIDTH;
    }
    else if (i_row < 0)
    {
	minx = cursorx - deltax;
	maxx = cursorx + deltax;
	miny = CUR_STARTY;
	maxy = cursory - deltay;
    }
    else if (i_row > 0)
    {
	minx = cursorx - deltax;
	maxx = cursorx + deltax;
	miny = cursory + deltay;
	maxy = CUR_STARTY + CUR_WIDTH;
    }
    else if (i_col < 0)
    {
	minx = CUR_STARTX;
	maxx = cursorx - deltax;
	miny = cursory - deltay;
	maxy = cursory + deltay;
    }
    else if (i_col > 0)
    {
	minx = cursorx + deltax;
	maxx = CUR_STARTX + CUR_WIDTH;
	miny = cursory - deltay;
	maxy = cursory + deltay;
    }
    selectp = -1;

    /* get all distances */
    for (p2 = 0; p2 < g_num_planets; p2++)
	if (legal_planet(p2))
	    /* make sure planet within boundaries */
	    if ((get_planet_x(p2) >= minx) &&
		    (get_planet_x(p2) < maxx) &&
		    (get_planet_y(p2) >= miny) &&
		    (get_planet_y(p2) < maxy))
	    {
		dist = planet_xy_dist(cursorx, cursory, p2);

		if (dist < mindist)
		{
		    mindist = dist;
		    selectp = p2;
		}
	    }
    if (selectp == -1)
	return NOT_OKAY;

    g.cursor.r = PlanetWorldYToLoc(get_planet_y(selectp));
    g.cursor.c = PlanetWorldXToLoc(get_planet_x(selectp));
    PlanetShow();

    return OKAY;
}

int PlanetAtMark (void)
{
    int x, y, p;

    y = PlanetWorldYToLoc(g.marky);
    x = PlanetWorldXToLoc(g.markx);

    p = PlanetAt(y, x);
    if (p == -1)
    {
	sprintf(g_msg, "No planet at mark position");
	message_print(g_msg);
    }
    return p;
}

/*
 *  Returns the planet at the cursor.  If there is more
 *  than one choice, then if i_flag is QUERY a prompt
 *  dialog allows the player to select the planet,
 *  otherwise if i_flag is CLOSEST, then just one of the
 *  planets is picked.
 */
int PlanetAtCursor (int i_flag)
    /* i_flag may be QUERY or CLOSEST */
{
    int p;

    p = PlanetAtQuery(g.cursor.r, g.cursor.c, i_flag);

    if (p == -1)
    {
	sprintf(g_msg, "No planet at cursor position.");
	message_print(g_msg);
    }
    return p;
}

/*
 * An important function! Returns the planet
 * underneath the cursor.  If multiple choices
 * then the first one met is returned.
 *
 * -1 if no planet under cursor.
 */
static int PlanetAt (int row, int col)
{
    return PlanetAtQuery(row, col, CLOSEST);
}

#define MAX_P 10
/*
 * An important function! Returns the planet
 * underneath the cursor.  If multiple choices
 * the user is able to select which planet
 * if i_flag is QUERY, otherwise if i_flag
 * is CLOSEST then the first planet encountered
 * is selected.
 *
 * -1 if no planet under cursor.
 *
 * Note: if the position is not near any planet, the
 *   search distance is increased and attempted again.
 */
static int PlanetAtQuery (int row, int col, int i_flag)
    /* i_flag	may be QUERY or CLOSEST */
{
    float deltax, deltay;
    float posx, posy;
    float x1, y1, x2, y2;
    char *slist[MAX_P];
    char *prompt;
    int plist[MAX_P];
    int num_p, p, selectp;
    int trys;

    if (!MAPWIN)
	return -1;	/* MAP window not visible */

    deltax = CUR_WIDTH / g.numcols;
    deltay = CUR_WIDTH / g.numrows;
    trys = 0;

AGAIN:

    posx = PlanetLocToWorldX(col);
    posy = PlanetLocToWorldY(row);

    x1 = posx - deltax/2;
    y1 = posy - deltay/2;
    x2 = x1 + deltax;
    y2 = y1 + deltay;

    num_p = 0;

    for (p = 0; p < g_num_planets; p++)
	if (planet[p].set)
	    if ((planet[p].x >= x1) && (planet[p].x <= x2) &&
		    (planet[p].y >= y1) && (planet[p].y <= y2))
	    {
		if (i_flag == CLOSEST)
		    return p;
		if (num_p < MAX_P)
		    plist[num_p++] = p;
	    }
    if (num_p == 0)
    {
	/* Double distance and try again */
	if (trys++ == 0)
	{
	    deltax *= 2;
	    deltay *= 2;
	    goto AGAIN;
	}
	return -1;
    }
    if (num_p == 1)
	return plist[0];

    for (p = 0; p < num_p; p++)
	zalloc_cpy(&(slist[p]), get_planet_name(plist[p]));

    prompt = "More than one planet at loc: Select Planet";

    selectp = ScreenGetOneOfMany(slist, num_p, prompt);

    /* free up space */
    for (p = 0; p < num_p; p++)
	zalloc_cpy(&(slist[p]), NULL);

    if ((selectp >= 0) && (selectp < num_p))
	return plist[selectp];

    return -1;
}

static int compares( const void *e1, const void *e2 )
{
    return ((struct sorted *)e1)->dist >= ((struct sorted *)e2)->dist ? 1 : -1;
}

static char *get_who_str (int who)
{
    switch (who)
    {
	case WHO_ALIEN : return "(Alien)";
	case WHO_ME : return "(Me)";
	case WHO_FRIEND : return "(Friend)";
	default : return "";
    }
}

/*
 * Calculate distance from planet at cursor
 *   and rest of planets.  Returns id of
 *   planet we displayed for (>= 0).
 */
int PlanetDisplayDistAll (FILE *fp)
{
    struct sorted *darray;
    int p1, p2, i, who;
    double dist;
    float res, size;
    char *whos;

    if ((p1 = PlanetAtCursor(QUERY)) == -1)
	return -1;

    darray = (struct sorted *) MALLOC(sizeof(struct sorted) * g_num_planets);
    /* get all distances */
    for (p2 = 0; p2 < g_num_planets; p2++)
	if (legal_planet(p2))
	{
	    dist = planet_dist(p1, p2);
	    darray[p2].dist = dist;
	    darray[p2].p = p2;
	}
	else
	{
	    darray[p2].dist = 9999;
	    darray[p2].p = -1;
	}
    qsort((char *) darray, g_num_planets, sizeof(struct sorted), compares);

    for (i = 0; i < g_num_planets; i++)
	if ((p2 = darray[i].p) >= 0)
	    if (legal_planet(p2))
	    {
		who = get_who(p2);
		whos = get_who_str(who);

		fprintf(fp, " %s: %6.3f ly apart.",
			get_planet_name(p2), darray[i].dist);

		res = get_planet_res(p2);
		size = get_planet_size(p2);

		if (res == 0 && size == 0)
		    fprintf(fp, "%s\n", whos);
		else
		    fprintf(fp, " %.2f %.1f %s\n", res, size, whos);
	    }
    free(darray);

    return p1;
}

static long PlanetWorldXToLoc (float x)
{
    float v;

    v = ((x - CUR_STARTX) / CUR_WIDTH) * g.numcols + g.min.c;
    return ROUND(v);
}

static long PlanetWorldYToLoc (float y)
{
    float v;

    v = ((y - CUR_STARTY) / CUR_WIDTH) * g.numrows + g.min.r;
    return ROUND(v);
}

static float PlanetLocToWorldX (int c)
{
    float deltax;

    deltax = CUR_WIDTH / g.numcols;
    return CUR_STARTX + ((c - g.min.c) * deltax);
}

static float PlanetLocToWorldY (int r)
{
    float deltay;

    deltay = CUR_WIDTH / g.numrows;
    return CUR_STARTY + ((r - g.min.r) * deltay);
}


int save_zooms (void)
{
    FILE *fp;
    int i;

    if ((fp = fopen(SAVE_ZOOM, "w")) == NULL)
    {
	errnoMsg(g_msg, SAVE_ZOOM);
	message_print(g_msg);
	return FILEERR;
    }
    /* do not save top most, since this is the full screen */
    for (i = 1; i <= g.cur_zoom; i++)
    {
	fprintf(fp, "%5.2f %5.2f %5.2f\n",
		g.zoom[i].startx, g.zoom[i].starty, g.zoom[i].width);
    }
    fclose(fp);

    return DONE;
}

static int read_zooms (void)
{
    FILE *fp;
    float startx, starty, width;
    char line[80];

    if ((fp = fopen(SAVE_ZOOM, "r")) == NULL)
	return FILEERR;

    g.top_zoom = 0;

    while (fgets(line, sizeof(line), fp))
    {
	break_up(line);

	startx = element_float(line, 0);
	starty = element_float(line, 1);
	width = element_float(line, 2);

	g.top_zoom++;
	g.zoom[g.top_zoom].startx = startx;
	g.zoom[g.top_zoom].starty = starty;
	g.zoom[g.top_zoom].width = width;
    }
    fclose(fp);

    g.cur_zoom = g.top_zoom;

    return DONE;
}
