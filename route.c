/*
 * File: route.c
 * Author: Douglas Selph
 * Maintainer: Robin Powell
 * $Id: route.c,v 1.4 1997/12/15 19:23:37 rlpowell Exp $
 */

#include <stdio.h>
#include "common.h"
#include "data.h"

#include "errno.h"
#include "main.h"
#include "util.h"
#include "planet.h"
#include "route.h"

/**
 **  Local Structures and Defines
 **/
extern int g_line_num;
extern char g_msg[MAX_MSG];

static int route_has_planet (int i_planet);

/**
 ** Functions:
 **/

int print_routes (FILE *outfp)
{
    FILE *fp;
    char line[160];

    if ((fp = fopen(ROUTE_FILE, "r")) == NULL)
    {
	errnoMsg(g_msg, ROUTE_FILE);
	message_print(g_msg);
	return FILEERR;
    }
    while (fgets(line, 160, fp) != NULL)
    {
	strExpandTab(line);
	fprintf(outfp, "%s", line);
    }
    fclose(fp);

    return DONE;
}

/* print routes that involve the passed planet */
int print_pl_routes (FILE *outfp, int i_planet)
{
    FILE *fp;
    char line[160];
    char *name;
    int once;

    if (!route_has_planet(i_planet))
	return IS_ERR;

    name = get_planet_name(i_planet);

    if ((fp = fopen(ROUTE_FILE, "r")) == NULL)
    {
	errnoMsg(g_msg, ROUTE_FILE);
	message_print(g_msg);
	return FILEERR;
    }
    /* Print out first non-blank line (title) then any line with
     *  the planet name in it
     */
    fprintf(outfp, "\n");
    once = 0;

    while (fgets(line, 160, fp))
    {
	strExpandTab(line);
	strRmReturn(line);

	if (line[0] == '\0')
	    continue;

	if (!once || strLocate(line, name) >= 0)
	{
	    fprintf(outfp, "%s\n", line);
	    once = 1;
	}
    }
    fclose(fp);
    return DONE;
}


/* See if there is a route with this name */
static int route_has_planet (int i_planet)
{
    FILE *fp;
    char line[160];
    char *name;

    name = get_planet_name(i_planet);

    if ((fp = fopen(ROUTE_FILE, "r")) == NULL)
	return 0;

    while (fgets(line, 160, fp) != NULL)
	if (strLocate(line, name) >= 0)
	    return 1;
    fclose(fp);

    return 0;
}
