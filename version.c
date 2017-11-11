/*
 * File: version.c
 * Author: Douglas Selph
 * Maintained by: Robin Powell
 * $Id: version.c,v 1.4 1997/12/14 15:13:23 rlpowell Exp $
 */
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include "common.h"
#include "data.h"

#include "util.h"
#include "errno.h"
#include "main.h"
#include "version.h"

/**
 **  Local Structures and Defines
 **/
/**
 ** Functions:
 **/
extern float g_galaxy_version;
extern MapData g_map;
extern char g_msg[MAX_MSG];
extern char g_your_race[MAX_RNAME];
extern int g_starting_cols;

int check_version (void)
{
    FILE *fp;
    char line[256];
    char key[128];
    char value[128];
    char *equal;
    int line_num;
    int bad_code = 0;

    g_galaxy_version = 3;

    if ((fp = fopen(VERSION_FILE,"r")) == NULL)
	return DONE;

    line_num = 0;

    while (fgets(line, sizeof(line), fp))
    {
	line_num++;

	strRmReturn(line);
	strStripSpaces(line);

	if (!(equal = strchr(line, '=')))
	{
	    printf("File %s, Line %d, malformed line:\n  \"%s\".\n",
		    VERSION_FILE, line_num, line);
	    bad_code = TRUE;
	    continue;
	}
	*equal = '\0';
	strcpy(key, line);
	strcpy(value, equal+1);
	*equal = '=';

	strStripSpaces(key);
	strStripSpaces(value);

	if (!strcmp(key, "version"))
	{
	    if (strIsDouble(value))
		g_galaxy_version = atof(value);
	    else
	    {
		printf("File %s, Line %d, Unrecognized integer (%s) following 'version' keyword.\n",
			VERSION_FILE, line_num, value);
		bad_code = TRUE;
	    }
	}
	else if (!strcmp(key, "map_width"))
	{
	    if (strIsInt(value))
		g_map.max_width = (float) atoi(value);
	    else
	    {
		printf("File %s, Line %d, Unrecognized integer (%s) following 'map_width' keyword.\n",
			VERSION_FILE, line_num, value);
		bad_code = TRUE;
	    }
	}
	else if (!strcmp(key, "racename"))
	{
	    strcpy(g_your_race, value);
	}
	else if (!strcmp(key, "columns"))
	{
	    if( strIsInt( value ) )
	    {
		g_starting_cols = atoi( value );
	    }
	}
	else
	{
	    printf("File %s, Line %d, unrecognized line:\n  \"%s\".\n",
		    VERSION_FILE, line_num, line);
	    bad_code = TRUE;
	    continue;
	}
    }
    fclose(fp);

    if (bad_code)
	exit(1);

    return DONE;
}

int write_version (void)
{
    FILE *fp;

    if ((fp = fopen(VERSION_FILE,"w")) == NULL)
    {
	errnoMsg(g_msg, VERSION_FILE);
	message_print(g_msg);
	return FILEERR;
    }
    fprintf(fp, "version = %3.1f\n", g_galaxy_version);
    fprintf(fp, "map_width = %d\n", (int) g_map.max_width);
    fprintf(fp, "racename = %s\n", g_your_race);
    fprintf(fp, "columns = %d\n", g_starting_cols );
    fclose(fp);

    sprintf(g_msg, "Wrote to \"%s\"", VERSION_FILE);
    message_print(g_msg);

    return DONE;
}
