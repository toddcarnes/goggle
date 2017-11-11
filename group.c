/*
 * File: group.c
 * Author: Douglas Selph
 * Maintained by: Robin Powell
 * $Id: group.c,v 1.4 1997/12/11 00:43:44 rlpowell Exp $
 */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include "common.h"
#include "data.h"

#include "util.h"
#include "readdata.h"
#include "main.h"
#include "field.h"
#include "ship.h"
#include "planet.h"
#include "version.h"
#include "battle.h"
#include "group.h"

/**
 **  Local Structures and Defines
 **/
#define PL_ALL -1
#define BLKSIZE 50

struct groups
{
    int groupid;
    int race;
    int num_ships;
    int planet_id;
    char *ship_name;
    char *planet_name;
    char *what;
    float dist_from_planet, quantity;
    float drive, weapon, shield, cargo;
    unsigned char set;
};

static struct group_data
{
    struct groups *group;	/* complete list of groups at all planets */
    int max_groups;
    char *racenames[NUM_RACES];
    int num_races;

    struct race_data
    {
	int inited;
	struct tech_level_def
	{
	    float drive, weapon, shield, cargo;	
		/* highest tech level achieved */
	} tech_level[2];
    } data[NUM_RACES];		/* data specific to each race */
} gG;

/* typedef struct tech_level_def *techPtr; ??? not used */

extern int g_line_num;
extern char g_msg[MAX_MSG];
extern float g_galaxy_version;
extern int g_your_race_id;
int g_print_group_tech;
int g_print_group_nums;
extern char g_your_race[MAX_RNAME];
extern int g_num_planets;

static int group_alloc (void);
static int write_group_data (char *cp_file);
static int write_all_groups (FILE *fp);
static int write_group (FILE *fp, int i_race, int i_pl, int i_dotitle);
static int write_group_line (FILE *fp, int i, int i_race);
static int get_group_slot (int i_race, int i_group);
static int group_exists (int i_race, int i_groupid, int i_numships,
	char *cp_shipname, char *cp_planetname, float f_dist,
	float f_drive, float f_weapon, float f_shield, float f_cargo);

/**
 ** Functions:
 **/

int init_groups (void)
{
    /* First race reserved for Test race */
    /* Second race reserved for Your race, to be
     * later replaced with actual race name */
    g_your_race_id = 1;
    g_print_group_nums = 1;
    gG.num_races = 2;

    gG.max_groups = 0;
    gG.group = 0;

    zalloc_cpy(&gG.racenames[0], "Test");
    zalloc_cpy(&gG.racenames[1], g_your_race);

    group_alloc();

    return read_data_file(GROUP_FILE, DATA_GROUPS);
}

/* clear all groups */
void clear_groups (int i_race)
{
    int id;

    for (id = 0; id < gG.max_groups; id++)
	if ((i_race == RACE_ALL) || (gG.group[id].race == i_race))
	{
	    gG.group[id].set = 0;
	    gG.group[id].race = -1;
	    gG.group[id].groupid = -1;
	}
}

int save_groups (void)
{
    return write_group_data(GROUP_FILE);
}

static int write_group_data (char *cp_file)
{
    FILE *fp;
    int sv;

    if ((fp = fopen(cp_file, "w")) == NULL)
    {
	perror(cp_file);
	return FILEERR;
    }
    sv = g_print_group_tech;
    g_print_group_tech = 1;
    write_all_races_groups(fp);
    g_print_group_tech = sv;

    sprintf(g_msg, "Wrote %s\n", cp_file);
    message_print(g_msg);

    fclose(fp);

    return DONE;
}

int write_all_races_groups (FILE *fp)
{
    g_print_group_nums = 0;
    fieldReset();
    write_all_groups(0);
    write_all_groups(fp);
    g_print_group_nums = 1;
    return DONE;
}

static int write_all_groups (FILE *fp)
{
    g_line_num = 0;
    write_group(fp, RACE_ALL, PL_ALL, 1/*, 0*/);  /* ??? what means 0 ??? */
	return DONE;
}


void write_pl_groups (FILE *fp, int i_pl)
{
    fieldReset();
    write_group(0, RACE_ALL, i_pl, 1/*, 0*/);     /* ??? what means 0 ??? */
	write_group(fp, RACE_ALL, i_pl, 1/*, 0*/);    /* ??? what means 0 ??? */
}

