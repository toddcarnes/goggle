/*
 * File: battle.c
 * Author: Douglas Selph
 * Maintained by: Robin Powell
 * $Id: battle.c,v 1.3 1997/12/11 00:43:44 rlpowell Exp $
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include "common.h"
#include "data.h"

#include "errno.h"
#include "main.h"
#include "util.h"
#include "lex.h"
#include "group.h"
#include "ship.h"
#include "field.h"
#include "battle.h"

/**
 **  Local Structures and Defines
 **/
#define RACE_BLK_SIZE    5
#define GROUP_BLK_SIZE  16
#define MAX_RACE        20
#define FLAG_WAR         1
#define FLAG_PEACE       0
/* #define D(A) ??? not used */

struct battle_group
{
    char *ship_name;	/* name of ship design */
    int   ship_id;
    int   num_ships; 	/* before fight */
    int   num_left;	/* after fight */
    int   total_num_left;	/* cumulative left after all fights */
    struct tech_values
    {
	float drive;
	float weapon;
	float shield;
	float cargo;
    } tech;
    char *cargo_type;	/* type of cargo holding */
    float quantity;	/* amount of cargo holding (affects shield strength) */
    int   dist;		/* simulated dist from planet */
};


static struct global_battle_group_data
{
    int num_races;	/* number of races involved in fight */
    int alloc_size;
    int num_fights;	/* total number of fights done */

    struct battle_race
    {
	char                *name;		   /* name of race */
	struct battle_group *group;		   /* list of groups in fight */
	Boolean              at_war[MAX_RACE]; /* array of flags for each race */
	int                  num_groups;	   /* size of group array */
	int                  alloc_size;	   /* allocated # of groups */
	int                  race_id2;	   /* group module version of raceid */
	int                  num_survived;	   /* number of fights where # ships left > 0 */
    } *race;
} gB;

static int locate_battle_race (char *cp_racename);
static int atwar (int i_race1, int i_race2);
static int cankill(struct battle_group *attack, struct battle_group *target);
static int do_kill(struct battle_group *attack, struct battle_group *target);

extern char g_msg[MAX_MSG];
extern char g_your_race[MAX_RNAME];
extern int g_line_num;


/**
 ** Functions:
 **/

/*
 *  Read from the passed file some battle data.
 *
 *  The format expected is as follows (given as an example):

 Sample Battle

 AtWar

 Arachnids	War 	New_People
 Arachnids	Peace 	Tomar
 etc..

 Your Groups

#  T                  D     W     S  C  T  Q
1  Hungry_Orb-E    2.90  2.18  4.06  0  -  0
1  Killing_Orb-A   2.90  3.22  4.06  0  -  0
1  Silken_Strand   2.90  0.00  0.00  0  -  0
etc..

New_People Groups

#  T                D     W     S  C  T  Q
4  Destroyer-1   1.37  1.00  1.00  0  -  0
1  Destroyer-1   1.37  1.00  1.21  0  -  0
1  Battleship-1  1.37  1.09  1.23  0  -  0
etc..

 */
