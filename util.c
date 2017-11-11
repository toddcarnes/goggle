/*
 * File: util.c
 * Author: Douglas Selph
 * Maintained by: Robin Powell
 * $Id: util.c,v 1.4 1998/06/19 20:58:00 rlpowell Exp $
 */

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>
#include <string.h>
#include "common.h"

#include "errno.h"
#include "main.h"
#include "util.h"

/**
 **  Local Structures and Defines
 **/
extern char g_msg[MAX_MSG];

static void mem_error (char *file, int line);
static int str_IsDouble (char *cp_str);

/**
 ** Functions:
 **/

char *zcalloc (int num, int size, char *file, int line)
{
    char *p;

    if ((p = calloc(num, size)) == NULL)
	mem_error(file, line);
    return p;
}

char *zmalloc (int size, char *file, int line)
{
    char *p;

    if ((p = malloc(size)) == NULL)
	mem_error(file, line);

    return p;
}

char *zrealloc (char *ptr, int size, char *file, int line)
{
    char *p;

    if ((p = realloc(ptr,size)) == NULL)
	mem_error(file, line);

    return p;
}

static void mem_error (char *file, int line)
{
    fprintf(stderr, "Out of memory at %s, %d\n", file, line);
    exit(1);
}

int zrealloc_cpy (char **cpp_to, char *cp_from)
{
    int from_len;

    /*
     *  In this case free any memory used by cpp_to and
     *  set to NULL.
     */
    if (cp_from == NULL)
    {
	if (*cpp_to != NULL)
	{
	    free(*cpp_to);
	    *cpp_to = NULL;
	}
	return OKAY;
    }
    /*
     *  Calculate length of string.
     */
    from_len = strlen(cp_from);
    /*
     *  Is the target string going to be too small
     *  for our source string?
     */
    if (*cpp_to != NULL)
	if (from_len > strlen(*cpp_to))
	{
	    free(*cpp_to);
	    *cpp_to = NULL;
	}
    /*
     *  Allocate target string.
     */
    if (*cpp_to == NULL)
	*cpp_to = (char *) CALLOC(1, from_len + 1);
    strcpy(*cpp_to, cp_from);

    return OKAY; /* okay */
}

/*
 *   Copy cp_from to the what cpp_to points to.  Irregardless
 *   of what cpp_to points to, a newly alloced string will
 *   be alloced and cpp_to will be set to that string.
 *   If it is wished that what cpp_to points to should be
 *   freed() first, then use zrealloc_cpy().
 */
int zalloc_cpy (char **cpp_to, char *cp_from)
{
    *cpp_to = NULL;
    return zrealloc_cpy(cpp_to, cp_from);
}

/*
 *   Returns TRUE cp_str contains a valid integer.
 */
int strIsInt (char *cp_str)
{
    int i;

    if (cp_str == NULL)
	return FALSE;

    i = 0;

    /* skip over leading spaces */
    while (cp_str[i] == ' ')
	i++;

    if (cp_str[i] == '-')
	i++;

    if (cp_str[i] == '\0')
	return FALSE;

    for (; cp_str[i] != '\0'; i++)
	if (!isdigit(cp_str[i]))
	    return FALSE;

    return TRUE;
}

/*
 *   Returns TRUE cp_str contains a valid double.
 *   There may be spaces leading the double.
 */
int strIsDouble (char *cp_str)
{
    return str_IsDouble(cp_str);
}

/*
 *   Returns TRUE cp_str contains a valid double.
 *   There may be spaces leading the double.
 */