/*
 *   Write out the specified race's groups to the specified file.
 *   If i_pl is not PL_ALL will list groups on specified planet.
 *   Special: if fp is 0 then the field lengths for each field
 *    only will be generated using the field* functions.  The result?
 *    the computed lengths of each field.
 */
static int write_group (FILE *fp, int i_race, int i_pl, int i_dotitle)
    /* i_race	may be RACE_ALL
       i_pl	may be PL_ALL */
{
    int i;
    int r;
    int once;
    int printed_a_line;

    once = 0;
    printed_a_line = 0;

    if (i_race == RACE_ERR)
	return 0;

    if (i_race == RACE_ALL)
    {
	for (r = 0; r < gG.num_races; r++)
	    if (write_group(fp, r, i_pl, i_dotitle))
		printed_a_line = 1;
	return printed_a_line;
    }
    for (i = 0; i < gG.max_groups; i++)
    {
	if ((gG.group[i].race == i_race) && gG.group[i].set)
	{
	    if (i_pl == PL_ALL || (i_pl == gG.group[i].planet_id))
	    {
		if (i_dotitle && !once)
		{
		    if (fp)
		    {
			fprintf(fp, "\n");
			fprintf(fp, "\t\t%s %s %s\n", ABBR_GROUPS,
				gG.racenames[i_race], GROUPS);
			fprintf(fp, "\n");
		    }
		    fieldPrintStr(fp, 0, "G#", " ", LEFT_JUSTIFY);
		    fieldPrintStr(fp, 1, "#", " ", LEFT_JUSTIFY);
		    fieldPrintStr(fp, 2, "Type", " ", LEFT_JUSTIFY);
		    fieldPrintStr(fp, 3, "Planet", " ", LEFT_JUSTIFY);
		    fieldPrintStr(fp, 4, "Dist", " ", LEFT_JUSTIFY);

		    if (g_print_group_tech)
		    {
			fieldPrintStr(fp, 5, "DRIV", " ", LEFT_JUSTIFY);
			fieldPrintStr(fp, 6, "WEAP", " ", LEFT_JUSTIFY);
			fieldPrintStr(fp, 7, "SHLD", " ", LEFT_JUSTIFY);
			fieldPrintStr(fp, 8, "CARG", " ", LEFT_JUSTIFY);
		    }
		    else
		    {
			fieldPrintStr(fp, 5, " LY", " ", LEFT_JUSTIFY);
			fieldPrintStr(fp, 6, " ATK", " ", LEFT_JUSTIFY);
			fieldPrintStr(fp, 7, " SAF", " ", LEFT_JUSTIFY); /* Anders - SAFe not DEFence */
			fieldPrintStr(fp, 8, "CARG", " ", LEFT_JUSTIFY);
		    }
		    if (g_galaxy_version >= 3)
			fieldPrintStr(fp, 9, "UPGRD", " ", LEFT_JUSTIFY);

		    fieldPrintStr(fp, 10, "QUAN", " ", LEFT_JUSTIFY);

		    if (fp)
			fprintf(fp, "\n");

		    once = 1;
		}
		write_group_line(fp, i, i_race);
		printed_a_line = 1;
	    }
	}
    }
    return printed_a_line;
}

/*
 *   Special: if fp is 0 then the field lengths for each field
 *    only will be generated using the field* functions.  The result?
 *    the computed lengths of each field.
 */