int read_battle_data (char *cp_filename)
{
    FILE *fp;
    char line[180], save[180], save2[180];
    char *rname1, *rname2, *status;
    char racename[100];
    int cur_race;
    int type, race1, race2;
    int num_ships, num_groups;
    float drive, weapon, shield, cargo, quantity;
    char *cargotype, *shiptype;

    init_battle();
    cur_race = -1;
    g_line_num = 0;

    if ((fp = fopen(cp_filename, "r")) == NULL)
    {
	errnoMsg(g_msg, cp_filename);
	message_print(g_msg);
	return FILEERR;
    }
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

	switch (type)
	{
	    case TYPE_RESULTS :
		break;

	    case TYPE_NEW_RACE :
		strcpy(racename, extract_race_name(save2));
		cur_race = battle_new_race(racename);
		/* add to group data too */

		break;
	    case TYPE_SAMPLE_BATTLE :
		break;
		/*
		 * Arachnids	War 	New_People
		 */
	    case TYPE_AT_WAR :

		rname1 = element_str(line, 0);
		status = element_str(line, 1);
		rname2 = element_str(line, 2);

		race1 = battle_new_race(rname1);
		race2 = battle_new_race(rname2);
		if ((status[0] == 'p') || (status[0] == 'P'))
		    gB.race[race1].at_war[race2] = FLAG_PEACE;
		else if ((status[0] == 'w') || (status[0] == 'W'))
		    gB.race[race1].at_war[race2] = FLAG_WAR;
		else
		{
		    sprintf(g_msg, "File %s, line %d: expected War or Peace, found %s",
			    cp_filename, g_line_num, status);
		    message_print(g_msg);
		    sprintf(g_msg, "  Line: %s", save2);
		    message_print(g_msg);
		}
		break;
		/*
		 * #  T                  D     W     S  C  T  Q
		 * 1  Hungry_Orb-E    2.90  2.18  4.06  0  -  0
		 *
		 * #  T                D     W     S  C  T  Q
		 * 4  Destroyer-1   1.37  1.00  1.00  0  -  0
		 */
	    case TYPE_YOUR_GROUPS :
	    case TYPE_ALIEN_GROUPS :

		num_ships = element_int(line, 0);
		shiptype = element_str(line, 1);
		drive = element_float(line, 2);
		weapon = element_float(line, 3);
		shield = element_float(line, 4);
		cargo = element_float(line, 5);
		cargotype = element_str(line, 6);
		quantity = element_float(line, 7);
		battle_new_group(cur_race, num_ships, shiptype,
			drive, weapon, shield, cargo, cargotype, quantity);
		break;
	}
    }
    fclose(fp);

    num_groups = 0;
    for (race1 = 0; race1 < gB.num_races; race1++)
	num_groups += gB.race[race1].num_groups;

    sprintf(g_msg, "Finished reading %d lines from \"%s\" for %d races and %d groups.\n",
	    g_line_num, cp_filename, gB.num_races, num_groups);
    message_print(g_msg);

    return DONE;
}


void init_battle (void)
{
    int i;

    if (gB.race)
    {
	for (i = 0; i < gB.num_races; i++)
	{
	    if (gB.race[i].name)
		free(gB.race[i].name);
	    if (gB.race[i].group)
		free(gB.race[i].group);
	}
	free(gB.race);
    }
    gB.num_races = 0;
    gB.num_fights = 0;
    gB.alloc_size = 0;
    gB.race = NULL;
}

/*
 *  Returns index value of race.
 */
int battle_new_race (char *cp_racename)
{
    int i;
    int next;

    if (!strcmp(cp_racename, YOUR))
	if (strlen(g_your_race) > 0)
	    return battle_new_race(g_your_race);
	else
	{
	    sprintf(g_msg, "'Your' race not specified.\n");
	    abort_program(g_msg);
	}
    /* Already exists? */
    if ((next = locate_battle_race(cp_racename)) >= 0)
	return next;

    next = gB.num_races++;

    if (gB.num_races >= gB.alloc_size)
    {
	if (gB.race)
	{
	    gB.alloc_size += RACE_BLK_SIZE;
	    gB.race = (struct battle_race *)
		REALLOC((char *)gB.race, gB.alloc_size * sizeof(struct battle_race));
	    for (i = gB.num_races; i < gB.alloc_size; i++)
	    {
		gB.race[i].group = 0;
		gB.race[i].name = 0;
		gB.race[i].race_id2 = 0;
	    }
	}
	else
	{
	    gB.num_races = 1;
	    gB.alloc_size = RACE_BLK_SIZE;
	    gB.race = (struct battle_race *)
		CALLOC(gB.alloc_size, sizeof(struct battle_race));
	    next = 0;
	}
    }
    zalloc_cpy(&(gB.race[next].name), cp_racename);
    gB.race[next].group = 0;
    gB.race[next].num_groups = 0;
    gB.race[next].alloc_size = 0;
    gB.race[next].num_survived = 0;
    /* Assume at war with everyone */
    for (i = 0; i < MAX_RACE; i++)
	if (next == i)
	    gB.race[next].at_war[i] = FLAG_PEACE;
	else
	    gB.race[next].at_war[i] = FLAG_WAR;

    if ((gB.race[next].race_id2 = get_race_id_no_err(cp_racename)) == RACE_ERR)
    {
	sprintf(g_msg, "Error!  No race %s listed in %s or %s.",
		cp_racename, SHIP_FILE, GROUP_FILE);
	message_print(g_msg);
	message_print("  If this race was not intended to exist then");
	sprintf(g_msg, " add a dummy entry to %s with the ship designs", SHIP_FILE);
	message_print(g_msg);
	abort_program("  that will be participating in the fight.");
    }
    if (next >= MAX_RACE)
    {
	sprintf(g_msg, "Battle Simulator error in %s\n", __FILE__);
	message_print(g_msg);
	sprintf(g_msg, "  Too many races: %d, with max of %d\n", next, MAX_RACE);
	message_print(g_msg);
	abort_program("  Increase # of races (MAX_RACE) in battle file.");
    }
    return next;
}

