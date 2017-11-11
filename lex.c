/*
 * File: lex.c
 * Author: Douglas Selph
 * Maintained by: Robin Powell
 * $Id: lex.c,v 1.6 1998/06/19 20:58:00 rlpowell Exp $
 *   Extracted from the Machine archive.
 */
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include "common.h"
#include "data.h"

#include "util.h"
#include "group.h"
#include "lex.h"

/* #define MORE_MAX 22 ??? not used */

static int line_is_empty (char *s);
static int has_number (char *line);
static char *extract_race_name_from_title (char *cp);

int g_line_num;
int g_cur_type;
int g_read_one;
int g_race_id;
int g_your_race_id;
float g_galaxy_version;
int g_cur_display;
char g_msg[MAX_MSG];
MapData g_map;
char g_your_race[MAX_RNAME];
static int g_last_cur_type;

/* skip lines that do not begin with a simple number */
int line_type (char *line)
{
    /*
     * If a blank line and we read one, then
     * turn off current type
     */
    if (line_is_empty(line)  || (strLocate(line, OWN_ORDERS) >= 0)) /* Anders - don't parse own orders */
    {
	/* Blank lines in messages are OK */
	if( ( g_cur_type != TYPE_GMESSAGES ) 
		&& ( g_cur_type != TYPE_PMESSAGES ) )
	{
	    if (g_read_one)
	    {
		g_cur_type = TYPE_INVALID;
		g_read_one = 0;
	    }
	    return TYPE_INVALID;
	}
    }
    if (strLocate(line, REPORT_TITLE) >= 0)
    {
	set_your_race_name(extract_race_name_from_title(line));

	g_cur_type = TYPE_INVALID;
	g_read_one = 0;
	return TYPE_INVALID;
    }
    if (strLocate(line, STATUS_OF) >= 0)
    {
	g_cur_type = TYPE_STATUS_OF;
	g_read_one = 0;
	return TYPE_INVALID;
    }
    if (strLocate(line, YOUR_ROUTES) >= 0)
    {
	g_cur_type = TYPE_ROUTES;
	g_read_one = 0;
	return TYPE_INVALID;
    }
    if (strLocate(line, YOUR_PLANETS) >= 0)
    {
	g_cur_type = TYPE_YOUR_PLANETS;
	g_read_one = 0;
	return TYPE_INVALID;
    }
    if (strLocate(line, ALIEN_PLANETS) >= 0)
    {
	g_cur_type = TYPE_UNIDENTIFIED;
	g_read_one = 0;
	return TYPE_INVALID;
    }
    if (strLocate(line, NEUTRAL_PLANETS) >= 0)
    {
	g_cur_type = TYPE_UNINHABITED;
	g_read_one = 0;
	return TYPE_INVALID;
    }
    if (strLocate(line, PLANETS) >= 0)
    {
	g_cur_type = TYPE_PLANETS;
	g_read_one = 0;
	return TYPE_INVALID;
    }
    if (strLocate(line, SHIP_TYPES) >= 0)
    {
	g_cur_type = TYPE_SHIPS;
	g_read_one = 0;
	g_race_id = RACE_NEW;
	return TYPE_NEW_RACE;
    }
    if (strLocate(line, AT_WAR) >= 0)
    {
	g_cur_type = TYPE_AT_WAR;
	g_read_one = 0;
	return TYPE_INVALID;
    }
    if (strLocate(line, RESULTS) >= 0)
    {
	g_cur_type = TYPE_RESULTS;
	g_read_one = 0;
	return TYPE_INVALID;
    }
    if (strLocate(line, FLEET) >= 0)
    {
	g_cur_type = g_last_cur_type;
	g_race_id = g_your_race_id;
	return TYPE_INVALID;
    }
    if( strLocate( line, GLOBAL_MESSAGES ) >= 0 )
    {
	g_cur_type = TYPE_GMESSAGES;
	g_read_one = 0;
	return TYPE_INVALID;
    }
    if( strLocate( line, PERSONAL_MESSAGES ) >= 0 )
    {
	g_cur_type = TYPE_PMESSAGES;
	g_read_one = 0;
	return TYPE_INVALID;
    }
    if( strLocate( line, YOUR_OPTIONS ) >= 0 )
    {
	g_cur_type = TYPE_INVALID;
	g_read_one = 0;
	return TYPE_INVALID;
    }
    if (strLocate(line, GROUPS) >= 0)
    {
	if (strLocate(line, ABBR_GROUPS) >= 0)
	    if (strLocate(line, YOUR) >= 0)
		g_cur_type = TYPE_YOUR_ABBR_GROUPS;
	    else
		g_cur_type = TYPE_ALIEN_ABBR_GROUPS;
	else
	    if (strLocate(line, YOUR) >= 0)
		g_cur_type = TYPE_YOUR_GROUPS;
	    else
		g_cur_type = TYPE_ALIEN_GROUPS;
	g_last_cur_type = g_cur_type;
	g_race_id = RACE_NEW;
	g_read_one = 0;

	return TYPE_NEW_RACE;
    }
    if (strLocate(line, BATTLE_AT) >= 0)
	return TYPE_BATTLE_AT;
    if (strLocate(line, SAMPLE_BATTLE) >= 0)
	return TYPE_SAMPLE_BATTLE;
    if (strLocate(line, DESTROYED) >= 0)
	return TYPE_DESTROYED;
    if ((g_cur_type != TYPE_ROUTES) && (g_cur_type != TYPE_AT_WAR) &&
	(g_cur_type != TYPE_GMESSAGES) &&
	(g_cur_type != TYPE_PMESSAGES))
    {
	/* If there is no number on the line, 
	   then we have a title or something */
	if (!has_number(line))
	    return TYPE_INVALID;
    }
    g_read_one = 1;
    return g_cur_type;
}

