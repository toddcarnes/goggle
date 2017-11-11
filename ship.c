/*
 * File: ship.c
 * Author: Douglas Selph
 * Maintained by: Robin Powell
 * $Id: ship.c,v 1.5 1997/12/11 00:43:44 rlpowell Exp $
 */
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <assert.h>
#include <string.h>
#include <math.h>
#include "common.h"
#include "data.h"

#include "util.h"
#include "readdata.h"
#include "errno.h"
#include "main.h"
#include "group.h"
#include "field.h"
#include "ship.h"

/**
 **  Local Structures and Defines
 **/
#define MAX_SHIP_LEN 32
#define BLKSIZE 20
#define BATTLEMAGIC 3.107232506

static struct ships
{
    char *name;
    float drive, weapon, shield, cargo;
    int attacks;
    float mass;
    float ly, ly_full;
    float attack_pow, defense, cost, eff_cargo;
    float eff_defense; 	/* after cargo effects */
    float upgrade_percent;/* % of normal ship cost to upgrade a single ship */
    float quantity;	/* current calc save of amount carrying */
    int race;		/* which race */
    float safe,eff_safe;  /* Anders - add safe against gun data */
    int set;
} *ship;

int g_max_ships;
int g_print_ship_nums;
extern int g_line_num;
extern float g_galaxy_version;
extern char g_msg[MAX_MSG];

static char *get_ship_name(int id);
static int ship_alloc (void);
static int write_ship_data (char *cp_file);
static int write_ship_type (FILE *fp, int race, int i_tech, int i_title);
static double frand2(void);
static float weapon_mass(int attacks, float weapon);

/**
 ** Functions:
 **/

int init_ships (void)
{
    g_max_ships = 0;
    g_print_ship_nums = 1;
    ship = 0;
    ship_alloc();

    ship[TEST_SLOT].set = 1;	/* reserved for test slot */
    ship[TEST_SLOT].race = -1;
    zalloc_cpy(&(ship[TEST_SLOT].name), "Test Ship");

    return read_data_file(SHIP_FILE, DATA_SHIPS);
}

int save_ships (void)
{
    return write_ship_data(SHIP_FILE);
}

static int ship_alloc (void)
{
    int i;

    if (ship == 0)
    {
	g_max_ships = BLKSIZE;
	ship = (struct ships *) CALLOC(g_max_ships,
		sizeof(struct ships));
    }
    else
    {
	g_max_ships += BLKSIZE;
	ship = (struct ships *) REALLOC((char *)ship,
		g_max_ships * sizeof(struct ships));

	for (i = g_max_ships - BLKSIZE; i < g_max_ships; i++)
	{
	    ship[i].set = 0;
	    ship[i].name = 0;
	    ship[i].race = -1;
	}
    }
    return DONE;
}

static int write_ship_data (char *cp_file)
{
    FILE *fp;

    if ((fp = fopen(cp_file, "w")) == NULL)
    {
	errnoMsg(g_msg, cp_file);
	message_print(g_msg);
	return FILEERR;
    }
    write_all_races_ships(fp);

    sprintf(g_msg, "Wrote %s\n", cp_file);
    message_print(g_msg);

    fclose(fp);
    return DONE;
}

void write_all_races_ships (FILE *fp)
{
    int i;

    g_print_ship_nums = 0;
    for (i = 0; i < get_num_races(); i++)
	write_all_ships(fp, i, CURRENT_TECH);
    g_print_ship_nums = 1;
}

void write_all_ships (FILE *fp, int race, int tech)
{
    g_line_num = 0;
    fieldReset();
    write_ship_type(NULL, race, tech, WITH_TITLE);
    write_ship_type(fp, race, tech, WITH_TITLE);
}