static int locate_battle_race (char *cp_racename)
{
    int i;

    for (i = 0; i < gB.num_races; i++)
	if (!strcmp(gB.race[i].name, cp_racename))
	    return i;
    return -1;
}

/* Allocate space for a new group.  Return index value */
static int battle_alloc_new_group (int i_raceid)
{
    int next;
    int i;

    next = gB.race[i_raceid].num_groups++;

    if (gB.race[i_raceid].num_groups >= gB.race[i_raceid].alloc_size)
    {
	if (gB.race[i_raceid].group)
	{
	    gB.race[i_raceid].alloc_size += RACE_BLK_SIZE;
	    gB.race[i_raceid].group = (struct battle_group *)
		REALLOC((char *)gB.race[i_raceid].group, gB.race[i_raceid].alloc_size * sizeof(struct battle_group));

	    for (i = gB.race[i_raceid].num_groups; i < gB.race[i_raceid].alloc_size; i++)
	    {
		gB.race[i_raceid].group[i].ship_name =
		    gB.race[i_raceid].group[i].cargo_type = 0;
		gB.race[i_raceid].group[i].num_ships =
		    gB.race[i_raceid].group[i].num_left = 0;
		gB.race[i_raceid].group[i].total_num_left = 0;
		gB.race[i_raceid].group[i].dist = 0;
		gB.race[i_raceid].group[i].tech.drive =
		    gB.race[i_raceid].group[i].tech.weapon =
		    gB.race[i_raceid].group[i].tech.shield =
		    gB.race[i_raceid].group[i].tech.cargo =
		    gB.race[i_raceid].group[i].quantity = 0;
	    }
	}
	else
	{
	    gB.race[i_raceid].alloc_size = GROUP_BLK_SIZE;
	    gB.race[i_raceid].group = (struct battle_group *)
		CALLOC(gB.race[i_raceid].alloc_size, sizeof(struct battle_group));
	    gB.race[i_raceid].num_groups = 1;
	    next = 0;
	}
    }
    return next;
}

/*
 *  New group of ships!
 */
int battle_new_group (int i_raceid, int i_numships, char *cp_shiptype,
	float f_drive, float f_weapon, float f_shield, float f_cargo,
	char *cp_cargotype, float f_quantity)
{
    int id;

    id = battle_alloc_new_group(i_raceid);

    zrealloc_cpy(&(gB.race[i_raceid].group[id].ship_name), cp_shiptype);
    gB.race[i_raceid].group[id].num_ships = i_numships;
    gB.race[i_raceid].group[id].num_left = i_numships;
    gB.race[i_raceid].group[id].tech.drive = f_drive;
    gB.race[i_raceid].group[id].tech.weapon = f_weapon;
    gB.race[i_raceid].group[id].tech.shield = f_shield;
    gB.race[i_raceid].group[id].tech.cargo = f_cargo;

    if (cp_cargotype)
	zrealloc_cpy(&(gB.race[i_raceid].group[id].cargo_type), cp_cargotype);
    else /* doesn't matter */
	if (f_quantity > 0)
	    zrealloc_cpy(&(gB.race[i_raceid].group[id].cargo_type), "COL");
	else
	    zrealloc_cpy(&(gB.race[i_raceid].group[id].cargo_type), "-");
    gB.race[i_raceid].group[id].quantity = f_quantity;

    if ((gB.race[i_raceid].group[id].ship_id =
	    get_ship_id(cp_shiptype, gB.race[i_raceid].race_id2)) == IS_ERR)
    {
	sprintf(g_msg, "Error!  Could not find ship type %s in %s",
		cp_shiptype, SHIP_FILE);
	message_print(g_msg);
	sprintf(g_msg, "  Add this ship design to the race %s's ship design",
		gB.race[i_raceid].name);
	message_print(g_msg);
	sprintf(g_msg, "  list in %s if this ship design is indeed a valid design.",
		SHIP_FILE);
	abort_program(g_msg);
    }
    return id;
}

