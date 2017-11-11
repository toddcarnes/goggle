int read_battle_data (char *cp_filename);
void init_battle (void);
int battle_new_race (char *cp_racename);
int battle_new_group (int i_raceid, int i_numships, char *cp_shiptype,
		      float f_drive, float f_weapon, float f_shield, float f_cargo,
		      char *cp_cargotype, float f_quantity);
void battle_init_groups (void);
int write_battle_data (char *cp_filename);
int write_battle_data_raw (FILE *filefp, int i_mod_okay);
int do_battle (char *cp_filename);
