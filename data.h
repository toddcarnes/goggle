/*
 * File: data.h
 * Author: Douglas Selph
 * Maintained by: Robin Powell
 * $Id: data.h,v 1.3 1998/06/19 20:58:00 rlpowell Exp $
 *    Extracted from the Machine archive
 * Purpose:
 */
#ifndef DATA_H
#define DATA_H

#define TEST_SLOT               0
#define TEST_RACE               0

#define MAX_CARGO              -1.0

/* #define ERROR_NAME "??" ??? not used */
/* #define UNKNOWN_NAME "?" ??? not used */
#define MAX_RNAME              80

#define RACE_NEW               -3
#define RACE_ERR               -2
#define RACE_ALL               -1

#define DATA_ALL                0
#define DATA_GROUPS             1
#define DATA_PLANETS            2
#define DATA_SHIPS              3

#define CURRENT_TECH             0
#define OVERRIDE_TECH            1

/* #define CAPITAL_NAME            "Capital" ??? not used */
/* #define RAW_MAT_NAME            "Raw Materials" ??? not used */
/* #define RESEARCH_NAME           "Research" ??? not used */
/* #define SHIPS_NAME              "Ships" ??? not used */
/* #define TECH_LEVEL              "Tech Level" ??? not used */

#define YOUR_PLANETS            "Your Planets"
#define PLANETS                 "Planets"
#define ALIEN_PLANETS           "Unidentified Planets"
#define NEUTRAL_PLANETS         "Uninhabited Planets"
#define SHIP_TYPES              "Ship Types"
#define GROUPS                  "Groups"
#define YOUR                    "Your"
#define ABBR_GROUPS             "List Of"
#define REPORT_TITLE            "Report for"
#define YOUR_ROUTES             "Your Routes"
#define BATTLE_AT               "Battle at"
#define SAMPLE_BATTLE           "Sample Battle"
#define AT_WAR                  "At War"
#define DESTROYED               "Destroyed"
#define STATUS_OF               "Status of Players"
#define FLEET                   "Fleet"
#define RESULTS                 "Results"
#define GLOBAL_MESSAGES		"GLOBAL Messages"
#define PERSONAL_MESSAGES	"PERSONAL Messages"
#define YOUR_OPTIONS		"Your Options"


/* #define CAPITAL                  1 ??? not used */
/* #define RAW_MAT                  2 ??? not used */
/* #define RESEARCH                 3 ??? not used */
/* #define SHIPS                    4 ??? not used */

#define PR_CAP                   0
#define PR_MAT                   1
#define PR_DRIVE                 2
#define PR_WEAPONS               3
#define PR_SHIELDS               4
#define PR_CARGO                 5

/* #define WHO_NOID                 0 ??? not used */
#define WHO_ME                   1
#define WHO_ALIEN                2
#define WHO_NOONE                3
#define WHO_FRIEND               4
#define WHO_NOONE_SEEN           5

#define TYPE_INVALID             0
#define TYPE_UNIDENTIFIED        1
#define TYPE_UNINHABITED         2
#define TYPE_SHIPS               3
#define TYPE_YOUR_PLANETS        4
#define TYPE_YOUR_GROUPS         5
#define TYPE_YOUR_ABBR_GROUPS    6
#define TYPE_ALIEN_GROUPS        7
#define TYPE_ALIEN_ABBR_GROUPS   8
#define TYPE_PLANETS             9
#define TYPE_ROUTES             10
#define TYPE_NEW_RACE           11
/* #define TYPE_RACE_LINE          12 ??? not used */
#define TYPE_BATTLE_AT          13
#define TYPE_DESTROYED          14
#define TYPE_STATUS_OF          15
#define TYPE_SAMPLE_BATTLE      16
#define TYPE_AT_WAR             17
#define TYPE_RESULTS            18
#define TYPE_GMESSAGES		19	/* Global Messages */
#define TYPE_PMESSAGES		20	/* Personal Messages */

#define OPT_PLANET_ID            0
#define OPT_PLANET_SIZE          1
#define OPT_PLANET_RES           2
#define OPT_PLANET_DIST          3

#define WITH_TITLE               1
/* #define NO_TITLE                 0 ??? not used */

#define BATTLE_DIST             -1

/* char *get_planet_name(int id);           planet.c */
/* char *get_produce_name(int id);          planet.c */
/* char *get_who_name();  ??? neither used nor defined */

/* float calc_num_ships();  ??? neither used nor defined */
/* char *get_ship_title();  ??? neither used nor defined */
/* char *getGroupSaveFile();  ??? neither used nor defined */

#endif
