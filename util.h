char *zcalloc (int num, int size, char *file, int line);
char *zmalloc (int size, char *file, int line);
char *zrealloc (char *ptr, int size, char *file, int line);
int zrealloc_cpy (char **cpp_to, char *cp_from);
int zalloc_cpy (char **cpp_to, char *cp_from);
int strIsInt (char *cp_str);
int strIsDouble (char *cp_str);
int strLocate (char *cp_target, char *cp_match);
int strStripSpaces (char *cp_text);
int strStripZeros (char *cp_line);
int strRmReturn (char *cp_line);
void strRmTab (char *cp_line);
int strExpandTab (char *cp_line);
int strRmWord (char *cp_line, char *cp_word);
int strLongestLen (char **cpp_lines, int i_num);
char *getTmpFilename (char *filename);
int clear_tmps (void);
int expandHome (char *cp_in, char *cp_out);

#ifdef DEBUG
void bugFinish (void);
#endif
