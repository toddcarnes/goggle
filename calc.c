/*
 * File: calc.c
 * Author: Douglas Selph
 * Maintained by: Robin Powell
 * $Id: calc.c,v 1.4 1997/12/11 00:43:44 rlpowell Exp $
 */
#include <stdio.h>
#include <curses.h>
#include <signal.h>
#include <assert.h>
#include <math.h>
#include "common.h"
#include "screen.h"
#include "data.h"

#include "group.h"
#include "prompt.h"
#include "main.h"
#include "ship.h"
#include "planetshow.h"
#include "planet.h"
#include "calc.h"

/**
 **  Local Structures and Defines
 **/
extern char g_msg[MAX_MSG];
extern int g_prompt_flags;
extern int g_prompt_start_row;

static struct global_calc
{
    int inited;
    float tdrive, tweapon, tshield, tcargo;
    float drive, attack, shield, cargo;
    float mass, ly, defense;
    int numattack;
} g;

static int compute_kill (float attack, float tech_attack, float defense);
static int compute_drive_func (PromptObj *sp_prompts, int i_num,int  i_row, int *ip_redraw);
static int compute_shield_func (PromptObj *sp_prompts, int i_num, int i_row, int *ip_redraw);
static int compute_kill_1_func (PromptObj *sp_prompts, int i_num, int i_row, int *ip_redraw);
static int compute_kill_2_func (PromptObj *sp_prompts, int i_num, int i_row, int *ip_redraw);
static int compute_test_ship (PromptObj *sp_prompts, int i_num, int i_row, int *ip_redraw);

/**
 ** Functions:
 **/

static void init_calc (void)
{
    int race = 1;

    if (!g.inited)
    {
	g.inited = 1;
	get_race_techs(race, CURRENT_TECH,
		&g.tdrive, &g.tweapon, &g.tshield, &g.tcargo);
	g.drive = 1.0;
	g.numattack = 1;
	g.attack = 1.0;
	g.shield = 1.0;
	g.cargo = 0.0;
	g.mass = 3;
	g.ly = 0;
    }
}

int CalcDriveStat (void)
{
    PromptObj prompt[4];

    init_calc();

    prompt[0].type = GM_FLOAT;
    prompt[0].prompt = "Mass: ";
    prompt[0].u.fval = g.mass;

    prompt[1].type = GM_FLOAT;
    prompt[1].prompt = "Light Years: ";
    prompt[1].u.fval = g.ly;

    prompt[2].type = GM_FLOAT;
    prompt[2].prompt = "Drive Tech: ";
    prompt[2].u.fval = g.tdrive;

    if (ScreenGetMany(prompt, 3, compute_drive_func) != DONE)
	return NOT_OKAY;

    g.mass = prompt[0].u.fval;
    g.ly = prompt[1].u.fval;
    g.tdrive = prompt[2].u.fval;

    return DONE;
}

/*
 * Called each time we move to a new row
 */
static int compute_drive_func (PromptObj *sp_prompts, int i_num,int  i_row, int *ip_redraw)
    /* sp_prompts	array of prompts
       i_num		number of prompts
       i_row		current row cursor is on
       ip_redraw */
{
    float ly, mass, tech;

    if (i_row < 4)
	return DONE;

    mass = sp_prompts[0].u.fval;
    ly = sp_prompts[1].u.fval;
    tech = sp_prompts[2].u.fval;

    g.drive = ((ly * mass) / 20.0) / tech;

    sprintf(g_msg, "Drive power = %5.2f\n", g.drive);
    message_print(g_msg);

    return DONE;
}

int CalcShieldStat (void)
{
    PromptObj prompt[3];

    init_calc();

    prompt[0].type = GM_FLOAT;
    prompt[0].prompt = "Mass";
    prompt[0].u.fval = g.mass;

    prompt[1].type = GM_FLOAT;
    prompt[1].prompt = "Defense Power";
    prompt[1].u.fval = g.defense;

    prompt[2].type = GM_FLOAT;
    prompt[2].prompt = "Shield Tech";
    prompt[2].u.fval = g.tshield;

    if (ScreenGetMany(prompt, 3, compute_shield_func) != DONE)
	return NOT_OKAY;

    g.mass = prompt[0].u.fval;
    g.defense = prompt[1].u.fval;
    g.tshield = prompt[2].u.fval;

    return DONE;
}

