/*
 * File: simmain.c
 * Author: Douglas Selph
 * Maintained by: Robin Powell
 * $Id: battlemain.c,v 1.4 1997/12/14 15:13:23 rlpowell Exp $
 */
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <assert.h>
#include <sys/types.h>
#include <unistd.h>
#include "common.h"
#include "data.h"

#include "version.h"
#include "ship.h"
#include "util.h"
#include "battle.h"
#include "get.h"
#include "group.h"

static void trapfunc( int signal );
static int g_seed;
extern int g_print_group_tech;

static int check_args (int ac, char **av);
static int loop (void);
static int usage (void);

/* Defined in screen.c which is not used by battle */
int g_starting_cols;

void main (int ac, char **av)
{
    g_seed = getpid();
    g_print_group_tech = 0;

    signal(SIGINT, trapfunc);

    check_args(ac, av);
    SRAND(g_seed);
    printf("Initial seed = %d\n", g_seed);

    check_version();
    init_ships();

    loop();
}

static int check_args (int ac, char **av)
{
    int i /*, err ??? not used*/;

	if (ac > 1)
	    for (i=1;i<ac;i++)
	    {
		if (av[i][0]!='-')
		{
		    usage();
		    exit(1);
		}
		switch (av[i][1])
		{
		    case 's' :
			if (i+1 >= ac)
			{
			    printf("Expected number following -s\n");
			    usage();
			    exit(1);
			}
			else if (strIsInt(av[i+1]))
			    g_seed = atoi(av[i+1]);
			else
			{
			    printf("Expected number following -s\n");
			    printf("  Found '%s'\n", av[i+1]);
			    usage();
			    exit(1);
			}
			break;
		    default:
			usage();
			exit(1);
		}
	    }
    return DONE;
}

static int usage (void)
{
    fprintf(stderr,"Usage : battle [-s #]\n");
    fprintf(stderr,"  -s # : set starting random seed\n");
    return DONE;
}

static void trapfunc( int signal )
{
    printf("Quit\n");
    exit(1);
}

static int loop (void)
{
    char line[126];
    char *name, *s;
    int go, num_fights, i;

    for (go = 1; go;)
    {
	printf("Cmd> ");

	if (gets(line) == NULL)
	    break;

	s = line;

	switch (*(s++))
	{
	    case 'b' :
		name = askstr("output filename");
		if (strlen(name) == 0)
		    name = 0;
		do_battle(name);
		break;
	    case 'B' :
		num_fights = getd("Number of fights");
		for (i = 0; i < num_fights; i++)
		{
		    printf("Processing battle %d\n", i+1);
		    do_battle(0);
		}
		break;
	    case 'p' :
		write_battle_data(0);
		break;
	    case 'r' :
		name = askstr("filename");
		read_battle_data(name);
		break;
	    case 'w' :
		name = askstr("filename");
		write_battle_data(name);
		break;
	    case 'q' :
		go = 0;
		break;
	    case 'g' :
		battle_init_groups();
		write_all_races_groups(stdout);
		break;
	    case 's' :
		write_all_races_ships(stdout);
		break;
	    case '?' :
		printf(" b  - do battle\n");
		printf(" B  - simulate N battles\n");
		printf(" p  - print battle data\n");
		printf(" r  - read battle data from file\n");
		printf(" w  - write battle data to file\n");
		printf(" g  - print battle data in group format\n");
		printf(" s  - print ship data\n");
		printf(" q  - quit\n");
		break;
	    default :
		printf("Type '?' for a list of commands.\n");
	}
    }
    return DONE;
}

void abort_program (char *cp_msg)
{
    strRmReturn(cp_msg);
    puts(cp_msg);
    exit(1);
}

void message_print(char *cp_msg)
{
    strRmReturn(cp_msg);
    puts(cp_msg);
}
