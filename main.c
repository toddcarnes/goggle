/*
 * File: main.c
 * Author: Douglas Selph
 * Maintained by: Robin Powell
 * $Id: main.c,v 1.10 1998/06/19 20:58:00 rlpowell Exp rlpowell $
 */
#include <stdio.h>
#include <string.h>
#include <curses.h>
#include <signal.h>
#include <assert.h>
#include <unistd.h>
#include "common.h"
#include "screen.h"
#include "data.h"

#include "version.h"
#include "util.h"
#include "msg.h"
#include "menu.h"
#include "planet.h"
#include "planetshow.h"
#include "battle.h"
#include "battleshow.h"
#include "group.h"
#include "ship.h"
#include "shipshow.h"
#include "fileshow.h"
#include "prompt.h"
#include "readdata.h"
#include "errno.h"
#include "calc.h"
#include "route.h"
#include "main.h"

static int check_args (int ac, char **av);
static void Redisplay (void);
static int main_update_screen (void);
static int save_tmp_file (char *file);
static int usage (void);
static int DisplayInfo (int i_what);
static int Customize (void);
static int CustMapLabels (void);

/**
 **  Local Structures and Defines
 **/
#define MAJOR_REVISION "2"
#define MINOR_REVISION "8"

static void trapfunc ( int signal );
static int main_open();
static int main_save_data();
static int main_save_map();
static int main_quit();
static int main_read();
static int main_view();
static int main_calc();
static int main_calc_produce();
static int main_display();
/* static int main_split(); ??? never used */
static int main_callback();
static int main_cust();
static int main_battle();
static int main_redraw();
static int main_help();

static char *HelpFile (char *cp_file);

#define ID_FILE              0x00
#define ID_OPEN_FILE         0x01
#define ID_READ_TURN         0x02
#define ID_SAVE_DATA         0x03
#define ID_SAVE_MAP          0x04
#define ID_QUIT              0x0d
#define ID_VIEW              0x20
#define ID_VIEW_MARK         0x21
#define ID_VIEW_ZOOM         0x21
#define ID_VIEW_UNZOOM       0x22
#define ID_VIEW_RECENTER     0x23
#define ID_VIEW_REPOS        0x24
#define ID_VIEW_COORD        0x25
#define ID_VIEW_AUTOPOS      0x26
#define ID_CALC              0x30
#define ID_CALC_DIST         0x31
#define ID_CALC_DRIVE        0x33
#define ID_CALC_SHIELD       0x34
#define ID_CALC_KILL         0x35
#define ID_CALC_TEST_SHIP    0x36
#define ID_CALC_PRODUCE      0x37
#define ID_SHOW              0x40
#define ID_SHOW_GROUPS       0x43
#define ID_SHOW_ALL_GROUPS   0x44
#define ID_SHOW_ROUTES       0x45
#define ID_SHOW_PLANETS      0x46
#define ID_SHOW_SHIPS        0x47
#define ID_SHOW_DIST_ALL     0x48
#define ID_SHOW_MSG          0x49
#define ID_SHOW_REMOVE	     0x4a
#define ID_SHOW_PREV         0x4b
#define ID_SHOW_NEXT         0x4c
#define ID_OPTIONS           0x50
#define ID_OPTIONS_CUST      0x51
#define ID_OPTIONS_MAP       0x52
#define ID_OPTIONS_COL       0x54
#define ID_OPTIONS_TECH      0x55
#define ID_BATTLE            0x60
#define ID_BATTLE_USE        0x61
#define ID_BATTLE_SHOW       0x62
#define ID_BATTLE_RUN_ONE    0x63
#define ID_BATTLE_RUN_MULTI  0x64
#define ID_BATTLE_CLEAR      0x65
#define ID_BATTLE_SAVE       0x66
#define ID_BATTLE_READ       0x67
#define ID_HELP              0x90
#define ID_HELP_GENERAL      0x91
#define ID_HELP_CMDS         0x92
#define ID_HELP_SETUP        0x93
#define ID_HELP_BUGS         0x94
#define ID_HELP_KEYS         0x95
#define ID_HELP_CODE         0x96

