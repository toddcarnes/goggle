int init_groups (void);
void clear_groups (int i_race);
int save_groups (void);
int write_all_races_groups (FILE *fp);
void write_pl_groups (FILE *fp, int i_pl);
int set_group_data(int i_race, int i_groupid, int i_numships,
		   char *cp_shipname, char *cp_planetname,
		   float f_dist,
		   float f_drive, float f_weapon, float f_shield, float f_cargo,
		   char *cp_what, float f_quantity);
int new_alien_group_data (int i_race, int i_groupid, int i_numships,
			  char *cp_shipname, char *cp_planetname,
			  float f_dist, float f_drive, float f_weapon,
			  float f_shield, float f_cargo,
			  char *cp_what, float f_quantity);
int group_new_race (char *cp_race);
int set_your_race_name (char *cp);
int get_race_id_no_err (char *cp);
int find_race_id (char *cp);
char *get_race_name (int race);
int get_race_techs (int race, int i_tech,
		    float *fp_drive, float *fp_weapon, float *fp_shield, float *fp_cargo);
int set_race_techs (int race, int i_tech,
		    float f_drive, float f_weapon, float f_shield, float f_cargo);
int get_num_races (void);
int new_tech_level (int race,
		    float drive, float weapon, float shield, float cargo);
int group_check_destroyed (int i_raceid, char *cp_shipname, char *cp_planetname);
void sort_groups (void);
int print_sorted_groups (FILE *fp, int i_race);
int place_groups_at_planet_in_sim (int i_race, int i_pl);
int place_group_in_sim (int i_index);