/* Send all battle groups to the goggle group file */
void battle_init_groups (void)
{
    int race, grp, grpid;
    struct battle_group *bgrp;

    for (race = 0; race < gB.num_races; race++)
    {
	clear_groups(gB.race[race].race_id2);
	grpid = 0;

	for (grp = 0; grp < gB.race[race].num_groups; grp++)
	{
	    bgrp = &(gB.race[race].group[grp]);
	    set_group_data(gB.race[race].race_id2, grpid++,
		    bgrp->num_ships, bgrp->ship_name,
		    "PlanetName", (float) 0.0 /*dist*/,
		    bgrp->tech.drive, bgrp->tech.weapon,
		    bgrp->tech.shield, bgrp->tech.cargo,
		    bgrp->cargo_type, bgrp->quantity);
	}
    }
}

#define EXPLAIN_MSG1 \
"The simulator results were run against %d simulations.\n\
The 'L' field below represents the number of ships left after the last simulation.\n\
The 'Av' field below represents average number of ships left after %d simulations.\n\
Listed in the Results section is the number of simulations in which the race had at\n\
at least one ship that survived.\n"

#define EXPLAIN_MSG2 \
"Feel free to modify any of the fields below, or to add or remove lines.\n\
This file may be used as input to the battle simulator.\n"
/*
 *  Write out internal battle data to a file.
 */
int write_battle_data (char *cp_filename)
{
    FILE *filefp;
    int num_groups;

    if (cp_filename == 0)
	filefp = stdout;
    else if ((filefp = fopen(cp_filename, "w")) == NULL)
    {
	errnoMsg(g_msg, cp_filename);
	message_print(g_msg);
	return FILEERR;
    }
    num_groups = write_battle_data_raw(filefp, TRUE);

    if (cp_filename)
    {
	fclose(filefp);

	sprintf(g_msg, "Finished writing to %d races %d groups to \"%s\".\n",
		gB.num_races, num_groups, cp_filename);
	message_print(g_msg);
    }
    return DONE;
}