#define MAX_ITEMS 60
/*
   int id;				-- id to indetify this item --
   int top;				-- pulldown menu identifier --
   char *name;				-- name of item --
   char *accl;				-- keycode accelerator --
   int (*callback)();			-- callback to call when executed --
   pointer arg;			-- passed argument to call --
   Boolean is_pulldown;		-- top level name for pulldown --
 */
static itemData g_itemlist[MAX_ITEMS] =
{
    { ID_FILE, 	 	ID_FILE, "(F)ile", "F", 0, 0, TRUE },
    { ID_READ_TURN, 	ID_FILE, 
	"Read Turn Report", "ft", main_read, 0, FALSE },
    { ID_OPEN_FILE, 	ID_FILE, "Open File", "fo", main_open, 0, FALSE },
    { ID_SAVE_DATA, 	ID_FILE, "Save Data", "fs", main_save_data, 0, FALSE },
    { ID_SAVE_MAP, 	ID_FILE, "Save Map", "fm", main_save_map, 0, FALSE },
    { ID_QUIT, 	 	ID_FILE, "Quit", "Q", main_quit, 0, FALSE },
    { ID_VIEW, 	     	ID_VIEW, "(V)iew", "V", 0, 0, TRUE },
    { ID_VIEW_MARK, 	ID_VIEW, "Place Mark", "m", main_callback, 0, FALSE },
    { ID_VIEW_ZOOM,  	ID_VIEW, "Zoom", "z", main_view, 0, FALSE },
    { ID_VIEW_UNZOOM,  	ID_VIEW, "Unzoom", "u", main_view, 0, FALSE },
    { ID_VIEW_REPOS,    ID_VIEW, 
	"Center Around Planet", "vp", main_view, 0, FALSE },
    { ID_VIEW_RECENTER, ID_VIEW, 
    	"Center Around Cursor", "vc", main_view, 0, FALSE },
    { ID_VIEW_AUTOPOS,  ID_VIEW, 
	"Move To Closest Planet", ".", main_view, 0, FALSE },
    { ID_SHOW, 	  	ID_SHOW, "(S)how", "S", 0, 0, TRUE },
    { ID_SHOW_GROUPS,   ID_SHOW, "Planet Info", "i", main_display, 0, FALSE },
    { ID_SHOW_ALL_GROUPS,ID_SHOW, "Groups", "sg", main_display, 0, FALSE },
    { ID_SHOW_PLANETS,  ID_SHOW, "Planets", "sp", main_display, 0, FALSE },
    { ID_SHOW_ROUTES,   ID_SHOW, "Routes", "sr", main_display, 0, FALSE },
    { ID_SHOW_SHIPS,    ID_SHOW, "Ships", "ss", main_display, 0, FALSE },
    { ID_SHOW_DIST_ALL,	ID_SHOW, 
	"Distance from all planets", "sa", main_display, 0, FALSE },
    { ID_SHOW_MSG,	ID_SHOW, "Messages", "sm", main_display, 0, FALSE },
    { ID_SHOW_PREV,     ID_SHOW, 
	"Previous Display File", "p", main_display, 0, FALSE },
    { ID_SHOW_NEXT,     ID_SHOW, 
    	"Next Display File", "n", main_display, 0, FALSE },
    { ID_SHOW_REMOVE,	ID_SHOW, 
	"Remove Display File", "rd", main_display, 0, FALSE },
    { ID_CALC, 	  	ID_CALC, "(C)alc", "C", 0, 0, TRUE },
    { ID_CALC_DIST,   	ID_CALC, 
	"Distance between 2 planets", "d", main_calc, 0, FALSE },
    { ID_CALC_TEST_SHIP,ID_CALC, "Test Ship", "ct", main_calc, 0, FALSE },
    { ID_CALC_DRIVE,   	ID_CALC, "Drive Stat", "cD", main_calc, 0, FALSE },
    { ID_CALC_SHIELD,   ID_CALC, "Shield Stat", "cs", main_calc, 0, FALSE },
    { ID_CALC_KILL,     ID_CALC, "Kill Percentage", "ck", main_calc, 0, FALSE },
    { ID_CALC_PRODUCE,  ID_CALC, 
	"Production", "cp", main_calc_produce, 0, FALSE },
    { ID_OPTIONS,  	ID_OPTIONS, "(O)ptions", "O", 0, 0, TRUE },
    { ID_OPTIONS_CUST, 	ID_OPTIONS, 
	"Customize", "oc", main_cust, 0, FALSE },
    { ID_OPTIONS_MAP,  	ID_OPTIONS, "Map Labels", "om", main_cust, 0, FALSE },
    { ID_OPTIONS_COL,	ID_OPTIONS, 
	"Toggle 1/2/0 Columns", "ot", main_cust, 0, FALSE },
    { ID_OPTIONS_TECH,	ID_OPTIONS, 
    	"Toggle Group Stat/Tech", "og", main_cust, 0, FALSE },
    { ID_BATTLE,	ID_BATTLE, "(B)attle", "B", 0, 0, TRUE },
    { ID_BATTLE_USE, 	ID_BATTLE, 
	"Use in simulation", "bu", main_battle, 0, FALSE },
    { ID_BATTLE_SHOW, 	ID_BATTLE, 
    	"Display data", "bd", main_battle, 0, FALSE },
    { ID_BATTLE_RUN_ONE,ID_BATTLE, "Run", "bb", main_battle, 0, FALSE },
    { ID_BATTLE_CLEAR, 	ID_BATTLE, "Clear Data", "bc", main_battle, 0, FALSE },
    { ID_BATTLE_SAVE, 	ID_BATTLE, "Save Data", "bs", main_battle, 0, FALSE },
    { ID_BATTLE_READ, 	ID_BATTLE, 
	"Read in Data", "br", main_battle, 0, FALSE },
    { ID_HELP, 	  	ID_HELP, "Help(?)", "?", 0, 0, TRUE },
    { ID_HELP_KEYS, 	ID_HELP, "Keys", "ek", main_help, 0, FALSE },
    { ID_HELP_GENERAL, 	ID_HELP, "General", "eg", main_help, 0, FALSE },
    { ID_HELP_CMDS, 	ID_HELP, "Commands", "ec", main_help, 0, FALSE },
    { ID_HELP_SETUP, 	ID_HELP, "Setup", "es", main_help, 0, FALSE },
    { ID_HELP_BUGS, 	ID_HELP, "Bugs", "eb", main_help, 0, FALSE },
    { ID_HELP_CODE, 	ID_HELP, "Code", "ed", main_help, 0, FALSE },
    { 0, 0, 0, 0, 0, 0, 0 }
};