/*
 * Called each time we move to a new row
 */
static int compute_shield_func (PromptObj *sp_prompts, int i_num, int i_row, int *ip_redraw)
    /* sp_prompts	array of prompts
       i_num		number of prompts
       i_row		current row cursor is on
       ip_redraw */
{
    float mass, def, tech;

    if (i_row < 4)
	return DONE;

    mass = sp_prompts[0].u.fval;
    def = sp_prompts[1].u.fval;
    tech = sp_prompts[2].u.fval;

    g.shield = (CBRT((double) mass) / 3.107 * def) / tech;

    sprintf(g_msg, "Shield power = %5.2f\n", g.shield);
    message_print(g_msg);

    return DONE;
}


/* compute percentage of kill based on basic attack and shield values */
int CalcKillPercentage (void)
{
    PromptObj prompt[5];
    int pow_flag;

    init_calc();

    prompt[0].type = GM_LOGICAL;
    prompt[0].prompt = "Enter Defensive Power?";
    prompt[0].u.lval = TRUE;

    if (ScreenGetMany(prompt, 1, 0) != DONE)
	return NOT_OKAY;

    pow_flag = prompt[0].u.lval;

    prompt[0].type = GM_FLOAT;
    prompt[0].prompt = "Attack Strength";
    prompt[0].u.fval = 0;

    prompt[1].type = GM_FLOAT;
    prompt[1].prompt = "Attach Tech";
    prompt[1].u.fval = g.tweapon;

    if (pow_flag)
    {
	prompt[2].type = GM_FLOAT;
	prompt[2].prompt = "Defensive Power";
	prompt[2].u.fval = 0.0;

	if (ScreenGetMany(prompt, 3, compute_kill_1_func) != DONE)
	    return NOT_OKAY;
    }
    else
    {
	prompt[2].type = GM_FLOAT;
	prompt[2].prompt = "Defense Mass";
	prompt[2].u.fval = g.mass;

	prompt[3].type = GM_FLOAT;
	prompt[3].prompt = "Shield Strength";
	prompt[3].u.fval = g.shield;

	prompt[4].type = GM_FLOAT;
	prompt[4].prompt = "Shield Tech";
	prompt[4].u.fval = g.tshield;

	if (ScreenGetMany(prompt, 5, compute_kill_2_func) != DONE)
	    return NOT_OKAY;
    }
    return DONE;
}

/*
 * Called each time we move to a new row
 */
static int compute_kill_1_func (PromptObj *sp_prompts, int i_num, int i_row, int *ip_redraw)
    /* sp_prompts	array of prompts
       i_num		number of prompts
       i_row		current row cursor is on
       ip_redraw */
{
    float defense, attack, tech_attack;

    if (i_row < 4)
	return DONE;

    attack = sp_prompts[0].u.fval;
    tech_attack = sp_prompts[1].u.fval;
    defense = sp_prompts[2].u.fval;

    compute_kill(attack, tech_attack, defense);

    return DONE;
}

/*
 * Called each time we move to a new row
 */
static int compute_kill_2_func (PromptObj *sp_prompts, int i_num, int i_row, int *ip_redraw)
    /* sp_prompts	array of prompts
       i_num		number of prompts
       i_row		current row cursor is on
       ip_redraw */
{
    float mass, defense, shield, tech_shield, tech_attack, attack;

    if (i_row < 6)
	return DONE;

    attack = sp_prompts[0].u.fval;
    tech_attack = sp_prompts[1].u.fval;
    mass = sp_prompts[2].u.fval;
    shield = sp_prompts[3].u.fval;
    tech_shield = sp_prompts[4].u.fval;

    defense = (shield*tech_shield) / CBRT((double) mass) * 3.107;

    compute_kill(attack, tech_attack, defense);

    return DONE;
}