/* returns num_groups written */
int write_battle_data_raw (FILE *filefp, int i_mod_okay)
{
    FILE *fp;
    struct battle_group *bgrp;
    int race, race2;
    int group, pass, num_groups;
    char *state;

    fprintf(filefp, "\n\t\t%s\n\n", SAMPLE_BATTLE);
    fprintf(filefp, EXPLAIN_MSG1, gB.num_fights, gB.num_fights);

    if (i_mod_okay)
	fprintf(filefp, EXPLAIN_MSG2);

    /* Pass 1: determine field lengths (fp NULL) */
    /* Pass 2: do actual writes */
    if (gB.num_races > 1)
    {
	fieldReset();

	fprintf(filefp, "\n\t\t%s\n\n", AT_WAR);

	for (pass = 1; pass <= 2; pass++)
	{
	    if (pass == 1)
		fp = NULL;
	    else
		fp = filefp;

	    for (race = 0; race < gB.num_races; race++)
	    {
		for (race2 = 0; race2 < gB.num_races; race2++)
		    if (race != race2)
		    {
			fieldPrintStr(fp, 0, gB.race[race].name, " ", LEFT_JUSTIFY);
			state = (gB.race[race].at_war[race2] ? "War" : "Peace");
			fieldPrintStr(fp, 1, state, " ", LEFT_JUSTIFY);
			fieldPrintStr(fp, 2, gB.race[race2].name, " ", LEFT_JUSTIFY);
			if (fp)
			    fprintf(fp, "\n");
		    }
	    }
	}
    }
    else
	fprintf(filefp, "\n");

    num_groups = 0;

    for (race = 0; race < gB.num_races; race++)
    {
	fieldReset();

	for (pass = 1; pass <= 2; pass++)
	{
	    if (pass == 1)
		fp = NULL;
	    else
		fp = filefp;

	    if (fp)
		fprintf(fp, "\n\t\t%s %s\n\n", gB.race[race].name, GROUPS);

	    fieldPrintStr(fp, 0, "#", " ", LEFT_JUSTIFY);
	    fieldPrintStr(fp, 1, "T", " ", LEFT_JUSTIFY);
	    fieldPrintStr(fp, 2, "D", " ", LEFT_JUSTIFY);
	    fieldPrintStr(fp, 3, "W", " ", LEFT_JUSTIFY);
	    fieldPrintStr(fp, 4, "S", " ", LEFT_JUSTIFY);
	    fieldPrintStr(fp, 5, "C", " ", LEFT_JUSTIFY);
	    fieldPrintStr(fp, 6, "T", " ", LEFT_JUSTIFY);
	    fieldPrintStr(fp, 7, "Q", " ", LEFT_JUSTIFY);
	    fieldPrintStr(fp, 8, "L", " ", LEFT_JUSTIFY);
	    fieldPrintStr(fp, 9, "Av", " ", LEFT_JUSTIFY);
	    if (fp)
		fprintf(fp, "\n");

	    for (group = 0; group < gB.race[race].num_groups; group++)
	    {
		bgrp = &(gB.race[race].group[group]);
		fieldPrintInt(fp, 0, bgrp->num_ships, " ", LEFT_JUSTIFY);
		fieldPrintStr(fp, 1, bgrp->ship_name, " ", LEFT_JUSTIFY);
		fieldPrintFloat(fp, 2, bgrp->tech.drive, " ", LEFT_JUSTIFY);
		fieldPrintFloat(fp, 3, bgrp->tech.weapon, " ", LEFT_JUSTIFY);
		fieldPrintFloat(fp, 4, bgrp->tech.shield, " ", LEFT_JUSTIFY);
		fieldPrintFloat(fp, 5, bgrp->tech.cargo, " ", LEFT_JUSTIFY);
		fieldPrintStr(fp, 6, bgrp->cargo_type, " ", LEFT_JUSTIFY);
		fieldPrintFloat(fp, 7, bgrp->quantity, " ", LEFT_JUSTIFY);
		fieldPrintInt(fp, 8, bgrp->num_left, " ", LEFT_JUSTIFY);
		if (gB.num_fights > 0)
		    fieldPrintFloat(fp, 9,
			    (float) bgrp->total_num_left / (float) gB.num_fights, " ", LEFT_JUSTIFY);
		if (fp)
		{
		    fprintf(fp, "\n");
		    num_groups++;
		}
	    }
	    if (fp)
		fprintf(fp, "\n");
	}
    }
    /*
     *  Write result section out listing number of fights were
     *  we had survivors.
     */
    fprintf(filefp, "\n\t\t%s\n\n", RESULTS);

    fieldReset();

    for (pass = 1; pass <= 2; pass++)
    {
	if (pass == 1)
	    fp = NULL;
	else
	    fp = filefp;

	fieldPrintStr(fp, 0, "N", " ", LEFT_JUSTIFY);
	fieldPrintStr(fp, 1, "W", " ", LEFT_JUSTIFY);

	if (fp)
	    fprintf(fp, "\n");

	/* Write out Results for all groups all races */
	for (race = 0; race < gB.num_races; race++)
	{
	    fieldPrintStr(fp, 0, gB.race[race].name, " ", LEFT_JUSTIFY);
	    fieldPrintInt(fp, 1, gB.race[race].num_survived, " ", LEFT_JUSTIFY);

	    if (fp)
		fprintf(fp, "\n");
	}
    }
    return num_groups;
}