static int write_group_line (FILE *fp, int i, int i_race)
{
    int ship_id;
    int print_tech;
    float drive, weapon, shield, cargo, quan;

    fieldPrintInt(fp, 0, gG.group[i].groupid, " ", LEFT_JUSTIFY);
    fieldPrintInt(fp, 1, gG.group[i].num_ships, " ", LEFT_JUSTIFY);
    fieldPrintStr(fp, 2, gG.group[i].ship_name, " ", LEFT_JUSTIFY);
    fieldPrintStr(fp, 3, gG.group[i].planet_name, " ", LEFT_JUSTIFY);

    if (gG.group[i].dist_from_planet >= 0)
	fieldPrintFloat(fp, 4, gG.group[i].dist_from_planet, " ", LEFT_JUSTIFY);
    else if (gG.group[i].dist_from_planet == BATTLE_DIST)
	fieldPrintStr(fp, 4, "Battle", " ", LEFT_JUSTIFY);
    else
	fieldPrintStr(fp, 4, "x", " ", LEFT_JUSTIFY);

    print_tech = 0;
    ship_id = get_ship_id(gG.group[i].ship_name, i_race);
    drive = gG.group[i].drive;
    weapon = gG.group[i].weapon,
    shield = gG.group[i].shield,
    cargo = gG.group[i].cargo;
    quan = gG.group[i].quantity;

    if (g_print_group_tech)
	print_tech = 1;
    else if (ship_id == IS_ERR)
	print_tech = 1;

    if (print_tech)
    {
	if (fp)
	    fprintf(fp, "[");

	fieldPrintFloat(fp, 5, gG.group[i].drive, ",", RIGHT_JUSTIFY);
	fieldPrintFloat(fp, 6, gG.group[i].weapon, ",", RIGHT_JUSTIFY);
	fieldPrintFloat(fp, 7, gG.group[i].shield, ",", RIGHT_JUSTIFY);
	fieldPrintFloat(fp, 8, gG.group[i].cargo, "]", RIGHT_JUSTIFY);

	if (fp)
	    fprintf(fp, " ");
    }
    else
    {
	calc_ship_data(ship_id, drive, weapon, shield, cargo, quan);
	write_ship_brief_data(fp, ship_id, 5);	/* 5 6 7 8 9 */
    }
    if ((gG.group[i].what != 0) && (gG.group[i].what[0] != '-'))
    {
	fieldPrintStr(fp, 9, gG.group[i].what, " ", LEFT_JUSTIFY);
	fieldPrintFloat(fp, 10, quan, " ", LEFT_JUSTIFY);
    }
    if (fp)
    {
	if (g_print_group_nums)
	    /* simulation comment too */
	    fprintf(fp, "##G%d\n", i);
	else
	    fprintf(fp, "\n");
    }
    return DONE;
}

int set_group_data(int i_race, int i_groupid, int i_numships,
	char *cp_shipname, char *cp_planetname,
	float f_dist,
	float f_drive, float f_weapon, float f_shield, float f_cargo,
	char *cp_what, float f_quantity)
{
    int planet_id;
    int id;

    /* okay to have a unrecognized planet id */
    planet_id = decode_planet_name(cp_planetname);

    if (i_groupid == BATTLE_DIST)
	/* special: battle group id */;
    else if (i_groupid < 0)
    {
	sprintf(g_msg, "set_group_data(), line %d : Illegal group number: %d\n",
		g_line_num, i_groupid);
	message_print(g_msg);
	return IS_ERR;
    }
    id = get_group_slot(i_race, i_groupid);

    gG.group[id].set = 1;
    /* set groupid if current -1(unset) or > 0 */
    if ((gG.group[id].groupid == -1) || (i_groupid > 0))
	gG.group[id].groupid = i_groupid;
    gG.group[id].race = i_race;
    gG.group[id].num_ships = i_numships;
    gG.group[id].planet_id = planet_id;
    gG.group[id].drive = f_drive;
    gG.group[id].weapon = f_weapon;
    gG.group[id].shield = f_shield;
    gG.group[id].cargo = f_cargo;
    zrealloc_cpy(&(gG.group[id].planet_name), cp_planetname);
    gG.group[id].dist_from_planet = f_dist;
    zrealloc_cpy(&(gG.group[id].ship_name), cp_shipname);
    zrealloc_cpy(&(gG.group[id].what), cp_what);
    if ((cp_what != 0) && strlen(gG.group[id].what) > 3)
	gG.group[id].what[3] = '\0';
    gG.group[id].quantity = f_quantity;

    /* the tech level of the ship should be recalled for the race */
    new_tech_level(i_race, f_drive, f_weapon, f_shield, f_cargo);

    return DONE;
}

int new_alien_group_data (int i_race, int i_groupid, int i_numships,
	char *cp_shipname, char *cp_planetname,
	float f_dist, float f_drive, float f_weapon,
	float f_shield, float f_cargo,
	char *cp_what, float f_quantity)
{
    int planet_id;
    int id;

    /* okay to have a unrecognized planet id */
    planet_id = decode_planet_name(cp_planetname);

    /* don't do anything if this group is already present */
    if ((id = group_exists(i_race, i_groupid, i_numships,
	    cp_shipname, cp_planetname, f_dist,
	    f_drive, f_weapon, f_shield, f_cargo)) >= 0)
	return id;

    id = get_group_slot(i_race, -1);

    gG.group[id].set = 1;
    gG.group[id].groupid = i_groupid;
    gG.group[id].race = i_race;
    gG.group[id].num_ships = i_numships;
    gG.group[id].planet_id = planet_id;
    gG.group[id].drive = f_drive;
    gG.group[id].weapon = f_weapon;
    gG.group[id].shield = f_shield;
    gG.group[id].cargo = f_cargo;
    zrealloc_cpy(&(gG.group[id].planet_name), cp_planetname);
    gG.group[id].dist_from_planet = f_dist;
    zrealloc_cpy(&(gG.group[id].ship_name), cp_shipname);
    gG.group[id].quantity = f_quantity;
    zrealloc_cpy(&(gG.group[id].what), cp_what);
    if ((cp_what != 0) && strlen(gG.group[id].what) > 3)
	gG.group[id].what[3] = '\0';
    /* the tech level of the ship should be recalled for the race */
    new_tech_level(i_race, f_drive, f_weapon, f_shield, f_cargo);

    return id;
}

