int line_type (char *line);
void break_up (char *cp_line);
char *element_str (char *cp_line, int top);
int element_int (char *cp_line, int e);
float element_float (char *cp_line, int e);
char *extract_race_name (char *s);

#define OWN_ORDERS     "> " /* Anders - identify own orders lines in lexer */