static int str_IsDouble (char *cp_str)
{
    char *copy;
    int found_dot, found_e, found_num;
    register int i;

    if (cp_str == NULL)
	return FALSE;

    zalloc_cpy(&copy, cp_str);
    strStripSpaces(copy);

    i = 0;
    /*
     *  May start number with optional minus sign.
     */
    if (copy[i] == '-')
	i++;
    /*
     *  If we already are at the end of the string
     *  then return FALSE.
     */
    if (copy[i] == '\0')
    {
	free(copy);
	return FALSE;
    }
    /*
     *  More initialization
     */
    found_dot = 0;
    found_e = 0;
    found_num = 0;

    for (; copy[i] != '\0'; i++)
	/*
	 *  Digits always allowable, must have at least one.
	 */
	if (isdigit(copy[i]))
	    found_num = 1;
	else
	    switch (copy[i])
	    {
		case 'e' :
		case 'E' :
		    /*
		     *  More than one 'e' not allowed
		     */
		    if (found_e)
		    {
			free(copy);
			return FALSE;
		    }
		    found_e = 1;
		    /*
		     *  Plus or minus legal following the E.
		     */
		    if ((copy[i+1] == '+') || (copy[i+1] == '-'))
			i++;
		    break;
		case '.' :
		    /*
		     *  More than one dot not allowed,
		     *  also dot not allowed following 'e'.
		     */
		    if ((found_e) || (found_dot))
		    {
			free(copy);
			return FALSE;
		    }

		    found_dot = 1;
		    break;
		    /*
		     *  Illegal character.
		     */
		default :
		    free(copy);
		    return FALSE;
	    }
    free(copy);
    return found_num;
}

/*
 *   The integer position is returned in 'cp_target' where there is a string
 *   that matches 'cp_match'.   0 refers to the first character in
 *   cp_target.
 *
 *   Returns NO_MATCH(-1) if no-match at all.
 */
int strLocate (char *cp_target, char *cp_match)
{
    int cur_target, cur_match, match_start;

    if (cp_match[0] == '\0')
	return 0; /* empty match string */

    cur_match = 0;
    match_start = NO_MATCH;

    for (cur_target = 0; cp_target[cur_target] != '\0'; cur_target++)
    {
	if (cp_target[cur_target] == cp_match[cur_match])
	{
	    if (match_start == NO_MATCH)
		match_start = cur_target; /* first character matched */

	    if (cp_match[++cur_match] == '\0')
		return match_start; /* completed match */
	}
	else
	{
	    cur_match = 0;
	    match_start = NO_MATCH;
	}
    }
    return NO_MATCH;
}


/*
 *   Strip preceeding and proceeding spaces in the string.
 *   0 if there is no non-space character in the passed string.
 */
int strStripSpaces (char *cp_text)
{
    int start, end;

    if (cp_text == NULL)
	return NOT_OKAY;

    for (start = 0; cp_text[start] != '\0'; start++)
	if (!isspace(cp_text[start]))
	    break;

    end = strlen(cp_text) - 1;

    for (; end >= 0; end--)
	if (!isspace(cp_text[end]))
	    break;

    if ((cp_text[start] == '\0') || (end < 0))
	cp_text[0] = '\0';
    else
    {
	char *tmp;

	tmp = (char *) MALLOC(end+3);
	cp_text[end+1] = '\0';
	strcpy(tmp, cp_text+start);
	strcpy(cp_text, tmp);
	free(tmp);
    }
    return cp_text[0] == '\0' ? NOT_OKAY : OKAY;
}

int strStripZeros (char *cp_line)
{
    char *pos, *first_zero, *cur, *to, *from;
    int num_zeros;
    /*  int has_non_zero; ??? not used */

    cur = cp_line;

    while (*cur && (pos = strchr(cur, '.')))
    {
	num_zeros = 0;

	pos++;

	while (isdigit(*pos))
	{
	    /*
	     * Check for zero.
	     */
	    if (*pos == '0')
	    {
		if (num_zeros == 0)
		    first_zero = pos;

		num_zeros++;
	    }
	    else
		num_zeros = 0;
	    pos++;
	}
	if (num_zeros > 0)
	{
	    /* copy the rest of the line removing the extra zeros */
	    to = first_zero;
	    from = pos;

	    while (*from)
		*to++ = *from++;

	    *to = *from;	/* copy end of line marker too */

	    pos = first_zero+1;
	}
	cur = pos;
    }
    /*
     *  Special : check for #.
     */
    cur = cp_line;

    while (*cur && (pos = strchr(cur, '.')))
    {
	pos++;

	if (!isdigit(*pos))
	{
	    to = pos-1;
	    from = pos;

	    while (*from)
		*to++ = *from++;

	    *to = *from;	/* copy end of line marker too */
	}
	cur = pos;
    }
    return DONE;
}

