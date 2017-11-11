/*
 * File: planet.c
 * Author: Douglas Selph
 * Maintained by: Robin Powell
 * $Id: planet.c,v 1.4 1997/12/11 00:43:44 rlpowell Exp $
 */

#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include "common.h"
#include "data.h"
#define DEF
#include "planet.h"

#include "readdata.h"
#include "errno.h"
#include "main.h"
#include "field.h"
#include "util.h"

/**
 **  Local Structures and Defines
 **/
struct planets *planet;

#define BLK_SIZE 100

extern int g_line_num;
extern char g_msg[MAX_MSG];
extern float g_galaxy_version;
int g_num_planets;

static int get_new_planet_id (void);
static int write_planet_data (char *cp_file);
static int write_all_planets (FILE *fp);
static int write_planet_type (FILE *fp, int who, char *str, int i_xy);
static int write__planet_type (FILE *fp, int who, char *str, int i_xy);
static int write_planet_title (FILE *fp, int who, int i_xy);
static int write_single_planet (FILE *fp, int i, int who, int i_xy);
static int has_one_ptype (int who);
static int copy_planet (int from, int to);

/**
 ** Functions:
 **/

int init_planets (void)
{
    g_num_planets = 0;
    planet = 0;
    get_new_planet_id();	/* allocate first batch */

    return read_data_file(PLANET_FILE, DATA_PLANETS);
}

int save_planets (void)
{
    return write_planet_data(PLANET_FILE);
}

static int write_planet_data (char *cp_file)
{
    FILE *fp;

    if ((fp = fopen(cp_file, "w")) == NULL)
    {
	errnoMsg(g_msg, cp_file);
	message_print(g_msg);
	return FILEERR;
    }
    write_all_planets(fp);

    sprintf(g_msg, "Wrote %s.", cp_file);
    message_print(g_msg);

    fclose(fp);

    return DONE;
}

static int write_all_planets (FILE *fp)
{
    write_planet_type(fp, WHO_ME, YOUR_PLANETS, TRUE);
    if (has_one_ptype(WHO_FRIEND))
    {
	fprintf(fp, "\n");
	write_planet_type(fp, WHO_FRIEND, PLANETS, TRUE);
    }
    if (has_one_ptype(WHO_ALIEN))
    {
	fprintf(fp, "\n");
	write_planet_type(fp, WHO_ALIEN, ALIEN_PLANETS, TRUE);
    }
    if (has_one_ptype(WHO_NOONE))
    {
	fprintf(fp, "\n");
	write_planet_type(fp, WHO_NOONE, NEUTRAL_PLANETS, TRUE);
    }
    return DONE;
}

int print_some_planets (FILE *fp)
{
    write_planet_type(fp, WHO_ME, YOUR_PLANETS, FALSE);
    if (has_one_ptype(WHO_FRIEND))
    {
	fprintf(fp, "\n");
	write_planet_type(fp, WHO_FRIEND, PLANETS, FALSE);
    }
    if (has_one_ptype(WHO_NOONE_SEEN))
    {
	if (fp) fprintf(fp, "\n");
	write_planet_type(fp, WHO_NOONE_SEEN, PLANETS, FALSE);
    }
    return DONE;
}

static int write_planet_type (FILE *fp, int who, char *str, int i_xy)
{
    fieldReset();
    write__planet_type(NULL, who, str, i_xy);
    write__planet_type(fp, who, str, i_xy);
    return DONE;
}

/*
 * Called with NULL or fp depending on if we are measuring
 * field lengths or not
 */
static int write__planet_type (FILE *fp, int who, char *str, int i_xy)
{
    int i;

    if (str && fp)
	fprintf(fp, "\t\t%s\n\n", str);
    switch (who)
    {
	case WHO_FRIEND :
	case WHO_ME :
	case WHO_ALIEN :
	case WHO_NOONE :

	    write_planet_title(fp, who, i_xy);

	    for (i = 0; i < g_num_planets; i++)
		if (planet[i].set && (planet[i].who == who))
		    write_single_planet(fp, i, who, i_xy);
	    break;

	case WHO_NOONE_SEEN :

	    for (i = 0; i < g_num_planets; i++)
		if ((planet[i].set) && (planet[i].who == WHO_NOONE) && planet[i].size > 0)
		    write_single_planet(fp, i, who, i_xy);
	    break;
    }
    return DONE;
}