extern MapData g_map;
extern int g_cur_display;
extern int g_print_group_tech;
extern int g_insert_mode;
extern float g_galaxy_version;
extern char g_msg[MAX_MSG];
extern int g_num_col;
extern int old_num_col;
extern int g_starting_cols;
extern char g_your_race[MAX_RNAME];

/**
 ** Functions:
 **/

int main (int ac, char **av)
{
    g_map.max_width = 100;
    g_map.option = OPT_PLANET_ID;
    g_cur_display = DISPLAY_FILE;
    g_starting_cols = ALL_FILE;
    g_insert_mode = REPLACE;
    g_print_group_tech = FALSE;
    strcpy(g_your_race, "Unset");

    check_version();
    clear_tmps();

    ScreenInit();
    MsgInit();

    signal(SIGINT, trapfunc);

    init_planet_show();
    init_planets();
    init_groups();
    init_ships();

    init_battle();
    
    /* Check if we should read from standard input for
     *  an incoming turn report.  Then quit.
     */
    check_args(ac, av);

    ScreenSetDefaultCallback(main_callback, 1);
    ScreenSetRedrawCallback(main_redraw, 1);

    ScreenMenuInit(g_itemlist);

    FileResetFilename(HelpFile(HELP_USAGE), 0, 0, 0);

    sprintf(g_msg, "Goggle Version %s.%s, Galaxy Version %3.1f",
	    MAJOR_REVISION, MINOR_REVISION, g_galaxy_version);
    message_print(g_msg);
   
    /* We have to do this here in case the starting cols is NO_FILE,
       since FileResetFilename is used to switch between all file and no
       file when one, say, does sg on no_file display */
    g_num_col = g_starting_cols;

    ScreenResize();

    ScreenMainLoop();

#ifdef DEBUG
    bugFinish();
#endif

    return 0;
}