int strRmReturn (char *cp_line)
{
    char *s;

    /* find end of string */
    for (s = cp_line; *s != '\0'; s++)
	;

    s--;

    if (s >= cp_line)
	if (*s == '\n')
	    *s = '\0';

    return DONE;
}

void strRmTab (char *cp_line)
{
    char *s;

    for (s = cp_line; *s != '\0'; s++)
	if (isspace(*s))
	    *s = ' ';
}

int strExpandTab (char *cp_line)
{
    char *s, *s2;
    char buf[256];
    int c;

    s2 = buf;
    c = 1;
    for (s = cp_line; *s != '\0'; s++, c++)
	if (*s == '\t')
	{
	    for (;c % 8 > 0; c++)
		*s2++ = ' ';
	    c--;
	}
	else
	    *s2++ = *s;
    *s2++ = '\0';
    strcpy(cp_line, buf);
    return DONE;
}

int strRmWord (char *cp_line, char *cp_word)
{
    int p, i;

    if ((p=strLocate(cp_line, cp_word)) >= 0)
	for (i = p; i < p+strlen(cp_word);i++)
	    cp_line[i] = ' ';

    return OKAY;
}


/*
 * Purpose:
 *
 *  Find the longest string in a list of strings
 *  and return it's length.
 */
int strLongestLen (char **cpp_lines, int i_num)
{
    int len, max;
    int i;

    max = 0;
    for (i = 0; i < i_num; i++)
	if ((len = strlen(cpp_lines[i])) > max)
	    max = len;
    return max;
}

char *getTmpFilename (char *filename)
{
    static int which;

    sprintf(filename, "%s%c", TMP_FILE, (char) ('a' + which));

    if (++which >= MAX_TMPS)
	which = 0;

    return filename;
}

int clear_tmps (void)
{
    int i;
    char filename[256];

    for (i = 0; i < MAX_TMPS; i++)
    {
	getTmpFilename(filename);
	unlink(filename);
    }
    return DONE;
}

int expandHome (char *cp_in, char *cp_out)
{
    while (*cp_in == ' ')
	cp_in++;

    if (cp_in[0] == '~')
    {
	char *home = getenv("HOME");

	if (home != NULL)
	    sprintf(cp_out, "%s%s", home, cp_in+1);
	else
	    sprintf(cp_out, "%s", cp_in+1);
    }
    else
	strcpy(cp_out, cp_in);
    return OKAY;
}

#if 0
/*This is in here because it's never used but I wanted to keep it around
  just in case*/
static int filecopy(char *cp_src, char *cp_tgt)
{
    FILE *fp, *fp2;
    char line[256];

    if ((fp = fopen(cp_src, "r")) == NULL)
    {
	errnoMsg(g_msg, cp_src);
	message_print(g_msg);
	return FILEERR;
    }
    if ((fp2 = fopen(cp_tgt, "w")) == NULL)
    {
	errnoMsg(g_msg, cp_tgt);
	message_print(g_msg);
	return FILEERR;
    }
    while (fgets(line, sizeof(line), fp))
	fputs(line, fp2);

    fclose(fp2);
    fclose(fp);

    return DONE;
}
#endif

#ifdef DEBUG

static FILE *gfp;

static void bugPrint (char *msg)
{
    if (gfp == 0)
	if ((gfp = fopen("bug.out", "w")) == 0)
	{
	    perror("bug.out");
	    exit(1);
	}
    fprintf(gfp, "%s", msg);
}

static void bugFlush (void)
{
    if (gfp)
    {
	fclose(gfp);
	gfp = fopen("bug.out", "a");
    }
}

void bugFinish (void)
{
    if (gfp)
    {
	fclose(gfp);
	gfp = 0;
    }
}

#endif