static int write_planet_title (FILE *fp, int who, int i_xy)
{
    int f;

    switch (who)
    {
	case WHO_FRIEND :
	case WHO_ME :
	    f = 0;
	    fieldPrintStr(fp, f++, "Name", " ", LEFT_JUSTIFY);
	    if (i_xy)
	    {
		fieldPrintStr(fp, f++, "X", " ", LEFT_JUSTIFY);
		fieldPrintStr(fp, f++, "Y", " ", LEFT_JUSTIFY);
	    }
	    fieldPrintStr(fp, f++, "Size", " ", LEFT_JUSTIFY);
	    fieldPrintStr(fp, f++, "Pop", " ", LEFT_JUSTIFY);
	    fieldPrintStr(fp, f++, "Indu", " ", LEFT_JUSTIFY);
	    fieldPrintStr(fp, f++, "Res", " ", LEFT_JUSTIFY);
	    fieldPrintStr(fp, f++, "Produce", " ", LEFT_JUSTIFY);
	    fieldPrintStr(fp, f++, "Cap", " ", LEFT_JUSTIFY);
	    fieldPrintStr(fp, f++, "Mat", " ", LEFT_JUSTIFY);
	    fieldPrintStr(fp, f++, "Col", "\n", LEFT_JUSTIFY);
	    break;
	case WHO_NOONE :
	case WHO_ALIEN :
	    fieldPrintStr(fp, 0, "Name", " ", LEFT_JUSTIFY);
	    fieldPrintStr(fp, 1, "X", " ", LEFT_JUSTIFY);
	    fieldPrintStr(fp, 2, "Y", " ", LEFT_JUSTIFY);
	    fieldPrintStr(fp, 3, "Size", " ", LEFT_JUSTIFY);
	    fieldPrintStr(fp, 4, "Res", "\n", LEFT_JUSTIFY);
	    break;
	case WHO_NOONE_SEEN :
	    fieldPrintStr(fp, 0, "Name", " ", LEFT_JUSTIFY);
	    fieldPrintStr(fp, 1, "Size", " ", LEFT_JUSTIFY);
	    fieldPrintStr(fp, 2, "Res", "\n", LEFT_JUSTIFY);
	    break;
    }
    return DONE;
}


static int write_single_planet (FILE *fp, int i, int who, int i_xy)
{
    int f;

    switch (who)
    {
	case WHO_FRIEND :
	case WHO_ME :
	    f = 0;
	    fieldPrintStr(fp, f++, get_planet_name(i), " ", LEFT_JUSTIFY);
	    if (i_xy)
	    {
		fieldPrintFloat(fp, f++, planet[i].x, " ", LEFT_JUSTIFY);
		fieldPrintFloat(fp, f++, planet[i].y, " ", LEFT_JUSTIFY);
	    }
	    fieldPrintFloat(fp, f++, planet[i].size, " ", LEFT_JUSTIFY);
	    fieldPrintInt(fp, f++, planet[i].pop, " ", LEFT_JUSTIFY);
	    fieldPrintFloat(fp, f++, planet[i].industry, " ", LEFT_JUSTIFY);
	    fieldPrintFloat(fp, f++, planet[i].res, " ", LEFT_JUSTIFY);
	    fieldPrintStr(fp, f++, get_produce_name(i), " ", LEFT_JUSTIFY);
	    fieldPrintFloat(fp, f++, planet[i].cap, " ", LEFT_JUSTIFY);
	    fieldPrintFloat(fp, f++, planet[i].mat, " ", LEFT_JUSTIFY);
	    fieldPrintFloat(fp, f++, planet[i].col, "\n", LEFT_JUSTIFY);
	    break;
	case WHO_ALIEN :
	case WHO_NOONE :
	    fieldPrintStr(fp, 0, get_planet_name(i), " ", LEFT_JUSTIFY);
	    fieldPrintFloat(fp, 1, planet[i].x, " ", LEFT_JUSTIFY);
	    fieldPrintFloat(fp, 2, planet[i].y, " ", LEFT_JUSTIFY);
	    if (planet[i].size == 0)
		fieldPrintStr(fp, 3, " ", " ", LEFT_JUSTIFY);
	    else
		fieldPrintFloat(fp, 3, planet[i].size, " ", LEFT_JUSTIFY);
	    if (planet[i].res == 0)
		fieldPrintStr(fp, 4, " ", "\n", LEFT_JUSTIFY);
	    else
		fieldPrintFloat(fp, 4, planet[i].res, "\n", LEFT_JUSTIFY);
	    break;
	case WHO_NOONE_SEEN :
	    fieldPrintStr(fp, 0, get_planet_name(i), " ", LEFT_JUSTIFY);
	    fieldPrintFloat(fp, 1, planet[i].size, " ", LEFT_JUSTIFY);
	    fieldPrintFloat(fp, 2, planet[i].res, "\n", LEFT_JUSTIFY);
	    break;
    }
    return DONE;
}