/*
 * If i_group id does not exist, will
 * create new slot for that group id
 * and return this.
 */
static int get_group_slot (int i_race, int i_group)
{
    int id, try;

    /* first see if this group already exists */
    if (i_group >= 0)
	for (id = 0; id < gG.max_groups; id++)
	    if (gG.group[id].set)
		if ((gG.group[id].race == i_race) && (gG.group[id].groupid == i_group))
		    return id;

    for (try = 0; try < 2; try++)
    {
	/* find usused slot */
	for (id = 0; id < gG.max_groups; id++)
	    if (!gG.group[id].set)
		return id;

	/* need more space */
	group_alloc();
    }
    abort_program("Can't make any room for new groups!\n"); /* no return! */
    return -1;
}

static int group_alloc (void)
{
    int i;

    if (gG.group == 0)
    {
	gG.max_groups = BLKSIZE;
	gG.group = (struct groups *) CALLOC(gG.max_groups,
		sizeof(struct groups));
    }
    else
    {
	gG.max_groups += BLKSIZE;
	gG.group = (struct groups *)
	    REALLOC((char *)gG.group, gG.max_groups * sizeof(struct groups));

	for (i = gG.max_groups - BLKSIZE; i < gG.max_groups; i++)
	{
	    gG.group[i].set = 0;
	    gG.group[i].race = -1;
	    gG.group[i].groupid = -1;
	    gG.group[i].planet_id = -1;
	    gG.group[i].ship_name = 0;
	    gG.group[i].planet_name = 0;
	    gG.group[i].what = 0;
	    gG.group[i].dist_from_planet =
		gG.group[i].quantity =
		gG.group[i].drive =
		gG.group[i].weapon =
		gG.group[i].shield =
		gG.group[i].cargo = 0;
	}
    }
    return DONE;
}

static int group_exists (int i_race, int i_groupid, int i_numships,
	char *cp_shipname, char *cp_planetname, float f_dist,
	float f_drive, float f_weapon, float f_shield, float f_cargo)
{
    int id;

    for (id = 0; id < gG.max_groups; id++)
	if (gG.group[id].set && (gG.group[id].race == i_race) &&
		(gG.group[id].groupid == i_groupid) &&
		(gG.group[id].num_ships == i_numships) &&
		!strcmp(gG.group[id].ship_name, cp_shipname) &&
		!strcmp(gG.group[id].planet_name, cp_planetname) &&
		(gG.group[id].dist_from_planet == f_dist) &&
		(gG.group[id].drive == f_drive) &&
		(gG.group[id].weapon == f_weapon) &&
		(gG.group[id].shield == f_shield) &
		(gG.group[id].cargo == f_cargo))
	    return id;	/* good grief what a match! */

    return -1;
}

int group_new_race (char *cp_race)
{
    int next;
    int r;

    if (cp_race[0] == '\0')
	return RACE_ERR;
    if (!strcmp(cp_race, YOUR))
	return 1;
    /*
     *  Make sure this new race is not already in here or the
     *  name is part of another name.
     */
    for (r = 0; r < gG.num_races; r++)
	if (strLocate(gG.racenames[r], cp_race) >= 0)
	{
	    sprintf(g_msg, "Error: Already have race '%s' in race list.\n",
		    gG.racenames[r]);
	    message_print(g_msg);
	    return RACE_ERR;
	}
    if (gG.num_races >= NUM_RACES)
    {
	sprintf(g_msg, "Too many races. Could not add %s\n", cp_race);
	message_print(g_msg);
	return RACE_ERR;
    }
    next = gG.num_races++;
    zalloc_cpy(&gG.racenames[next], cp_race);
    /* add basics tech level for race */
    new_tech_level(next, (float) 1.0, (float) 1.0, (float) 1.0, (float) 1.0);
    return next;
}