static int write_ship_type (FILE *fp, int race, int i_tech, int i_title)
{
    int i, once;
    char tmp[256];
    float drive, weapon, shield, cargo;

    get_race_techs(race, i_tech, &drive, &weapon, &shield, &cargo);
    once = 0;

    for (i = 0; i < g_max_ships; i++)
	if (ship[i].set && (ship[i].race == race))
	{
	    if (!once)
	    {
		once = 1;

		if (fp)
		    if (i_title)
		    {
			fprintf(fp, "\n\t%s %s\n", get_race_name(race), SHIP_TYPES);
			fprintf(fp, "    Tech Level %5.2f %5.2f %5.2f %5.2f\n\n",
				drive, weapon, shield, cargo);
		    }
		    else
		    {
			fprintf(fp, "\t%s %5.2f %5.2f %5.2f %5.2f\n",
				get_race_name(race),
				drive, weapon, shield, cargo);
		    }
		fieldPrintStr(fp, 0, "Name", " ", LEFT_JUSTIFY);
		fieldPrintStr(fp, 1, "Drive", " ", LEFT_JUSTIFY);
		fieldPrintStr(fp, 2, "A", " ", LEFT_JUSTIFY);
		fieldPrintStr(fp, 3, "Weap", " ", LEFT_JUSTIFY);
		fieldPrintStr(fp, 4, "Sh", " ", LEFT_JUSTIFY);
		fieldPrintStr(fp, 5, "Cargo", " ", LEFT_JUSTIFY);
		fieldPrintStr(fp, 6, "Mass", " ", LEFT_JUSTIFY);
		fieldPrintStr(fp, 7, "LY/full", " ", LEFT_JUSTIFY);
		fieldPrintStr(fp, 8, "AT", " ", LEFT_JUSTIFY);
		fieldPrintStr(fp, 9, "DA", " ", LEFT_JUSTIFY);
		fieldPrintStr(fp, 10, "Cost", "\n", LEFT_JUSTIFY);
	    }
	    calc_ship_data(i, drive, weapon, shield, cargo, MAX_CARGO);

	    fieldPrintStr(fp, 0, get_ship_name(i), " ", LEFT_JUSTIFY);
	    fieldPrintFloat(fp, 1, ship[i].drive, " ", LEFT_JUSTIFY);
	    fieldPrintInt(fp, 2, ship[i].attacks, " ", LEFT_JUSTIFY);
	    fieldPrintFloat(fp, 3, ship[i].weapon, " ", LEFT_JUSTIFY);
	    fieldPrintFloat(fp, 4, ship[i].shield, " ", LEFT_JUSTIFY);
	    fieldPrintFloat(fp, 5, ship[i].cargo, " ", LEFT_JUSTIFY);
	    fieldPrintFloat(fp, 6, ship[i].mass, " ", LEFT_JUSTIFY);

	    if (ship[i].ly == ship[i].ly_full)
		fieldPrintFloat(fp, 7, ship[i].ly, " ", LEFT_JUSTIFY);
	    else
	    {
		sprintf(tmp, "%.2f/%.2f",
			ship[i].ly,
			ship[i].ly_full);
		fieldPrintStr(fp, 7, tmp, " ", LEFT_JUSTIFY);
	    }
	    if (ship[i].attacks > 1)
	    {
		sprintf(tmp, "%.2f*%d", ship[i].attack_pow, ship[i].attacks);
		fieldPrintStr(fp, 8, tmp, " ", LEFT_JUSTIFY);
	    }
	    else
		fieldPrintFloat(fp, 8, ship[i].attack_pow, " ", LEFT_JUSTIFY);

	    if (ship[i].eff_safe == ship[i].safe) /* Anders - safe not defence */
		fieldPrintFloat(fp, 9, ship[i].safe, " ", LEFT_JUSTIFY);
	    else
	    {
		sprintf(tmp, "%.2f/%.2f",
			ship[i].safe,
			ship[i].eff_safe);     /* Anders - safe not defence */
		fieldPrintStr(fp, 9, tmp, " ", LEFT_JUSTIFY);
	    }
	    fieldPrintFloat(fp, 10, ship[i].cost, " ", LEFT_JUSTIFY);

	    if (fp)
		if (g_print_ship_nums)
		    /* simulation comment too */
		    fprintf(fp, "##S%d\n", i);
		else
		    fprintf(fp, "\n");
	}
    return DONE;
}