/*
 *  This function is used to write out a single planet's
 *  description to the user during the application run.
 *   (as opposed to file save).
 */
int write_single_planet_with_title (FILE *fp, int i)
{
    int who;

    if (!planet[i].set)
	return NOT_OKAY;

    fieldReset();

    who = planet[i].who;

    write_planet_title(NULL, who, FALSE);
    write_single_planet(NULL, i, who, FALSE);

    write_planet_title(fp, who, FALSE);
    write_single_planet(fp, i, who, FALSE);

    return DONE;
}

static int has_one_ptype (int who)
{
    int i;

    if (who == WHO_NOONE_SEEN)
    {
	for (i = 0; i < g_num_planets; i++)
	    if (planet[i].set && (planet[i].who == WHO_NOONE) && planet[i].size > 0)
		return 1;
    }
    else
	for (i = 0; i < g_num_planets; i++)
	    if (planet[i].set && (planet[i].who == who))
		return 1;
    return 0;
}

int legal_planet (int id)
{
    if ((id >= 0) && (id < g_num_planets))
	return planet[id].set;

    return 0;
}


int translate_planet (char *cp_name)
{
    int val;

    if ((val = decode_planet_name(cp_name)) == NEW)
    {
	/* find a new position */
	sprintf(g_msg, "Illegal planet name: '%s'", cp_name);
	message_print(g_msg);

	return IS_ERR;
    }
    return val;
}

int decode_planet_name (char *cp_name)
{
    int i;

    for (i = 0; i < g_num_planets; i++)
	if (planet[i].set && planet[i].name)
	    if (!strcmp(planet[i].name, cp_name))
		return i;

    return NEW;
}

static int locate_planet (float x, float y)
{
    int i;

    for (i = 0; i < g_num_planets; i++)
	if (planet[i].set)
	    if (planet[i].x == x && planet[i].y == y)
		return i;

    return IS_ERR;
}

static int is_number (char *cp_name)
{
    return isdigit(cp_name[0]);
}

char *get_planet_name (int id)
{
    char text[256];

    if ((id < 0) || (id >= g_num_planets))
    {
	sprintf(text, "get_planet_name(), line %d : Illegal planet number: %d\n",
		g_line_num, id);
	abort_program(text);
    }
    if (planet[id].name == NULL)
    {
	sprintf(text, "get_planet_name(), planet index %d has no name!\n", id);
	abort_program(text);
    }
    return planet[id].name;
}

char *get_produce_name (int id)
{
    static char text[128];

    if ((id < 0) || (id >= g_num_planets))
    {
	sprintf(text, "get_produce_name(), line %d : Illegal planet number: %d\n",
		g_line_num, id);
	abort_program(text);
    }
    if (planet[id].produce == NULL)
	return "<none>";

    strncpy(text, planet[id].produce, 8);
    text[8] = '\0';

    return text;
}