/*
 *  Do a single fight.
 *
 *   Note: the main idea here is not to be efficient but to
 *    accurately represent the galaxy3.4 combat code.  There
 *    were efficiencies I was tempted to place in, but resisted
 *    for fear of altering the combat algorithm.  The code
 *    below was copied from the galaxy3.4 code and modified
 *    slightly to use my own equivelent structures.
 */
int do_battle (char *cp_filename)
{
    FILE *fp;
    int ngroups,nships,nshipstofire,agroup,tgroup,shipno,involved;
    int i, j, r, g, /*found, ??? unused */ r2, g2;
    int moretargets,foundtarget;
    int numattacks;
    float weapon, shield;
    struct battle_group **fightgroups;
    int *shipstofire;
    int *groupplayers;
    int survived;

    fp = 0;

    if (cp_filename && strlen(cp_filename) > 0)
	if ((fp = fopen(cp_filename, "w")) == NULL)
	{
	    perror(cp_filename);
	    return FILEERR;
	}
	else
	{
	    sprintf(g_msg, "Output redirected to %s\n", cp_filename);
	    message_print(g_msg);
	}

    /* set num_left vars */
    for (r = 0; r < gB.num_races; r++)
	for (g = 0; g < gB.race[r].num_groups; g++)
	    gB.race[r].group[g].num_left = gB.race[r].group[g].num_ships;

    /* Set involved status based on at_war settings.  Rules:
     *   RACE's SHIP is involved if :
     *      SHIP has a weapon and at_war with another race participating, or,
     *      another race's ship has a weapon and is at_war with with SHIP, or,
     *	  RACE has any ship who is involved.
     * Net effect is as so:
     *   If a ship is at_war with another race, it will fire on all
     *    ships of that race.
     *   If a RACE is at_peace with another race, and that race is
     *    is at_war with RACE, then first race will kill only ships
     *    from second race who has a weapon against it.
     */
    ngroups = 0;
    nships = 0;

    /* count number of groups and number of ships involved */
    for (r = 0; r < gB.num_races; r++)
	for (g = 0; g < gB.race[r].num_groups; g++)
	    if (gB.race[r].group[g].dist == 0)
	    {
		involved = 0;

		for (r2 = 0; r2 < gB.num_races && !involved; r2++)
		    for (g2 = 0; g2 < gB.race[r2].num_groups; g2++)
			if ((gB.race[r2].group[g2].dist == 0) &&
				(((gB.race[r].group[g].tech.weapon > 0) && atwar(r, r2)) ||
				 ((gB.race[r2].group[g2].tech.weapon > 0) && atwar(r2, r))))
			{
			    involved = 1;
			    break;
			}
		if (involved)
		{
		    ngroups++;
		    nships += gB.race[r].group[g].num_ships;
		}
		else
		    gB.race[r].group[g].dist = -2;
	    }
    /* if any ship from race is firing, all ships for race implicated */
    for (r = 0; r < gB.num_races; r++)
	for (g = 0; g < gB.race[r].num_groups; g++)
	    if (gB.race[r].group[g].dist == -2)
		for (g2 = 0; g2 < gB.race[r].num_groups; g2++)
		    if (g2 != g && (gB.race[r].group[g2].dist == 0))
		    {
			gB.race[r].group[g].dist = 0;
			ngroups++;
			nships += gB.race[r].group[g].num_ships;
			break;
		    }
    /* clear dist bits */
    for (r = 0; r < gB.num_races; r++)
	for (g = 0; g < gB.race[r].num_groups; g++)
	    if (gB.race[r].group[g].dist == -2)
		gB.race[r].group[g].dist = 0;

    if (ngroups)
    {
	/* Collect data on all groups involved */
	fightgroups = (struct battle_group **)
	    CALLOC(ngroups, sizeof(struct battle_group *));
	shipstofire = (int *) CALLOC(ngroups, sizeof(int));
	groupplayers = (int *) CALLOC(ngroups, sizeof(int));

	i = 0;
	for (r = 0; r < gB.num_races; r++)
	    for (g = 0; g < gB.race[r].num_groups; g++)
		if (gB.race[r].group[g].dist == 0)
		{
		    involved = 0;
		    for (r2 = 0; r2 < gB.num_races && !involved; r2++)
			for (g2 = 0; g2 < gB.race[r2].num_groups; g2++)
			    if ((gB.race[r2].group[g2].dist == 0) &&
				    (((gB.race[r].group[g].tech.weapon > 0) && atwar(r, r2)) ||
				     ((gB.race[r2].group[g2].tech.weapon > 0) && atwar(r2, r))))
			    {
				involved = 1;
				break;
			    }
		    if (involved)
		    {
			groupplayers[i] = r;
			shipstofire[i] = 0;
			fightgroups[i] = &(gB.race[r].group[g]);
			i++;
		    }
		    else
			gB.race[r].group[g].dist = -2;
		}
	/* if any ship from race is firing, all ships for race implicated */
	for (r = 0; r < gB.num_races; r++)
	    for (g = 0; g < gB.race[r].num_groups; g++)
		if (gB.race[r].group[g].dist == -2)
		    for (g2 = 0; g2 < gB.race[r].num_groups; g2++)
			if (g2 != g && (gB.race[r].group[g2].dist == 0))
			{
			    gB.race[r].group[g].dist = 0;
			    groupplayers[i] = r;
			    shipstofire[i] = 0;
			    fightgroups[i] = &(gB.race[r].group[g]);
			    i++;
			    break;
			}
	/* clear dist bits */
	for (r = 0; r < gB.num_races; r++)
	    for (g = 0; g < gB.race[r].num_groups; g++)
		if (gB.race[r].group[g].dist == -2)
		    gB.race[r].group[g].dist = 0;

	assert(i == ngroups);

	/* Initialize struct for battle data
	 * (simulated -- nothing done here)
	 */

	/* Do combat */
	do
	{
	    /* Calculate number of ships to fire */
	    nshipstofire = 0;
	    for (i = 0; i != ngroups; i++)
		if (fightgroups[i]->tech.weapon > 0)
		{
		    shipstofire[i] = fightgroups[i]->num_left;
		    nshipstofire += fightgroups[i]->num_left;
		}
	    assert(nshipstofire);

	    /* Do one round of combat */
	    do
	    {
		/* Select a ship to fire */
		shipno = RAND() % nshipstofire;
		agroup = 0;
		while (shipno >= shipstofire[agroup])
		{
		    shipno -= shipstofire[agroup++];
		    assert(agroup < ngroups);
		}
		nshipstofire--;
		shipstofire[agroup]--;

		/* For each attack this ship has */
		get_ship_data(fightgroups[agroup]->ship_id,
			0, &numattacks, &weapon, &shield, 0);

		for (i = 0; i < numattacks; i++)
		{
		    /* Determine whether it has any available target */
		    moretargets = 0;
		    for (tgroup = 0; tgroup < ngroups && !moretargets; tgroup++) {
			if (!fightgroups[tgroup]->num_left)
			    continue;
			if (!cankill(fightgroups[agroup], fightgroups[tgroup]))
			    continue;
			if (atwar(groupplayers[agroup], groupplayers[tgroup]) ||
				(atwar(groupplayers[tgroup], groupplayers[agroup]) &&
				 fightgroups[tgroup]->tech.weapon > 0))
			    moretargets = 1;
			else if (atwar(groupplayers[tgroup], groupplayers[agroup]))
			    for (j = 0; j != ngroups; j++)
				if (groupplayers[j] == groupplayers[tgroup] &&
					fightgroups[j]->tech.weapon > 0) {
				    moretargets = 1;
				    break;
				}
		    }
		    if (!moretargets)
			break;

		    /* Select a target */
		    foundtarget = 0;
		    do
		    {
			shipno = RAND() % nships;
			tgroup = 0;
			while (shipno >= fightgroups[tgroup]->num_left)
			{
			    shipno -= fightgroups[tgroup++]->num_left;
			    assert (tgroup < ngroups);
			}
			assert (fightgroups[tgroup]->num_left);
			if (atwar (groupplayers[agroup],groupplayers[tgroup]) ||
				(atwar (groupplayers[tgroup],groupplayers[agroup]) &&
				 fightgroups[tgroup]->tech.weapon > 0))
			    foundtarget = 1;
			else
			    if (atwar (groupplayers[tgroup],groupplayers[agroup]))
				for (j=0; j!=ngroups; j++)
				    if (groupplayers[j] == groupplayers[tgroup] &&
					    fightgroups[j]->tech.weapon > 0)
				    {
					foundtarget = 1;
					break;
				    }
		    } while (!foundtarget);

		    /* Determine whether target destroyed */

		    assert(agroup >= 0 && agroup < ngroups);
		    assert(tgroup >= 0 && tgroup < ngroups);
		    assert(groupplayers[agroup] >= 0 && groupplayers[agroup] < gB.num_races);
		    assert(groupplayers[tgroup] >= 0 && groupplayers[tgroup] < gB.num_races);

		    if (fp)
			fprintf(fp, "%s %s fires on %s %s : ",
				gB.race[groupplayers[agroup]].name,
				fightgroups[agroup]->ship_name,
				gB.race[groupplayers[tgroup]].name,
				fightgroups[tgroup]->ship_name);

		    if (do_kill(fightgroups[agroup],fightgroups[tgroup]))
		    {
			if (RAND() % fightgroups[tgroup]->num_left < shipstofire[tgroup])
			{
			    shipstofire[tgroup]--;
			    nshipstofire--;
			}
			fightgroups[tgroup]->num_left--;
			nships--;
			if (fp)
			    fprintf(fp, "Destroyed\n");
		    }
		    else
			if (fp)
			    fprintf(fp, "Shields\n");
		}
	    } while (nshipstofire);

	    /* Determine whether combat needs to be continued */

	    moretargets = 0;
	    for (agroup=0; agroup!=ngroups && !moretargets; agroup++)
		if (fightgroups[agroup]->num_left)
		    for (tgroup=0; tgroup!=ngroups && !moretargets; tgroup++)
		    {
			if (!fightgroups[tgroup]->num_left ||
				!cankill(fightgroups[agroup],fightgroups[tgroup]))
			    continue;
			if (atwar(groupplayers[agroup],groupplayers[tgroup]) ||
				(atwar(groupplayers[tgroup],groupplayers[agroup]) &&
				 fightgroups[tgroup]->tech.weapon > 0))
			    moretargets = 1;
			else
			    if (atwar(groupplayers[tgroup],groupplayers[agroup]))
				for (j=0; j!=ngroups; j++)
				    if (groupplayers[j] == groupplayers[tgroup] &&
					    fightgroups[j]->tech.weapon > 0)
				    {
					moretargets = 1;
					break;
				    }
		    }
	} while (moretargets);

	free (groupplayers);
	free (shipstofire);
	free (fightgroups);
    }
    if (fp)
	fclose(fp);

    /* remember num_left vars */
    for (r = 0; r < gB.num_races; r++)
    {
	survived = 0;

	for (g = 0; g < gB.race[r].num_groups; g++)
	{
	    gB.race[r].group[g].total_num_left += gB.race[r].group[g].num_left;
	    if (gB.race[r].group[g].num_left > 0)
		survived = 1;
	}
	if (survived)
	    gB.race[r].num_survived++;
    }
    gB.num_fights++;

    return DONE;
}

static int cankill(struct battle_group *attack, struct battle_group *target)
{
    return ship_cankill(
	    attack->ship_id,
	    target->ship_id,
	    attack->tech.weapon,
	    target->tech.shield,
	    target->tech.cargo,
	    target->quantity);
}

static int do_kill(struct battle_group *attack, struct battle_group *target)
{
    return ship_kill(
	    attack->ship_id,
	    target->ship_id,
	    attack->tech.weapon,
	    target->tech.shield,
	    target->tech.cargo,
	    target->quantity);
}


/* Return True if race1 is at_war with race2 */
static int atwar (int i_race1, int i_race2)
{
    assert(i_race1 >= 0 && i_race1 < gB.num_races);
    assert(i_race2 >= 0 && i_race2 < gB.num_races);

    if (i_race1 == i_race2)
	return 0;

    return gB.race[i_race1].at_war[i_race2];
}