int show_test_ship (int i)
{
    char tmp[256];

    sprintf(g_msg,
	    "Drive: %.1f #Attack: %d Weapon: %.1f Shield %.1f Cargo: %.1f",
	    ship[i].drive,
	    ship[i].attacks,
	    ship[i].weapon,
	    ship[i].shield,
	    ship[i].cargo);

    message_print(g_msg);

    sprintf(g_msg, "Mass: %.2f ", ship[i].mass);

    if (ship[i].ly == ship[i].ly_full)
	sprintf(tmp, "Ly: %.2f ", ship[i].ly);
    else
	sprintf(tmp, "Ly: %.2f/%.2f ",
		ship[i].ly,
		ship[i].ly_full);
    strcat(g_msg, tmp);
    sprintf(tmp, "Offense: %.1f ", ship[i].attack_pow);

    if (ship[i].defense == ship[i].eff_defense)
    {
	sprintf(tmp, "SafeGun: %.1f ", ship[i].safe); /* Anders - safe not defence */
    }
    else
    {
	sprintf(tmp, "SafeGun: %.1f/%.1f ", ship[i].safe, ship[i].eff_safe); /* Anders - safe not defence */
    }
    strcat(g_msg, tmp);

    sprintf(tmp, "Eff Cargo: %.1f ", ship[i].eff_cargo);
    strcat(g_msg, tmp);

    sprintf(tmp, "Cost: %.1f", ship[i].cost);
    strcat(g_msg, tmp);

    message_print(g_msg);

    return DONE;
}

int get_ship_id (char *cp_name, int race)
{
    int i;

    /* See if there is a ship of this name already */
    for (i = 0; i < g_max_ships;  i++)
	if (ship[i].set && (ship[i].race == race) &&
		!strcmp(ship[i].name, cp_name))
	    return i;

    return IS_ERR;
}

int get_ship_data (int i_id, float *fp_drive, int *ip_attacks, float *fp_weapon, float *fp_shield, float *fp_cargo)
{
    assert((i_id >= 0 && i_id < g_max_ships));
    assert(ship[i_id].set);

    if (fp_drive) *fp_drive = ship[i_id].drive;
    if (fp_shield) *fp_shield = ship[i_id].shield;
    if (fp_weapon) *fp_weapon = ship[i_id].weapon;
    if (fp_cargo) *fp_cargo = ship[i_id].cargo;
    if (ip_attacks) *ip_attacks = ship[i_id].attacks;

    return OKAY;
}

static float get_ship_eff_defense(int i_id)
{
    assert((i_id >= 0 && i_id < g_max_ships));
    assert(ship[i_id].set);

    return(ship[i_id].eff_defense);
}

static int get_new_ship_id (void)
{
    int i, try;

    for (try = 0; try < 2; try++)
    {
	/* Get first unset slot */
	for (i = 0; i < g_max_ships;  i++)
	    if (!ship[i].set)
		return i;

	ship_alloc();
    }
    abort_program("Can't make any room for new ships!\n"); /* no return! */
    return -1;
}

static char *get_ship_name (int id)
{
    static char text[MAX_SHIP_LEN+1];

    if ((id < 0) || (id >= g_max_ships))
    {
	sprintf(g_msg, "get_ship_name(), line %d : Illegal ship number: %d\n",
		g_line_num, id);
	abort_program(g_msg);
    }
    if (ship[id].name)
    {
	strncpy(text, ship[id].name, MAX_SHIP_LEN);
	text[MAX_SHIP_LEN] = '\0';
	return text;
    }
    return "";
}

int new_ship_data (char *cp_name, float drive, int attacks, float weapon, float shield, float cargo, int race)
{
    int id;
    float tdrive, tweapon, tshield, tcargo;

    if ((id = get_ship_id(cp_name, race)) == IS_ERR)
	id = get_new_ship_id();

    ship[id].set = 1;
    zrealloc_cpy(&ship[id].name, cp_name);

    ship[id].drive = drive;
    ship[id].attacks = attacks;
    ship[id].weapon = weapon;
    ship[id].shield = shield;
    ship[id].cargo = cargo;
    ship[id].race = race;

    get_race_techs(race, CURRENT_TECH, &tdrive, &tweapon, &tshield, &tcargo);
    calc_ship_data(id, tdrive, tweapon, tshield, tcargo, MAX_CARGO);

    return DONE;
}