int set_planet_data (char *cp_name, float x, float y, float size, int pop,
	float industry, float res, char *cp_produce,
	float cap, float mat, float col, int who)
{
    int id, new_id, chk_id;
    int floater = 0;
    int reset = 0;

    /*
     * if we get the form, just "name" then
     * we don't mind where we put it
     */
    if (!is_number(cp_name))
	floater = 1;
    if ((id = locate_planet(x, y)) >= 0)
    {
	reset = 1;

	if ((chk_id = decode_planet_name(cp_name)) >= 0)
	{
	    if (id != chk_id)
	    {
		sprintf(g_msg,
			"Warning: Planet %d location does not match name %s(%d)\n",
			id, cp_name, chk_id);
		message_print(g_msg);
	    }
	}
    } else {
	if ((id = decode_planet_name(cp_name)) == IS_ERR)
	    return IS_ERR;
    }
    if (id == NEW)
    {
	id = get_new_planet_id();
	floater = 1;
    }
    if ((id < 0) || (id >= g_num_planets))
    {
	sprintf(g_msg, "set_planet_data(), line %d : Illegal planet number: %d\n",
		g_line_num, id);
	message_print(g_msg);
	return IS_ERR;
    }
    if (planet[id].set)
    {
	/*
	 * if we were specifying a specific planet,
	 * and the current planet is a floater, then
	 * move the planet.
	 */
	if (!reset && isdigit(cp_name[0]) && planet[id].floater)
	{
	    new_id = get_new_planet_id();
	    copy_planet(id, new_id);
	}
    }
    zrealloc_cpy(&(planet[id].name), cp_name);
    planet[id].set = 1;
    planet[id].floater = floater;
    planet[id].x = x;
    planet[id].y = y;
    if (size != 0)
	planet[id].size = size;
    if (pop != 0)
	planet[id].pop = pop;
    if (industry != 0)
	planet[id].industry = industry;
    if (res != 0)
	planet[id].res = res;
    zrealloc_cpy(&(planet[id].produce), cp_produce);
    planet[id].cap = cap;
    planet[id].mat = mat;
    planet[id].col = col;
    planet[id].who = who;

    return DONE;
}

static int copy_planet (int from, int to)
{
    planet[to].set = planet[from].set;
    planet[to].floater = planet[from].floater;
    planet[to].x = planet[from].x;
    planet[to].y = planet[from].y;
    planet[to].size = planet[from].size;
    planet[to].pop = planet[from].pop;
    planet[to].industry = planet[from].industry;
    planet[to].res = planet[from].res;
    planet[to].produce = planet[from].produce;
    planet[to].cap = planet[from].cap;
    planet[to].mat = planet[from].mat;
    planet[to].col = planet[from].col;
    planet[to].who = planet[from].who;
    planet[to].name = planet[from].name;
    planet[from].name = 0;
    planet[from].produce = 0;
    planet[from].size = 0;
    planet[from].pop = 0;
    planet[from].industry = 0;
    planet[from].res = 0;
    planet[from].cap = 0;
    planet[from].mat = 0;
    planet[from].col = 0;
    planet[from].who = 0;
    return DONE;
}


static int get_new_planet_id (void)
{
    int id;
    int prev;

    for (id = 0; id < g_num_planets; id++)
	if (!planet[id].set)
	    return id;

    /* allocate new array of planets */
    prev = g_num_planets;
    if (planet)
    {
	g_num_planets += BLK_SIZE;

	planet = (struct planets *)
	    REALLOC((char *)planet, sizeof(struct planets) * g_num_planets);
    }
    else
    {
	g_num_planets = BLK_SIZE;
	planet = (struct planets *)
	    MALLOC(sizeof(struct planets) * g_num_planets);
    }
    if (planet == NULL)
    {
	char msg[128];

	sprintf(msg, "get_new_planet_id(): Out of memory trying to allocate %d planets\n",
		g_num_planets);
	abort_program(msg);
    }
    for (id = prev; id < g_num_planets; id++)
    {
	planet[id].name =
	    planet[id].produce = (char *) 0;
	planet[id].x =
	    planet[id].y =
	    planet[id].size =
	    planet[id].res =
	    planet[id].pop =
	    planet[id].industry =
	    planet[id].cap =
	    planet[id].mat =
	    planet[id].col =
	    planet[id].who =
	    planet[id].set =
	    planet[id].floater = 0;
    }
    return prev;
}