static int main_update_screen (void)
{
    switch (g_cur_display)
    {
	case DISPLAY_MAP :
	    PlanetShow();
	    break;
	case DISPLAY_FILE :
	    FileDisplay();
	    break;
    }
    return DONE;
}

static int main_callback (int i_id, int i_chr, int i_arg)
{
    int ret = OKAY;

    if (i_arg == 1) /* default */
    {
	switch ((char) i_chr)
	{
	    case REDRAW :
		Redisplay();
		break;
	    case JUMP_LEFT :
		ScreenJumpWorld(0, -1);
		break;
	    case JUMP_DOWN :
		ScreenJumpWorld(1, 0);
		break;
	    case JUMP_UP :
		ScreenJumpWorld(-1, 0);
		break;
	    case JUMP_RIGHT :
		ScreenJumpWorld(0, 1);
		break;
	    case JUMP_CENTER :
		ScreenCursorJump(0, 0);
		break;
	    case SCROLL_LEFT :
		ScreenCursorJump(0, -1);
		break;
	    case SCROLL_DOWN :
		ScreenCursorJump(1, 0);
		break;
	    case SCROLL_UP :
		ScreenCursorJump(-1, 0);
		break;
	    case SCROLL_RIGHT :
		ScreenCursorJump(0, 1);
		break;
	    case MOVE_LEFT :
		ScreenCursorMove(0, -1);
		break;
	    case MOVE_DOWN :
		ScreenCursorMove(1, 0);
		break;
	    case MOVE_UP :
		ScreenCursorMove(-1, 0);
		break;
	    case MOVE_RIGHT :
		ScreenCursorMove(0, 1);
		break;
	    case TOGGLE_SCREENS :
		old_num_col = g_num_col;
		if (g_cur_display == DISPLAY_FILE)
		{
		    g_cur_display = DISPLAY_MAP;
		    if( g_num_col == ALL_FILE )
		    {
			g_num_col = NO_FILE;
			ScreenResize();
		    }
		}
		else
		{
		    g_cur_display = DISPLAY_FILE;
		    if( g_num_col == NO_FILE )
		    {
			g_num_col = ALL_FILE;
			ScreenResize();
		    }
		}

		ScreenPlaceCursor();
		break;
	    case '1' : case '2' : case '3' : case '4' :
	    case '5' : case '6' : case '7' : case '8' :
	    case '9' : case '0' :
		       FileToFile(i_chr - '0');
		       break;
	    default :
		       ret = NOT_OKAY;
		       break;
	}
    }
    else
    {
	switch (i_id)
	{
	    case ID_VIEW_MARK :
		PlanetPlaceMark();
		break;
	    default :
		ret = NOT_OKAY;
		break;
	}
    }
    return(ret);
}

static void Redisplay (void)
{
    if( MAPWIN )
    {
	werase(MAPWIN);
	wrefresh(MAPWIN);
    }
    if( FILEWIN )
    {
	werase(FILEWIN);
	wrefresh(FILEWIN);
    }
    ScreenRedraw();
}

static int main_open (int i_id, int i_chr, int i_arg)
{
    PromptObj prompt[2];
    char filename[256];
    /*  char filename2[256]; ??? not used */

    filename[0] = '\0';

    prompt[0].type = GM_STRING;
    prompt[0].prompt = "File To Open : ";
    prompt[0].u.sval = filename;

    if (ScreenGetMany(prompt, 1, 0) != DONE)
	return ABORT;

    FileResetFilename(filename, 0, 0, 0);

    return DONE;
}

static int main_read (int i_id, int i_chr, int i_arg)
{
    PromptObj prompt[2];
    char filename[256];
    int do_save;
    int err;

    filename[0] = '\0';

    prompt[0].type = GM_STRING;
    prompt[0].prompt = "Turn Filename : ";
    prompt[0].u.sval = filename;

    prompt[1].type = GM_LOGICAL;
    prompt[1].prompt = "Save After Read : ";
    prompt[1].u.lval = TRUE;

    if (ScreenGetMany(prompt, 2, 0) != DONE)
	return ABORT;

    do_save = prompt[1].u.lval;

    err = read_data_file(filename, DATA_ALL);

    main_update_screen();

    if (do_save)
	if (err == OKAY)
	{
	    save_planets();
	    save_groups();
	    save_ships();
	}
    if (err == OKAY)
	message_print("Finished reading in turn.");

    PlanetRedisplay();

    return DONE;
}