/* Return TRUE if the passed line has a number in it */
static int has_number (char *line)
{
    char *s;

    for (s = line; *s != '\0'; s++)
	if (isdigit(*s))
	    return TRUE;

    return FALSE;
}

/*
 * Break up the line into a lot of individual strings.
 * Elements are separated by spaces.
 * Wierdity: the word 'Research' will belong to the previous word.
 * Also: will break up the form '[#,#,#,#]' into 4 individual string,
 */
void break_up (char *cp_line)
{
    char *s, *last;
    char *l;

    for (s = cp_line; *s != '\0'; s++)
	if ((*s == '[') || (*s == ',') || (*s == ']'))
	    *s = ' ';

    s = cp_line;

    /* start with first non-space */
    while (isspace(*s))
    {
	s++;

	if (*s == '\0')
	    goto EXIT;
    }

    while (*s != '\0')
    {
	last = s;

	/* skip to the end of the current word */
	while (!isspace(*s))
	{
	    if (*s == '\0')
		goto EXIT;

	    s++;
	}
	/*
	 * Handle Drive Research, Cargo Research, etc.
	 * by deleting word 'Research'.  That is, if Research
	 * is the word, all we do this pass is delete it.
	 */
	if (!strncmp(last, "Research", 8))
	    for (l = last; !isspace(*l); l++)
		*l = ' ';
	/* terminate word converting into string */
	else
	    *s = '\0';

	s++;

	while (isspace(*s))
	{
	    if (*s == '\0')
		goto EXIT;

	    s++;
	}
    }
EXIT :
    *(s+1) = '\0';
}

#if 0
what is that shit ???
/*
 * if first element is a lettered word, and second element is
 *  a lettered word, then treat as same (first) word.
 */
int element_merge (char *cp_line)
{
    char *s;
    char *first_end;

    while (*s != '\0')
	s++;

    first_end = s;

    s++;

    while (isspace(*s))
	s++;

    /* if the next word begins with a digit, then abort */
    if (isdigit(*s))
	return 0;

    /* merge first and second words */
    *first_end = ' ';

    return 1;
}
#endif

char *element_str (char *cp_line, int top)
{
    char *s;
    int e;

    s = cp_line;

    for (e = 0; e < top; e++)
    {
	while (*s != '\0')
	    s++;

	s++;

	while (isspace(*s))
	    s++;

	if (*s == '\0')
	    return 0;	/* end of line */
    }
    return s;
}

int element_int (char *cp_line, int e)
{
    char *s;

    if ((s = element_str(cp_line, e)) == 0)
	return 0;

    return atoi(s);
}


float element_float (char *cp_line, int e)
{
    float d;
    char *s;

    if ((s = element_str(cp_line, e)) == 0)
	return 0;

    d = atof(s);

    return (float) d;
}

static int line_is_empty (char *s)
{
    for (;*s != '\0';s++)
	if (!isspace(*s))
	    return 0;
    return 1;
}

/* Understands: "Draconians Report for Galaxy Turn 0" */
/* Understands: "Galaxy Game 5 Turn 46 Report for Dhelynitas" */
static char *extract_race_name_from_title (char *cp)
{
    static char name[80];
    char *s;
    int p;

    /* scan for word just past REPORT_TITLE */
    if ((p = strLocate(cp, REPORT_TITLE)) < 0)
	return 0;

    p += strlen(REPORT_TITLE) + 1;
    s = name;

    while ((cp[p] != ' ') && (cp[p] != '\0'))
	*(s++) = cp[p++];
    (*s++) = '\0';

    if (!strcmp(name, "Galaxy"))
    {
	/* just use first word */
	s = name;
	p = 0;

	while (cp[p] == ' ')
	    p++;

	while ((cp[p] != ' ') && (cp[p] != '\0'))
	    *(s++) = cp[p++];

	(*s++) = '\0';
    }
    return name;
}

char *extract_race_name (char *s)
{
    static char line[80];
    int p;

    strcpy(line, s);

    if ((p=strLocate(line, GROUPS)) >= 0)
	line[p] = '\0';
    else if ((p=strLocate(line, SHIP_TYPES)) >= 0)
	line[p] = '\0';

    strRmWord(line, ABBR_GROUPS);
    strStripSpaces(line);
    strRmReturn(line);

    return line;
}