static int compute_kill (float attack, float tech_attack, float defense)
{
    float percent, ratio;

    attack *= tech_attack;

    ratio = attack / defense;

    if (ratio == 1)
	percent = 50.0;
    else if (attack > defense)
    {
	percent = ((50.0/3.0) * (ratio-1)) + 50.0;
	if (percent > 100)
	    percent = 100;
    }
    else /* defense > attack */
    {
	/* do 100 - (percent to miss) */
	ratio = defense / attack;
	percent = 100 - (((50.0/3.0) * (ratio-1)) + 50.0);
	if (percent < 0)
	    percent = 0;
    }
    sprintf(g_msg, "Attack Pow %.2f, Defense Pow %.2f, Ratio %.2f, Kill %% = %.2f%%\n",
	    attack, defense, ratio, percent);
    message_print(g_msg);

    return DONE;
}

int CalcTestShip (void)
{
    PromptObj prompt[9];

    init_calc();

    prompt[0].type = GM_FLOAT;
    prompt[0].prompt = "Drive Tech: ";
    prompt[0].u.fval = g.tdrive;

    prompt[1].type = GM_FLOAT;
    prompt[1].prompt = "Weapon Tech: ";
    prompt[1].u.fval = g.tweapon;

    prompt[2].type = GM_FLOAT;
    prompt[2].prompt = "Shield Tech: ";
    prompt[2].u.fval = g.tshield;

    prompt[3].type = GM_FLOAT;
    prompt[3].prompt = "Cargo Tech: ";
    prompt[3].u.fval = g.tcargo;

    /*
       prompt[4].type = GM_FLOAT;
       prompt[4].prompt = "Total Mass : ";
       prompt[4].u.fval = g.mass;

       prompt[5].type = GM_FLOAT;
       prompt[5].prompt = "Speed : ";
       prompt[5].u.fval = g.ly;

       prompt[6].type = GM_FLOAT;
       prompt[6].prompt = "Defense : ";
       prompt[6].u.fval = g.defense;

       prompt[7].type = GM_FLOAT;
       prompt[7].prompt = "Eff Cargo : ";
       prompt[7].u.fval = g.eff_cargo;
     */

    prompt[4].type = GM_FLOAT;
    prompt[4].prompt = "Drive Stat: ";
    prompt[4].u.fval = g.drive;

    prompt[5].type = GM_INTEGER;
    prompt[5].prompt = "Number Attacks: ";
    prompt[5].u.ival = g.numattack;

    prompt[6].type = GM_FLOAT;
    prompt[6].prompt = "Attack Stat: ";
    prompt[6].u.fval = g.attack;

    prompt[7].type = GM_FLOAT;
    prompt[7].prompt = "Shield Stat: ";
    prompt[7].u.fval = g.shield;

    prompt[8].type = GM_FLOAT;
    prompt[8].prompt = "Cargo Stat: ";
    prompt[8].u.fval = g.cargo;

    if (ScreenGetMany(prompt, 9, compute_test_ship) != DONE)
	return NOT_OKAY;

    /* remember entered values */
    g.tdrive = prompt[0].u.fval;
    g.tweapon = prompt[1].u.fval;
    g.tshield = prompt[2].u.fval;
    g.tcargo = prompt[3].u.fval;
    g.drive = prompt[4].u.fval;
    g.numattack = prompt[5].u.ival;
    g.attack = prompt[6].u.fval;
    g.shield = prompt[7].u.fval;
    g.cargo = prompt[8].u.fval;
    g.mass = get_ship_mass(TEST_SLOT);

    return DONE;
}

/*
 * Called each time we move to a new row
 */
static int compute_test_ship (PromptObj *sp_prompts, int i_num, int i_row, int *ip_redraw)
    /* sp_prompts	array of prompts
       i_num		number of prompts
       i_row		current row cursor is on
       ip_redraw */
{
    float tdrive, tweapon, tshield, tcargo;
    float drive, weapon, shield, cargo;
    int attacks;

    if (i_row < 10)
	return DONE;

    tdrive = sp_prompts[0].u.fval;
    tweapon = sp_prompts[1].u.fval;
    tshield = sp_prompts[2].u.fval;
    tcargo = sp_prompts[3].u.fval;
    drive = sp_prompts[4].u.fval;
    attacks = sp_prompts[5].u.ival;
    weapon = sp_prompts[6].u.fval;
    shield = sp_prompts[7].u.fval;
    cargo = sp_prompts[8].u.fval;

    set_ship_data(TEST_SLOT, drive, attacks, weapon, shield, cargo, TEST_RACE);
    calc_ship_data(TEST_SLOT, tdrive, tweapon, tshield, tcargo, MAX_CARGO);

    show_test_ship(TEST_SLOT);

    return DONE;
}