static int main_save_data (int i_id, int i_chr, int i_arg)
{
    save_planets();
    save_groups();
    save_ships();
    message_print("Finished saving data.");
    return DONE;
}

static int main_save_map (int i_id, int i_chr, int i_arg)
{
    PromptObj prompt[1];
    char filename[512];

    prompt[0].type = GM_STRING;
    prompt[0].prompt = "Filename : ";
    prompt[0].u.sval = filename;

    if (ScreenGetMany(prompt, 1, 0) != DONE)
	return ABORT;

    if (PlanetMapDump(filename) == DONE)
    {
	sprintf(g_msg, "Finished saving map to file '%s'.", filename);
	message_print(g_msg);
    }
    return DONE;
}

static int main_view (int i_id, int i_chr, int i_arg)
{
    switch (i_id)
    {
	case ID_VIEW_ZOOM :
	    PlanetDoZoom();
	    break;
	case ID_VIEW_UNZOOM :
	    PlanetUnZoom();
	    break;
	case ID_VIEW_RECENTER :
	    PlanetRecenterAtCursor();
	    break;
	case ID_VIEW_REPOS :
	    PlanetReposition();
	    break;
	case ID_VIEW_AUTOPOS :
	    PlanetAutoPosition(0, 0);
	    break;
	default :
	    return NOT_OKAY;
    }

    return DONE;
}

static int main_calc (int i_id, int i_chr, int i_arg)
{
    switch (i_id)
    {
	case ID_CALC_DIST :
	    CalcPlanetDist();
	    break;
	case ID_CALC_DRIVE :
	    CalcDriveStat();
	    break;
	case ID_CALC_SHIELD :
	    CalcShieldStat();
	    break;
	case ID_CALC_KILL :
	    CalcKillPercentage();
	    break;
	case ID_CALC_TEST_SHIP :
	    CalcTestShip();
	    break;
    }

    return DONE;
}

static int main_calc_produce (int i_id, int i_chr, int i_arg)
{
    char *list[7];
    char *prompt;
    int which;

    list[0] = "Num Ships";
    list[1] = "Capital";
    list[2] = "Materials";
    list[3] = "Drive Tech";
    list[4] = "Weapon Tech";
    list[5] = "Shield Tech";
    list[6] = "Cargo Tech";
    prompt = "Select Production Type To Estimate";

    which = ScreenGetOneOfMany(list, 7, prompt);

    switch (which)
    {
	case 0 :
	    CalcNumShips();
	    break;
	case 1 :
	    CalcEstimate(PR_CAP);
	    break;
	case 2 :
	    CalcEstimate(PR_MAT);
	    break;
	case 3 :
	    CalcEstimate(PR_DRIVE);
	    break;
	case 4 :
	    CalcEstimate(PR_WEAPONS);
	    break;
	case 5 :
	    CalcEstimate(PR_SHIELDS);
	    break;
	case 6 :
	    CalcEstimate(PR_CARGO);
	    break;
    }
    return DONE;
}

static int main_display (int i_id, int i_chr, int i_arg)
{
    switch (i_id)
    {
	case ID_SHOW_GROUPS :
	case ID_SHOW_ALL_GROUPS :
	case ID_SHOW_PLANETS :
	case ID_SHOW_ROUTES :
	case ID_SHOW_SHIPS :
	case ID_SHOW_DIST_ALL :
	    DisplayInfo(i_id);
	    break;
	case ID_SHOW_MSG :
	    FileResetFilename(MSG_FILE, "Messages", 0, 0);
	    break;
	case ID_SHOW_NEXT :
	    FileNextFile(1);
	    break;
	case ID_SHOW_PREV :
	    FileNextFile(-1);
	    break;
	case ID_SHOW_REMOVE :
	    FileDelete();
	    break;
    }
    return DONE;
}

