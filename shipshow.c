/*
 * File: shipshow.c
 * Author: Douglas Selph
 * Maintained by: Robin Powell
 * $Id: shipshow.c,v 1.3 1997/12/11 00:43:44 rlpowell Exp $
 */

#include <stdio.h>
#include <stdlib.h>
#include <curses.h>
#include <signal.h>
#include <assert.h>
#include "common.h"
#include "screen.h"
#include "data.h"

#include "util.h"
#include "ship.h"
#include "prompt.h"
#include "group.h"
#include "shipshow.h"

/**
 **  Local Structures and Defines
 **/
extern int g_prompt_flags;
extern int g_prompt_start_row;
extern char g_msg[MAX_MSG];

static int get_ship_tech (int race);

/**
 ** Functions:
 **/

/*
 * Returns id of race we showed for. (>= 0)
 */
int print_ships (FILE *fp)
{
    int race, tech;

    if ((race = get_ship_race()) < 0)
	return -1;

    if ((tech = get_ship_tech(race)) < 0)
	return -1;

    write_all_ships(fp, race, tech);

    return race;
}

int get_ship_race (void)
{
    char **list, *prompt;
    int i, which, num, subnum;

    if ((num = get_num_races()) == 1)
	return 0;

    list = (char **) CALLOC(num, sizeof(char *));

    subnum = 0;

    for (i = 0; i < num; i++)
	zalloc_cpy(&(list[subnum++]), get_race_name(i));
    prompt = "  Select A Race";

    if (subnum < 2)
	which = 0;
    else
	which = ScreenGetOneOfMany(list, subnum, prompt);

    for (i = 0; i < num; i++)
	if (which-- <= 0)
	{
	    which = i;
	    break;
	}

    for (i = 0; i < subnum; i++)
	free(list[i]);

    free(list);

    return which;
}

static int get_ship_tech (int race)
{
    PromptObj prompt[4];
    float drive, weapon, shield, cargo;

    get_race_techs(race, CURRENT_TECH, &drive, &weapon, &shield, &cargo);

    prompt[0].type = GM_FLOAT;
    prompt[0].prompt = "Drive Tech: ";
    prompt[0].u.fval = drive;

    prompt[1].type = GM_FLOAT;
    prompt[1].prompt = "Weapon Tech: ";
    prompt[1].u.fval = weapon;

    prompt[2].type = GM_FLOAT;
    prompt[2].prompt = "Shield Tech: ";
    prompt[2].u.fval = shield;

    prompt[3].type = GM_FLOAT;
    prompt[3].prompt = "Cargo Tech: ";
    prompt[3].u.fval = cargo;

    g_prompt_flags = PR_START_ROW;
    g_prompt_start_row = 6;

    if (ScreenGetMany(prompt, 4, 0) != DONE)
	return -1;

    drive = prompt[0].u.fval;
    weapon = prompt[1].u.fval;
    shield = prompt[2].u.fval;
    cargo = prompt[3].u.fval;

    set_race_techs(race, OVERRIDE_TECH, drive, weapon, shield, cargo);

    return OVERRIDE_TECH;
}