int set_ship_data (int id, float drive, int attacks, float weapon, float shield, float cargo, int race)
{
    ship[id].drive = drive;
    ship[id].attacks = attacks;
    ship[id].weapon = weapon;
    ship[id].shield = shield;
    ship[id].cargo = cargo;
    ship[id].race = race;
    return DONE;
}

/*
 *  Compute the internals of the specified ship at the
 *  set tech and current load value (quan).  If quan
 *  is MAX_CARGO this means that compute for a full
 *  load and not a partial load.
 */
int calc_ship_data (int id, float tdrive, float tweapon, float tshield, float tcargo, float quan)
{
    float cargo_mass, mass, ly, ly_full, defense, cost;
    float eff_defense, upgrade_cost, current_cost, upgrade_percent;
    float safe,eff_safe; /* Anders - add safe variables */
    float drive, weapon, shield, cargo;
    float rdrive, rweapon, rshield, rcargo;	/* race techs */
    float load;
    int attacks;

    drive = ship[id].drive;
    attacks = ship[id].attacks;
    weapon = ship[id].weapon;
    shield = ship[id].shield;
    cargo = ship[id].cargo;

    get_race_techs(ship[id].race, CURRENT_TECH,
	    &rdrive, &rweapon, &rshield, &rcargo);
    upgrade_cost = 10 *
	(((1 - tdrive / rdrive) * drive) +
	 ((1 - tweapon / rweapon) * weapon_mass(attacks, weapon)) +
	 ((1 - tshield / rshield) * shield) +
	 ((1 - tcargo / rcargo) * cargo));
    current_cost = 10 *
	(drive + weapon_mass(attacks, weapon) + shield + cargo);

    upgrade_percent = upgrade_cost /* Anders - show upgrade in production not percent   / current_cost */;

	if (g_galaxy_version >= 3)
	    mass = drive + shield + cargo + weapon_mass(attacks, weapon);
	else
	    mass = drive + attacks * weapon + shield + cargo;
    drive *= tdrive;
    weapon *= tweapon;
    shield *= tshield;
    /*#define COMPUTE_CARGO(c,t) (g_galaxy_version >= 3 ? ((c)+((c)*(c)/10.0)) : (c*t))
      cargo = COMPUTE_CARGO(cargo,tcargo);*/
    if (g_galaxy_version >= 3)
	cargo = cargo + cargo * cargo / 10.0;
    else
	cargo = cargo * tcargo;
    load = (quan == MAX_CARGO ? cargo : quan);

    if (tcargo > 0)
    {
	if (load > 0)
	{
	    if (g_galaxy_version >= 3)
		cargo_mass = mass + load/tcargo;
	    else
		cargo_mass = mass + load;
	}
	else
	{
	    if (g_galaxy_version >= 3)
		cargo_mass = mass + cargo/tcargo;
	    else
		cargo_mass = mass + cargo;
	}
    }
    else
	cargo_mass = 0;

    ly = (drive * 20.0) / mass;
    defense = shield * BATTLEMAGIC / CBRT(mass);
    safe = eff_safe = ((shield / pow (mass,1.0/3.0) * pow (30.0,1.0/3.0)))/4; /* Anders - calcluate safety against gun  */
    if (cargo_mass > 0.0)
    {
	ly_full = (drive * 20.0) / cargo_mass;
	eff_defense = shield * BATTLEMAGIC / CBRT(cargo_mass);
	eff_safe = ((shield / pow (cargo_mass,1.0/3.0) * pow (30.0,1.0/3.0)))/4; /* Anders - calcluate safety against gun */

    }
    else
    {
	ly_full = ly;
	eff_defense = defense;
    }
    cost = 10 * mass;

    ship[id].mass = mass;
    ship[id].ly = ly;
    ship[id].ly_full = ly_full;
    ship[id].defense = defense;
    ship[id].attack_pow = weapon;
    ship[id].cost = cost;
    ship[id].eff_cargo = cargo;
    ship[id].eff_defense = eff_defense;
    ship[id].eff_safe = safe; /* Anders - store safe gun value */
    ship[id].eff_safe = eff_safe; /* Anders - store safe gun value with cargo */
    ship[id].upgrade_percent = upgrade_percent;
    ship[id].quantity = (quan == MAX_CARGO ? cargo : quan);

    return DONE;
}