static int main_cust (int i_id, int i_chr, int i_arg)
{
    switch (i_id)
    {
	case ID_OPTIONS_CUST :
	    Customize();
	    break;
	case ID_OPTIONS_MAP :
	    CustMapLabels();
	    break;
	case ID_OPTIONS_COL :
	    if (g_num_col == ALL_FILE)
		g_num_col = NO_FILE;
	    else
		g_num_col--;
	    ScreenResize();
	    break;
	case ID_OPTIONS_TECH :
	    g_print_group_tech = !g_print_group_tech;
	    if (g_print_group_tech)
		message_print("Will display tech levels on groups lines instead of stats.");
	    else
		message_print("Will display stats on groups lines instead of techs.");
	    break;
    }
    return DONE;
}

static int main_help (int i_id, int i_chr, int i_arg)
{
    switch (i_id)
    {
	case ID_HELP_GENERAL   :
	    FileResetFilename(HelpFile(HELP_USAGE), 0, 1, " GENERAL ");
	    break;
	case ID_HELP_CMDS      :
	    FileResetFilename(HelpFile(HELP_USAGE), 0, 1, " COMMANDS ");
	    break;
	case ID_HELP_SETUP     :
	    FileResetFilename(HelpFile(HELP_SETUP), 0, 1, 0);
	    break;
	case ID_HELP_BUGS      :
	    FileResetFilename(HelpFile(HELP_BUGS), 0, 1, 0);
	    break;
	case ID_HELP_KEYS	     :
	    FileResetFilename(HelpFile(HELP_USAGE), 0, 1, " KEYS ");
	    break;
	case ID_HELP_CODE      :
	    FileResetFilename(HelpFile(HELP_CODE), 0, 1, 0);
	    break;
    }
    return DONE;
}

static int main_battle (int i_id, int i_chr, int i_arg)
{
    switch (i_id)
    {
	case ID_BATTLE_USE :
	    BattleUse();
	    break;
	case ID_BATTLE_SHOW :
	    DisplayInfo(i_id);
	    break;
	case ID_BATTLE_RUN_ONE :
	case ID_BATTLE_RUN_MULTI :
	    if (BattleRun() == DONE)
		DisplayInfo(ID_BATTLE_SHOW);
	    break;
	case ID_BATTLE_CLEAR :
	    init_battle();
	    message_print("Battle data cleared");
	    break;
	case ID_BATTLE_SAVE :
	    BattleSaveData();
	    break;
	case ID_BATTLE_READ :
	    BattleReadData();
	    break;
	default :
	    return NOT_OKAY;
    }
    return DONE;
}


static int main_quit (int i_id, int i_chr, int i_arg)
{
    clear_tmps();
    save_zooms();
    ScreenTerm();
    exit(0);
}



static int main_redraw (int arg)
{
    arg++;	/*Not actually used, but this stops warnings */
    return DONE;
}

static void trapfunc ( int signal )
{
    ScreenTerm();
    printf("Quit\n");
    exit(1);
}

/*
 *  Display some planet information in the other
 *  data window.  i_what may be
 *	SHOW_GROUPS
 */
static int DisplayInfo (int i_what)
{
    FILE *fp;
    char *title;
    char filename[256];
    int planet;
    int err = OKAY;
    int file_id;
    int race;

    title = 0;
    file_id = 0;

    getTmpFilename(filename);

    if ((fp = fopen(filename, "w")) == NULL)
    {
	errnoMsg(g_msg, filename);
	message_print(g_msg);
	return FILEERR;
    }
    switch (i_what)
    {
	case ID_SHOW_GROUPS :
	    if ((planet = PlanetAtCursor(QUERY)) < 0)
	    {
		fclose(fp);
		return NOT_OKAY;
	    }
	    write_single_planet_with_title(fp, planet);
	    print_pl_routes(fp, planet);
	    write_pl_groups(fp, planet);
	    title = "Planet Info";
	    file_id = planet;
	    break;
	case ID_SHOW_ALL_GROUPS :
	    if ((race = get_ship_race()) < 0)
		err = NOT_OKAY;
	    else if ((file_id = print_sorted_groups(fp, race)) == -1)
		err = NOT_OKAY;
	    title = "Groups Info";
	    break;
	case ID_SHOW_PLANETS :
	    err = print_some_planets(fp);
	    title = "Planets Info";
	    break;
	case ID_SHOW_ROUTES :
	    err = print_routes(fp);
	    title = "Route Info";
	    break;
	case ID_SHOW_SHIPS :
	    if ((file_id = print_ships(fp)) == -1)
		err = NOT_OKAY;
	    title = "Ship Info";
	    break;
	case ID_SHOW_DIST_ALL :
	    if ((file_id = PlanetDisplayDistAll(fp)) == -1)
		err = NOT_OKAY;
	    title = "Distance from Planet Info";
	    break;
	case ID_BATTLE_SHOW :
	    if ((file_id = BattleDisplayResults(fp)) == -1)
		err = NOT_OKAY;
	    title = "Simulation Battle Results";
	    break;
    }
    fclose(fp);

    if (err != OKAY)
	return NOT_OKAY;

    FileResetFilename(filename, title, file_id, 0);

    return DONE;
}