/*
 * Calculate distance between two planets
 */
int CalcPlanetDist (void)
{
    PromptObj prompt[2];
    char name1[100];
    char name2[100];
    double d;
    int p1, p2;

    g_prompt_start_row = 4;

    if ((p1 = PlanetAtMark()) >= 0)
	strcpy(name1, get_planet_name(p1));
    else
    {
	name1[0] = '\0';
	g_prompt_start_row = 1;
    }
    if ((p2 = PlanetAtCursor(CLOSEST)) >= 0)
	strcpy(name2, get_planet_name(p2));
    else
    {
	name2[0] = '\0';
	g_prompt_start_row = 2;
    }
    prompt[0].type = GM_STRING;
    prompt[0].prompt = "Planet 1 : ";
    prompt[0].u.sval = name1;

    prompt[1].type = GM_STRING;
    prompt[1].prompt = "Planet 2 : ";
    prompt[1].u.sval = name2;

    g_prompt_flags = PR_START_ROW;

    if (ScreenGetMany(prompt, 2, NULL) != DONE)
	return NOT_OKAY;

    if ((p1 = translate_planet(name1)) < 0)
	return NOT_OKAY;

    if ((p2 = translate_planet(name2)) < 0)
	return NOT_OKAY;

    if ((d = planet_dist(p1, p2)) <= 0)
	return NOT_OKAY;

    strcpy(name1, get_planet_name(p1));
    strcpy(name2, get_planet_name(p2));
    sprintf(g_msg, "Distance between %s and %s is %g light years.",
	    name1, name2, d);
    message_print(g_msg);

    return DONE;
}

/*
 * Try to compute the number of ships
 * a planet may produce.
 */
int CalcNumShips (void)
{
    PromptObj prompt[2];
    char name[100];
    float numships, mass;
    int p1;

    if ((p1 = PlanetAtCursor(CLOSEST)) >= 0)
	strcpy(name, get_planet_name(p1));
    else
	name[0] = '\0';
    prompt[0].type = GM_STRING;
    prompt[0].prompt = "Planet : ";
    prompt[0].u.sval = name;

    prompt[1].type = GM_FLOAT;
    prompt[1].prompt = "Ship Mass : ";
    prompt[1].u.fval = 1.0;

    if (ScreenGetMany(prompt, 2, NULL) != DONE)
	return NOT_OKAY;

    if ((p1 = translate_planet(name)) < 0)
	return NOT_OKAY;

    mass = prompt[1].u.fval;

    numships = compute_num_ships(p1, mass);
    /* Anders - Print out fraction of ships produces */
    sprintf(g_msg, "I estimate planet %s producing %.2f ships of mass %.2f.", 
	    get_planet_name(p1), numships, mass);
    message_print(g_msg);

    return DONE;
}

/*
 * Try to estimate the amount of the given resource
 * a planet may produce.
 */
int CalcEstimate (int i_what)
{
    PromptObj prompt[1];
    char name[100];
    float amt;
    int p1;
    char *str;

    if ((p1 = PlanetAtCursor(CLOSEST)) >= 0)
	strcpy(name, get_planet_name(p1));
    else
	name[0] = '\0';
    prompt[0].type = GM_STRING;
    prompt[0].prompt = "Planet : ";
    prompt[0].u.sval = name;

    if (ScreenGetMany(prompt, 1, NULL) != DONE)
	return NOT_OKAY;

    if ((p1 = translate_planet(name)) < 0)
	return NOT_OKAY;

    amt = compute_estimate(p1, i_what);

    switch (i_what)
    {
	case PR_CAP :
	    str = "Capital";
	    break;
	case PR_MAT :
	    str = "Raw Materials";
	    break;
	case PR_DRIVE :
	    str = "Drive Tech";
	    break;
	case PR_WEAPONS :
	    str = "Weapons Tech";
	    break;
	case PR_SHIELDS :
	    str = "Shields Tech";
	    break;
	case PR_CARGO :
	    str = "Cargo Tech";
	    break;
	default :
	    str = "";
	    break;
    }
    sprintf(g_msg, "I estimate planet %s producing %.2f %s.",
	    get_planet_name(p1), amt, str);
    message_print(g_msg);

    return DONE;
}