int set_your_race_name (char *cp)
{
    if (!cp || (cp[0] == '\0'))
	return RACE_ERR;

    strRmReturn(cp);

    zrealloc_cpy(&gG.racenames[1], cp);
    new_tech_level(0, (float) 1.0, (float) 1.0, (float) 1.0, (float) 1.0);
    new_tech_level(1, (float) 1.0, (float) 1.0, (float) 1.0, (float) 1.0);
    strcpy(g_your_race, cp);

    write_version();

    return 1;
}

int get_race_id_no_err (char *cp)
{
    int r;

    for (r = 0; r < gG.num_races; r++)
	if (!strcmp(cp, gG.racenames[r]))
	    return r;

    return RACE_ERR;
}

/* 'cp' will contain the race name within it */
/* It is okay for the race not to be known */
int find_race_id (char *cp)
{
    int r;

    if (cp)
    {
	/* Special: YOUR race automatically is set to 1 */
	if (strLocate(cp, YOUR) >= 0)
	    return 1;

	for (r = 0; r < gG.num_races; r++)
	    if (strLocate(cp, gG.racenames[r]) >= 0)
		return r;
    }
    return RACE_ERR;
}


char *get_race_name (int race)
{
    return gG.racenames[race];
}

int get_race_techs (int race, int i_tech,
	float *fp_drive, float *fp_weapon, float *fp_shield, float *fp_cargo)
{
    /* if no techs add default tech */
    if (!gG.data[race].inited)
	new_tech_level(race, 1.0, 1.0, 1.0, 1.0);
    assert (race >= 0 && race < gG.num_races);
    assert (i_tech >= 0 && i_tech <= 1);

    *fp_drive = gG.data[race].tech_level[i_tech].drive;
    *fp_weapon = gG.data[race].tech_level[i_tech].weapon;
    *fp_shield = gG.data[race].tech_level[i_tech].shield;
    *fp_cargo = gG.data[race].tech_level[i_tech].cargo;

    return DONE;
}

int set_race_techs (int race, int i_tech,
	float f_drive, float f_weapon, float f_shield, float f_cargo)
{
    assert (race >= 0 && race < gG.num_races);
    assert (i_tech >= 0 && i_tech <= 1);

    gG.data[race].tech_level[i_tech].drive = f_drive;
    gG.data[race].tech_level[i_tech].weapon = f_weapon;
    gG.data[race].tech_level[i_tech].shield = f_shield;
    gG.data[race].tech_level[i_tech].cargo = f_cargo;

    return DONE;
}

int get_num_races (void)
{
    return gG.num_races;
}

static int chk_race_id (int id)
{
    if ((id < 0) || (id >= gG.num_races))
    {
	sprintf(g_msg, "race id %d out of range.\n", id);
	message_print(g_msg);
	return -1;
    }
    return id;
}

int new_tech_level (int race,
	float drive, float weapon, float shield, float cargo)
{
    if (chk_race_id(race) < 0)
	return IS_ERR;
    /*
     * For simplicity tech level zero means use tech level one
     */
    if (drive == 0.0)
	drive = 1.0;
    if (weapon == 0.0)
	weapon = 1.0;
    if (shield == 0.0)
	shield = 1.0;
    if (cargo == 0.0)
	cargo = 1.0;
    /*
     * Only add if we don't have it yet.
     */
    if (drive > gG.data[race].tech_level[CURRENT_TECH].drive)
	gG.data[race].tech_level[CURRENT_TECH].drive = drive;
    if (weapon > gG.data[race].tech_level[CURRENT_TECH].weapon)
	gG.data[race].tech_level[CURRENT_TECH].weapon = weapon;
    if (shield > gG.data[race].tech_level[CURRENT_TECH].shield)
	gG.data[race].tech_level[CURRENT_TECH].shield = shield;
    if (cargo > gG.data[race].tech_level[CURRENT_TECH].cargo)
	gG.data[race].tech_level[CURRENT_TECH].cargo = cargo;

    gG.data[race].inited = 1;

    return DONE;
}

/*
 *  Go through the list of all ships.  If
 *  the following conditions are met, the associated
 *  group had one ship destroyed.
 *	i_raceid matches groups raceid
 *	cp_shipname matches groups ship name.
 *	cp_planetname matches groups planet name.
 *	dist indicates battle distance.
 */
