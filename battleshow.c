/*
 * File: battleshow.c
 * Author: Douglas Selph
 * Maintained by: Robin Powell
 * $Id: battleshow.c,v 1.3 1997/12/11 00:43:44 rlpowell Exp $
 */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <curses.h>
#include "common.h"
#include "screen.h"
#include "data.h"

#include "fileshow.h"
#include "main.h"
#include "util.h"
#include "group.h"
#include "planetshow.h"
#include "prompt.h"
#include "battle.h"
#include "battleshow.h"

/**
 **  Local Structures and Defines
 **/
extern int g_cur_display;
extern char g_msg[MAX_MSG];

/**
 ** Functions:
 **/

/*
 *  Purpose: specify a group, ship, or planet
 *   to be used in the current battle simulation.
 *  This function is context sensitive:
 *
 *	- If the current cursor position is over a planet,
 *        then all the groups at the planet are placed into
 *	  the current simulation.
 *      - If the current cursor position is over a group line,
 *        then that group is placed into the current simulation.
 *	  A prompt dialog appears allowing the user to re-specify
 *	  the number of ships that belong in the simulation group.
 *        The default would be the number in the original group.
 *      - If the current cursor position is over a ship type
 *        line, then a prompt dialog appears asking the user
 *        to specify the tech values, quantity (if cargo ship),
 *	  and number of ships in the simulation group.
 */
int BattleUse (void)
{
    char comment[64];
    int pl;
    int num, index, code;

    if (g_cur_display == DISPLAY_FILE)
    {
	/* Get the special comment character at the cursor */
	if (FileRetrieveCommentAtCursor(comment) != OKAY)
	{
	    sprintf(g_msg, "Error: No simulation info on this line.");
	    message_print(g_msg);
	    return NOT_OKAY;
	}
	/*
	 *  Expected comments:
	 *
	 *	G###	-- group line with internal index value of ###.
	 *        S###	-- ship line with internal index value of ###.
	 */
	code = comment[0];
	if (!strIsInt(comment+1))
	{
	    sprintf(g_msg, "Internal Error: simulation comment simulation %s unrecognized.",
		    comment);
	    message_print(g_msg);
	    return NOT_OKAY;
	}
	else
	    index = atoi(comment+1);
	if (code == 'G')
	{
	    num = place_group_in_sim(index);

	    sprintf(g_msg, "Added %d ships to current simulation.", num);
	    message_print(g_msg);
	}
	else if (code == 'S')
	    message_print("Not implemented");
    }
    else /* assume DISPLAY_MAP */
    {
	if ((pl = PlanetAtCursor(QUERY)) < 0)
	    return NOT_OKAY;	/* message already printed */

	/* dump all groups at planet into simulation */
	num = place_groups_at_planet_in_sim(RACE_ALL, pl);

	sprintf(g_msg, "Added %d groups to current simulation.", num);
	message_print(g_msg);
    }
    return DONE;
}


/*
 *  Show the results of the last simulation,
 *  or, show the current groups in the simulation:
 *  both do the same thing.
 *
 *  Return file_id.
 */
int BattleDisplayResults (FILE *fp)
{
    write_battle_data_raw(fp, FALSE);
    return DONE;
}

/*
 *  Run a battle.
 */
int BattleRun (void)
{
    PromptObj prompt[2];
    int i, num_fights;

    prompt[0].type = GM_INTEGER;
    prompt[0].prompt = "Number Of Simulations : ";
    prompt[0].u.ival = 1;

    if (ScreenGetMany(prompt, 1, 0) != DONE)
	return ABORT;

    num_fights = prompt[0].u.ival;

    for (i = 0; i < num_fights; i++)
    {
	sprintf(g_msg, "Processing battle %d\n", i+1);
	message_print(g_msg);
	do_battle(0);
    }
    return DONE;
}

int BattleSaveData (void)
{
    PromptObj prompt[2];
    char filename[256];

    prompt[0].type = GM_STRING;
    prompt[0].prompt = "Filename : ";
    strcpy(filename, "save.fight");
    prompt[0].u.sval = filename;

    if (ScreenGetMany(prompt, 1, 0) != DONE)
	return ABORT;

    write_battle_data(filename);

    return DONE;
}

int BattleReadData (void)
{
    PromptObj prompt[2];
    char filename[256];

    prompt[0].type = GM_STRING;
    prompt[0].prompt = "Filename : ";
    strcpy(filename, "save.fight");
    prompt[0].u.sval = filename;

    if (ScreenGetMany(prompt, 1, 0) != DONE)
	return ABORT;

    read_battle_data(filename);

    return DONE;
}
