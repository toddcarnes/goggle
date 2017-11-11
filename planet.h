/*
 * File: planet.h
 * Author: Douglas Selph
 * Maintained by: Robin Powell
 * $Id: planet.h,v 1.2 1997/12/05 03:42:41 rlpowell Exp $
 *   Extracted from the Machine archive
 * Purpose:
 */

/* ??? this structure is only used in planetshow.c... */
struct planets {
  char *name;
  float x, y;
  float size, res;
  int pop;
  float industry;
  char *produce;
  float cap;
  float mat;
  float col;
  int who;
  int set;
  int floater;		/* not particular about where we are in the array */
};

extern struct planets *planet;

int init_planets (void);
int save_planets (void);
int print_some_planets (FILE *fp);
int write_single_planet_with_title (FILE *fp, int i);
int legal_planet (int id);
int translate_planet (char *cp_name);
int decode_planet_name (char *cp_name);
char *get_planet_name (int id);
char *get_produce_name (int id);
int set_planet_data (char *cp_name, float x, float y, float size, int pop,
		     float industry, float res, char *cp_produce,
		     float cap, float mat, float col, int who);
double planet_dist (int pl_1, int pl_2);
double planet_xy_dist (float x, float y, int pl);
float get_planet_x (int id);
float get_planet_y (int id);
int get_who (int id);
float get_planet_res (int id);
float get_planet_size (int id);
float compute_num_ships (int id, float shipmass);
float compute_estimate (int id, int i_what);