int group_check_destroyed (int i_raceid, char *cp_shipname, char *cp_planetname)
{
    int i;

    for (i = 0; i < gG.max_groups; i++)
	if (gG.group[i].set && (gG.group[i].race == i_raceid) &&
		(gG.group[i].dist_from_planet == BATTLE_DIST) &&
		!strcmp(gG.group[i].ship_name, cp_shipname) &&
		!strcmp(gG.group[i].planet_name, cp_planetname))
	{
	    gG.group[i].num_ships--;
	    return TRUE;
	}
    return FALSE;
}

#define UNSET 10000

static int groupval (struct groups *e1)
{
    int val;

    if (!e1->set)
	return UNSET;

    if (e1->race >= 0)
	val = e1->race * 200;
    else
	val = 0;

    if (e1->groupid > 0)
	val += e1->groupid;
    /* group values of zero mean alien groups */
    /* in this case, sort by planet */
    else
	val += e1->planet_id;
    return val;
}

static int grouppos( const void *e1, const void *e2 )
{
    int val1, val2;

    val1 = groupval((struct groups *)e1);
    val2 = groupval((struct groups *)e2);

    return val1 - val2;
}

void sort_groups (void)
{
    qsort((char *) gG.group, gG.max_groups, sizeof(struct groups), grouppos);
}

/*
 *  Prints race id of race printed (>= 0)
 */
int print_sorted_groups (FILE *fp, int i_race)
{
    int *pl;
    int i, dotitle, once;

    once = 0;

    pl = (int *) MALLOC(g_num_planets * sizeof(int));
    if (!pl)
    {
	char msg[128];
	sprintf(msg, "printed_sorted_groups(): Out of memory trying to allocate %d ints\n",
		g_num_planets);
	abort_program(msg);
    }
    /* get list of planets first */
    for (i = 0; i < g_num_planets; i++)
	pl[i] = 0;

    for (i = 0; i < gG.max_groups; i++)
	if (gG.group[i].set)
	    pl[gG.group[i].planet_id] = 1;

    fieldReset();

AGAIN:

    dotitle = 1;
    g_line_num = 0;

    /* display each planet set in order of set */
    for (i = 0; i < g_num_planets; i++)
	if (pl[i])
	    if (write_group((once ? fp : NULL), i_race, i, dotitle/*, 0*/)) /* ??? */
		dotitle = 0;
    if (!once)
    {
	once = 1;
	goto AGAIN;
    }
    free(pl);

    return i_race;
}

/*
 *  Place all the groups of the specified race at the specified planet
 *  into the current simulation.
 *  Return number of groups added.
 */
int place_groups_at_planet_in_sim (int i_race, int i_pl)
    /* i_race may be RACE_ALL */
{
    int i, r, race_id2, num;
    /*  char *type; ??? unused */

    num = 0;

    if (i_race == RACE_ALL)
    {
	for (r = 0; r < gG.num_races; r++)
	    num += place_groups_at_planet_in_sim(r, i_pl);

	return num;
    }
    assert(i_pl != PL_ALL);
    race_id2 = -1;

    for (i = 0; i < gG.max_groups; i++)
    {
	if ((gG.group[i].race == i_race) && gG.group[i].set)
	{
	    if ((i_pl == gG.group[i].planet_id) && (gG.group[i].num_ships > 0))
	    {
		if (race_id2 == -1)
		    race_id2 = battle_new_race(gG.racenames[i_race]);

		battle_new_group(race_id2,
			gG.group[i].num_ships,
			gG.group[i].ship_name,
			gG.group[i].drive,
			gG.group[i].weapon,
			gG.group[i].shield,
			gG.group[i].cargo,
			0, gG.group[i].quantity);
		num++;
	    }
	}
    }
    return num;
}

/* place group into simulation */
/* returns num_ships added */
int place_group_in_sim (int i_index)
{
    int race_id2;

    assert(i_index >= 0 && i_index < gG.max_groups);
    assert(gG.group[i_index].race >= 0 && gG.group[i_index].race < gG.num_races);

    race_id2 = battle_new_race(gG.racenames[gG.group[i_index].race]);

    battle_new_group(race_id2,
	    gG.group[i_index].num_ships,
	    gG.group[i_index].ship_name,
	    gG.group[i_index].drive,
	    gG.group[i_index].weapon,
	    gG.group[i_index].shield,
	    gG.group[i_index].cargo,
	    0, gG.group[i_index].quantity);

    return gG.group[i_index].num_ships;
}