static float weapon_mass (int attacks, float weapon)
{
    float mass;

    mass = weapon;
    if (attacks > 1)
	mass += ((attacks-1) * weapon) / 2.0;

    return mass;
}

/*
 *   Special: if fp is 0 then the field lengths for each field
 *    only will be generated using the field* functions.  The result?
 *    the computed lengths of each field.
 *    Must take exactly four fields do print the info starting
 *	 with field i_field.
 */
int write_ship_brief_data (FILE *fp, int id, int i_field)
{
    char line[80];

    if (ship[id].set)
    {
	if (fp)
	    fprintf(fp, "(");

	if (ship[id].ly == ship[id].ly_full)
	    fieldPrintFloat(fp, i_field, ship[id].ly, ",", LEFT_JUSTIFY);
	else
	{
	    sprintf(line, "%.2f/%.2f", ship[id].ly, ship[id].ly_full);
	    fieldPrintStr(fp, i_field, line, ",", LEFT_JUSTIFY);
	}
	if (ship[id].attacks > 1)
	{
	    sprintf(line, "%.2f*%d", ship[id].attack_pow, ship[id].attacks);
	    fieldPrintStr(fp, i_field+1, line, ",", LEFT_JUSTIFY);
	}
	else
	    fieldPrintFloat(fp, i_field+1, ship[id].attack_pow, ",", LEFT_JUSTIFY);
	fieldPrintFloat(fp, i_field+2, ship[id].eff_defense, ",", LEFT_JUSTIFY);
	fieldPrintFloat(fp, i_field+3, ship[id].eff_cargo, ") ", LEFT_JUSTIFY);

	if (g_galaxy_version >= 3)
	    fieldPrintFloat(fp, i_field+4, ship[id].upgrade_percent, " ", LEFT_JUSTIFY);
    }
    return DONE;
}

float get_ship_mass (int id)
{
    return ship[id].mass;
}

#define LEVEL1     0
#define LEVEL2  2000
#define LEVEL3  4000
#define LEVEL4  6000
#define UNSET  10000

static int shipval (struct ships *e1)
{
    if (!e1->set)
	return UNSET;

    if (e1->cargo > 0)
	return e1->mass + e1->cargo + LEVEL1;
    if (e1->drive == 0)
	return e1->mass + LEVEL4;
    if (e1->attacks * e1->weapon == 0)
	return e1->mass + LEVEL2;
    return e1->mass + LEVEL3;
}

static int shippos ( const void *e1, const void *e2)
{
    int val1, val2;

    val1 = shipval((struct ships *)e1);
    val2 = shipval((struct ships *)e2);

    return val1 - val2;
}

void sort_ships (void)
{
    qsort((char *) ship, g_max_ships, sizeof(struct ships), shippos);
}


int ship_cankill (int i_attack_id, int i_target_id, float f_attack_tweapon, float f_target_tshield, float f_target_tcargo, float f_target_quantity)
{
    float defense, ratio;

    if ((ship[i_attack_id].attacks == 0) || (ship[i_attack_id].weapon == 0))
	return 0;

    calc_ship_data(i_target_id,
	    1.0, 1.0, f_target_tshield, f_target_tcargo,
	    f_target_quantity);

    defense = get_ship_eff_defense(i_target_id);

    if (!defense)
	return 1;

    ratio = ship[i_attack_id].weapon * f_attack_tweapon / defense;

    return ratio > .251;
}

int ship_kill (int i_attack_id, int i_target_id, float f_attack_tweapon, float f_target_tshield, float f_target_tcargo, float f_target_quantity)
{
    double defense, ratio;

    if ((ship[i_attack_id].attacks == 0) || (ship[i_attack_id].weapon == 0))
	return 0;
    calc_ship_data(i_target_id,
	    1.0, 1.0, f_target_tshield, f_target_tcargo,
	    f_target_quantity);
    defense = get_ship_eff_defense(i_target_id);

    if (!defense)
	return 1;
    ratio = ship[i_attack_id].weapon * f_attack_tweapon / defense;
    return ratio > pow(4.0, frand2());
}

static double frand2 (void)
{
    return (RAND() % 20000 - 10000) * .0001;
}