static int Customize (void)
{
    PromptObj prompt[3];

    prompt[0].type = GM_INTEGER;
    prompt[0].prompt = "Galaxy Width : ";
    prompt[0].u.ival = g_map.max_width;

    prompt[1].type = GM_FLOAT;
    prompt[1].prompt = "Galaxy Version : ";
    prompt[1].u.fval = g_galaxy_version;
    
    prompt[2].type = GM_INTEGER;
    prompt[2].prompt = "Map Window Colums (0/1/2) : ";
    prompt[2].u.ival = g_num_col;

    if (ScreenGetMany(prompt, 3, 0) != DONE)
	return ABORT;

    g_map.max_width = prompt[0].u.ival;
    g_galaxy_version = prompt[1].u.fval;
    g_starting_cols = prompt[2].u.ival;

    write_version();
    init_planet_show();
    PlanetRedisplay();

    return DONE;
}

static int CustMapLabels (void)
{
    char *list[4];
    char *prompt;
    int which;

    list[0] = "Planet Name";
    list[1] = "Planet Size";
    list[2] = "Planet Resource";
    list[3] = "Distance From Map Center";
    prompt = "Select Planet Map Label";

    which = ScreenGetOneOfMany(list, 4, prompt);

    switch (which)
    {
	case 0 :
	    g_map.option = OPT_PLANET_ID;
	    break;
	case 1 :
	    g_map.option = OPT_PLANET_SIZE;
	    break;
	case 2 :
	    g_map.option = OPT_PLANET_RES;
	    break;
	case 3 :
	    g_map.option = OPT_PLANET_DIST;
	    break;
    }
    PlanetRedisplay();

    return DONE;
}

static char *HelpFile (char *cp_file)
{
    static char path[256];

    expandHome(HELP_PATH, path);
    strcat(path, "/");
    strcat(path, cp_file);
    return path;
}

static int check_args (int ac, char **av)
{
    int i, err;

    if (ac > 1)
	for (i=1;i<ac;i++)
	{
	    if (av[i][0]!='-')
	    {
		usage();
		ScreenTerm();
		exit(1);
	    }

	    switch (av[i][1])
	    {
		case '\0':
		    save_tmp_file(TMP_FILE);

		    err = read_data_file(TMP_FILE, DATA_ALL);

		    if (err == OKAY)
		    {
			save_planets();
			save_groups();
			save_ships();
			unlink(TMP_FILE);
		    }
		    ScreenTerm();
		    exit(1);

		default:
		    usage();
		    ScreenTerm();
		    exit(1);
	    }
	}
    return DONE;
}

static int save_tmp_file (char *file)
{
    FILE *fp;
    int c;

    if ((fp=fopen(file,"w"))==NULL)
    {
	perror(file);
	ScreenTerm();
	exit(1);
    }
    while ((c=getchar())!=EOF)
	fputc(c,fp);

    fclose(fp);

    return OKAY;
}

static int usage (void)
{
    fprintf(stderr,"Usage : goggle [-]\n");
    fprintf(stderr,"  - standard input treated as input turn report.\n");
    return DONE;
}

void abort_program (char *cp_msg)
{
    ScreenTerm();
    puts(cp_msg);
    exit(1);
}

void message_print (char *cp_msg)
{
    MsgDisplay(cp_msg);
}
