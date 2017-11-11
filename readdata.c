/*
 * File: readdata.c
 * Author: Douglas Selph
 * Maintainer: Robin Powell
 * $Id: readdata.c,v 1.4 1998/06/19 20:58:00 rlpowell Exp rlpowell $
 *   Extracted from the Machine archive.
 */

#include <stdio.h>
#include <ctype.h>
#include <assert.h>
#include "common.h"
#include "data.h"

#include "errno.h"
#include "main.h"
#include "util.h"
#include "lex.h"
#include "group.h"
#include "planet.h"
#include "ship.h"
#include "readdata.h"
#include "message.h"

/**
 **  Local Structures and Defines
 **/
extern int g_line_num;
extern int g_cur_type;
extern int g_race_id;
extern int g_your_race_id;
extern int g_read_one;
extern char g_msg[MAX_MSG];

static char *type_str (int i_what);

/**
 ** Functions:
 **/

/*
 *  This function reads from the save files or turn
 *  report groups, ships, and/or planets.
 *
 *  i_what may be
 *	DATA_ALL 	(read groups, ships, and planets)
 *	DATA_GROUPS	(read groups only)
 *	DATA_SHIPS	(read ships only)
 *      DATA_PLANETS	(read planets only)
 */
int read_data_file (char *cp_file, int i_what)
{
    FILE *fp, *routefp;
    char line[STRING_SIZE], save[STRING_SIZE], save2[STRING_SIZE];
    char battle_planet[STRING_SIZE];
    char racename[STRING_SIZE];
    char *shipname, *planetname, *checkrace, *produce, *rname;
    char *what;
    float dist, drive, weapon, shield, cargo, quantity;
    float x, y, size, res, industry;
    float cap, mat, col;
    int pop, who;
    int groupid;
    int num_ships, left;
    int past_planets;
    int type, attacks;
    int race;
    int did_clear, did_your_clear;

    if ((fp = fopen(cp_file, "r")) == NULL)
    {
	errnoMsg(g_msg, cp_file);
	message_print(g_msg);
	return FILEERR;
    }
    switch (i_what)
    {
	case DATA_ALL :
	case DATA_GROUPS :
	    clear_groups(RACE_ALL);
	    break;
	case DATA_SHIPS :
	case DATA_PLANETS :
	    break;
	default :
	    assert(0);
    }
    did_clear = TRUE;
    did_your_clear = TRUE;

    g_line_num = 0;
    g_cur_type = TYPE_INVALID;
    past_planets = FALSE;
    racename[0] = '\0';
    battle_planet[0] = '\0';
    routefp = NULL;

    while (fgets(line, sizeof(line), fp) != NULL)
    {
	g_line_num++;

	strcpy(save, line);

	strStripSpaces(line);
	strRmTab(line);
	strRmReturn(line);

	if ((type = line_type(line)) == TYPE_INVALID)
	    continue;

	strcpy(save2, line);

	break_up(line);
	/* element_merge(line); */

	switch (type)
	{
	    /***********
	     *** ALL ***
	     ***********/
	    case TYPE_NEW_RACE :
		if ((g_race_id = find_race_id(save2)) == RACE_ERR)
		{
		    /* will be registered when first ship or group received */
		    g_race_id = RACE_NEW;
		    strcpy(racename, extract_race_name(save2));
		}
		break;
		/*
		 * Example:
		 * Arachnids        2.90  4.45  4.06  1.00  11056.65  8672.09  22
		 */
	    case TYPE_STATUS_OF :

		rname = element_str(line, 0);
		drive = element_float(line, 1);
		weapon = element_float(line, 2);
		shield = element_float(line, 3);
		cargo = element_float(line, 4);

		if ((race = find_race_id(rname)) != RACE_ERR)
		    new_tech_level(race, drive, weapon, shield, cargo);
		break;

		/***************
		 *** PLANETS ***
		 ***************/

		/*
		 * Example:
		 *   N	   X       Y       S	    P	     I	      R      P		$     M  C
		 *   Nest  188.54  102.21  1000.00  1000.00  1000.00  10.00  Griffin-A	0.00  0  29.90
		 */
	    case TYPE_YOUR_PLANETS :

		planetname = element_str(line, 0); x = element_float(line, 1);
		y = element_float(line, 2);
		size = element_float(line, 3);
		pop = element_int(line, 4);
		industry = element_float(line, 5);
		res = element_float(line, 6);
		produce = element_str(line, 7);
		cap = element_float(line, 8);
		mat = element_float(line, 9);
		col = element_float(line, 10);
		who = WHO_ME;

		set_planet_data(planetname, x, y, size, pop, industry, res,
			produce, cap, mat, col, who);

		break;

		/*
		 * Example:
		 *   N	   X       Y       S	    P	     I	      R      P		$     M  C
		 *   Nest  188.54  102.21  1000.00  1000.00  1000.00  10.00  Griffin-A	0.00  0  29.90
		 */
	    case TYPE_PLANETS :

		planetname = element_str(line, 0);
		x = element_float(line, 1);
		y = element_float(line, 2);
		size = element_float(line, 3);
		pop = element_int(line, 4);
		industry = element_float(line, 5);
		res = element_float(line, 6);
		produce = element_str(line, 7);
		cap = element_float(line, 8);
		mat = element_float(line, 9);
		col = element_float(line, 10);
		who = WHO_FRIEND;

		set_planet_data(planetname, x, y, size, pop, industry, res,
			produce, cap, mat, col, who);

		break;

		/*
		 * Example:
		 *   N	   X       Y
		 *   Nest  188.54  102.21
		 */
	    case TYPE_UNIDENTIFIED :

		planetname = element_str(line, 0);
		x = element_float(line, 1);
		y = element_float(line, 2);
		size = 0;
		pop = 0;
		industry = 0;
		res = 0;
		produce = 0;
		cap = 0;
		mat = 0;
		col = 0;
		who = WHO_ALIEN;

		set_planet_data(planetname, x, y, size, pop, industry, res,
			produce, cap, mat, col, who);

		past_planets = TRUE;	/* we are now ready to read in normal group section */
		break;
		/*
		 * Example:
		 *   N	   X       Y       S	    R
		 *   Nest  188.54  102.21  1000.00  10.00
		 */
	    case TYPE_UNINHABITED :

		planetname = element_str(line, 0);
		x = element_float(line, 1);
		y = element_float(line, 2);
		size = element_float(line, 3);
		res = element_float(line, 4);
		pop = 0;
		industry = 0;
		produce = 0;
		cap = 0;
		mat = 0;
		col = 0;
		who = WHO_NOONE;

		set_planet_data(planetname, x, y, size, pop, industry, res,
			produce, cap, mat, col, who);

		past_planets = TRUE;	/* we are now ready to read in normal group section */
		battle_planet[0] = '\0';
		break;

		/**************
		 *** GROUPS ***
		 **************/
		/*
		 * Example:
		 * G#  #   Type            Planet       Dist   DRIV WEAP SHLD CARG
		 * 102 1   Silken_Strand   Limburger    0.00   [1.31,0.00,0.00,0.00]
		 */
	    case TYPE_YOUR_ABBR_GROUPS :
		/*
		 * If we are reading from the save.groups file then
		 * we should clear out the groups before continuing
		 */
		if (!did_clear)
		{
		    clear_groups(RACE_ALL);
		    did_clear = TRUE;
		}
		race = g_your_race_id;
		groupid = element_int(line, 0);
		num_ships = element_int(line, 1);
		shipname = element_str(line, 2);
		planetname = element_str(line, 3);
		if (!strcmp(element_str(line,4), "Battle"))
		{
		    dist = BATTLE_DIST;
		    groupid = -1;
		}
		else
		    dist = element_float(line, 4);
		drive = element_float(line, 5);
		weapon = element_float(line, 6);
		shield = element_float(line, 7);
		cargo = element_float(line, 8);
		what = element_str(line, 9);
		quantity = element_float(line, 10);

		set_group_data(race, groupid, num_ships, shipname, planetname, dist,
			drive, weapon, shield, cargo, what, quantity);
		break;

		/*
		 * Example:
		 * G   #	T	      D  W     S  C  T	      Q  D	     R
		 * 1   8	Stork-A    2.42  1  1.97  1  COL   1.73  Olive	  0.19
		 */
	    case TYPE_YOUR_GROUPS :

		if (!did_clear && !did_your_clear)
		{
		    clear_groups(g_your_race_id);
		    did_your_clear = TRUE;
		}
		race = g_your_race_id;

		if (past_planets)
		{
		    groupid = element_int(line, 0);
		    num_ships = element_int(line, 1);
		    shipname = element_str(line, 2);
		    drive = element_float(line, 3);	/* Drive Tech */
		    weapon = element_float(line, 4);	/* Weapon Tech */
		    shield = element_float(line, 5);	/* Shield Tech */
		    cargo = element_float(line, 6);	/* Cargo Tech */
		    what = element_str(line, 7);
		    quantity = element_float(line, 8);
		    planetname = element_str(line, 9);
		    dist = element_float(line, 10);

		    set_group_data(race, groupid, num_ships, shipname, planetname, dist,
			    drive, weapon, shield, cargo, what, quantity);
		}
		/*
		 *  Otherwise these groups participated in a battle.
		 *  There is no need to show these groups, because
		 *  if they survived they would show up in the normal
		 *  groups section.  If they didn't they are not
		 *  around any more anyway.
		 */
		break;
		/* Example:
		 * G#  #   Type            Planet       Dist   DRIV WEAP SHLD CARG
		 * 0   1   slob            Mallow       Battle [1.06,0.00,0.00,0.00]
		 */
	    case TYPE_ALIEN_ABBR_GROUPS :

		if ((g_race_id == RACE_NEW) && (racename[0] != '\0'))
		    g_race_id = group_new_race(racename);
		if (g_race_id == RACE_ERR)
		    break;	/* unknown race */

		/* if we are reading from the save.groups file then
		 * we should clear out the groups before continuing */
		if (!did_clear)
		{
		    clear_groups(RACE_ALL);
		    did_clear = TRUE;
		}
		race = g_race_id;
		groupid = element_int(line, 0);
		num_ships = element_int(line, 1);
		shipname = element_str(line, 2);
		planetname = element_str(line, 3);
		if (!strcmp(element_str(line,4), "Battle"))
		    dist = BATTLE_DIST;
		else
		    dist = element_float(line, 4);
		drive = element_float(line, 5);
		weapon = element_float(line, 6);
		shield = element_float(line, 7);
		cargo = element_float(line, 8);
		what = element_str(line, 9);
		quantity = element_float(line, 10);

		new_alien_group_data(race, groupid, num_ships, shipname, planetname,
			dist, drive, weapon, shield, cargo, what, quantity);
		break;

		/*
		 * Example:
		 *  #  T                 D     W     S  C  T  Q  D
		 *  1  drone          1.00  0.00  0.00  0  -  0  Squid_Plane
		 * Battle Example:
		 *  #  T	    D  W  S  C	T  Q  L
		 *  2  The_Eye  1  1  0  0	-  0  0
		 */
	    case TYPE_ALIEN_GROUPS :

		if ((g_race_id == RACE_NEW) && (racename[0] != '\0'))
		    g_race_id = group_new_race(racename);
		if (g_race_id == RACE_ERR)
		    break;	/* unknown race */

		race = g_race_id;

		if (past_planets)
		{
		    num_ships = element_int(line, 0);
		    shipname = element_str(line, 1);
		    drive = element_float(line, 2);	/* Drive Tech */
		    weapon = element_float(line, 3);	/* Weap Tech */
		    shield = element_float(line, 4);	/* Shield Tech */
		    cargo = element_float(line, 5);	/* Cargo Tech */
		    what = element_str(line, 6);	/* Type */
		    quantity = element_float(line, 7);	/* Quantity */
		    planetname = element_str(line, 8);
		    dist = element_float(line, 9);

		    new_alien_group_data(race, 0, num_ships, shipname, planetname, dist,
			    drive, weapon, shield, cargo, what, quantity);
		}
		else /* from battle section */
		{
		    /* Expected: #  T         D     W  S  C  T  Q  L */
		    num_ships = element_int(line, 0);
		    shipname = element_str(line, 1);
		    drive = element_float(line, 2);	/* Drive Tech */
		    weapon = element_float(line, 3);	/* Weapon Tech */
		    shield = element_float(line, 4);	/* Shield Tech */
		    cargo = element_float(line, 5);	/* Cargo Tech */
		    what = element_str(line, 6);
		    quantity = element_float(line, 7);
		    if (element_str(line, 8) != 0)
			left = element_int(line, 8);
		    else
			left = num_ships;
		    planetname = battle_planet;
		    dist = BATTLE_DIST;

		    new_alien_group_data(race, 0, left, shipname, planetname, dist,
			    drive, weapon, shield, cargo, what, quantity);
		}
		break;
	    case TYPE_BATTLE_AT :

		strcpy(battle_planet, element_str(line, 2));
		break;
		/*
		 *  Try to update the groups read in from a battle.
		 *  If a ship was destroyed then update the #ships
		 *  in the associated group.
		 *
		 *  Example line:
		 *  Tokugawa  Tanto  fires on  Arachnids  Silken_Strand  :  Destroyed
		 */
	    case TYPE_DESTROYED :

		checkrace = element_str(line, 4);
		shipname = element_str(line, 5);
		if ((race = get_race_id_no_err(checkrace)) >= 0)
		    group_check_destroyed(race, shipname, battle_planet);
		break;

		/*************
		 *** SHIPS ***
		 *************/

		/* Sample: Daddy_Long_LegB   7.0  0   0   0.0  8.0   */
	    case TYPE_SHIPS :

		if ((g_race_id == RACE_NEW) && (racename[0] != '\0'))
		    g_race_id = group_new_race(racename);
		if (g_race_id == RACE_ERR)
		    break;	/* unknown race */

		shipname = element_str(line, 0);

		if (!strcmp(shipname, "Tech"))
		{
		    g_read_one = FALSE;	/* doesn't count */

		    shipname = element_str(line, 1);
		    drive = element_float(line, 2);
		    weapon = element_float(line, 3);
		    shield = element_float(line, 4);
		    cargo = element_float(line, 5);

		    new_tech_level(g_race_id, drive, weapon, shield, cargo);
		}
		else
		{
		    drive = element_float(line, 1);
		    attacks = element_int(line, 2);
		    weapon = element_float(line, 3);
		    shield = element_float(line, 4);
		    cargo = element_float(line, 5);

		    new_ship_data(shipname, drive, attacks, weapon, shield, cargo, g_race_id);
		}
		break;
		/**************
		 *** ROUTES ***
		 **************/
	    case TYPE_ROUTES :
		if (!routefp)
		    if ((routefp = fopen(ROUTE_FILE, "w")) == NULL)
		    {
			errnoMsg(g_msg, ROUTE_FILE);
			message_print(g_msg);
			g_cur_type = TYPE_INVALID;
		    }
		if (routefp)
		    fputs(save, routefp);
		break;
		/****************
		 *** MESSAGES ***
		 ****************/
	    case TYPE_PMESSAGES :
	    case TYPE_GMESSAGES :
		{ /*Starting a block arbitrarily allows you to declare vars*/
		    static char in_message=0;
		    
		    if( ! in_message )
		    {
			if( ! strcmp( save2, "-message starts-" ) )
			{
			    printf("\n****New message readdata");
			    in_message = 1;	/* strcmp's return val pisses
						   me off */
			    new_message( type );
			}
		    }
		    else
		    {
			if( ! strcmp( save2, "-message ends-" ) )
			{
			    in_message = 0;
			}
			else
			{
			    new_message_line( save2 );
			}
		    }
		}
		break;
	    case TYPE_RESULTS :
		break;
	}
    }
    fclose(fp);
    if (routefp)
	fclose(routefp);

    sort_ships();
    sort_groups();

    sprintf(g_msg, "%s read from '%s'\n", type_str(i_what), cp_file);
    message_print(g_msg);

    return DONE;
}

static char *type_str (int i_what)
{
    switch (i_what)
    {
	case DATA_ALL     : return "Data";
	case DATA_GROUPS  : return "Groups";
	case DATA_SHIPS   : return "Ships";
	case DATA_PLANETS : return "Planets";
    }
    return "";
}