double planet_dist (int pl_1, int pl_2)
{
    if ((pl_1 < 0) || (pl_1 >= g_num_planets))
    {
	sprintf(g_msg, "planet_dist() : Illegal planet#1 number : %d\n", pl_1);
	message_print(g_msg);
	return 0.0;
    }
    return planet_xy_dist(planet[pl_1].x, planet[pl_1].y, pl_2);
}

double planet_xy_dist (float x, float y, int pl)
{
    double dist;
    double hypot();

    if ((pl < 0) || (pl >= g_num_planets))
    {
	sprintf(g_msg, "planet_xy_dist() : Illegal planet#2 number : %d\n", pl);
	message_print(g_msg);
	return 0.0;
    }
    dist = hypot((double) x - (double) planet[pl].x,
	    (double) y - (double) planet[pl].y);
    return dist;
}

float get_planet_x (int id)
{
    return planet[id].x;
}

float get_planet_y (int id)
{
    return planet[id].y;
}

int get_who (int id)
{
    return planet[id].who;
}

float get_planet_res (int id)
{
    return planet[id].res;
}

float get_planet_size (int id)
{
    return planet[id].size;
}

/*
 *  Try to compute the number of ships
 *  a planet may produce.
 *
 *  Warning: for pre 3.0 this algorithm is flawed for
 *   resources values on planets less than, oh, about 1.0.
 */
#define INDPERSHIP 10

float compute_num_ships (int id, float shipmass)
{
    float ind;
    float nships;
    float matdemand;
    float planet_mat;
    float num_produced;
    float inships;        /* Anders - compute fragional ships not integer */

    if ((id < 0) || (id >= g_num_planets))
    {
	sprintf(g_msg, "compute_num_ships() : Illegal planet number : %d\n", id);
	message_print(g_msg);
	return 0.0;
    }
    ind = planet[id].industry * 0.75 + planet[id].pop * 0.25;

    if (ind < 0)
	return 0.0;

    planet_mat = planet[id].mat;

    nships = ind / shipmass / INDPERSHIP;
    matdemand = nships * shipmass;
    if (matdemand > planet_mat)
    {
	nships = nships * planet_mat / matdemand;
	matdemand = planet_mat;
    }
    inships = nships;     /* Anders - remove rounding to whole ships */
    nships = inships;
    ind -= nships * shipmass * INDPERSHIP;
    planet_mat -= nships * shipmass;
    num_produced = inships;
    inships = (ind / shipmass / (1 / planet[id].res + INDPERSHIP)); /* Anders - remove rounding again */
    num_produced += inships;

    return num_produced;
}

#define INDPERCAP 5

float compute_estimate (int id, int i_what)
{
    float ind;
    float matdemand;
    float amt;

    if ((id < 0) || (id >= g_num_planets))
    {
	sprintf(g_msg, "compute_num_ships() : Illegal planet number : %d\n", id);
	message_print(g_msg);
	return 0.0;
    }
    ind = planet[id].industry * 0.75 + planet[id].pop * 0.25;
    if (ind < 0)
	return 0;
    amt = 0;

    switch (i_what)
    {
	case PR_CAP :
	    matdemand = ind / INDPERCAP;
	    if (matdemand > planet[id].mat)
		matdemand = planet[id].mat;
	    ind -= matdemand * INDPERCAP;
	    amt += matdemand;
	    matdemand = ind / (INDPERCAP + 1 / planet[id].res);
	    amt += matdemand;
	    break;
	case PR_MAT :
	    amt = ind * planet[id].res;
	    break;
	case PR_DRIVE :
	case PR_WEAPONS :
	case PR_SHIELDS :
	    amt = ind / 5000;
	    break;
	case PR_CARGO :
	    if (g_galaxy_version >= 3.0)
		amt = ind / 2000;
	    else
		amt = ind / 5000;
	    break;
    }
    return amt;
}
