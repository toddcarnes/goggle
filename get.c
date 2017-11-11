/*
 * $Id: get.c,v 1.3 1997/12/11 00:43:44 rlpowell Exp $
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "util.h"
#include "get.h"

static int expandNewLine (char *cp_str);

/*
 * Globals & Defines
 */
#define MAX 3
#define MAX_NAME 512
/*
 * Functions
 * < list of functions avaialable >
 */


/*
 *   Get a string from the console and return it.  Uses three
 *   global strings and cycles through those.
 *
 *   The tilde character is translated to a return character.
 *
 *   The prompt will be appended with ": ".
 *   use test_setPromptEnd() to override.
 */
char *askstr (char *cp_prompt)
{
    static char str[MAX][256];
    static int cur = 0;
    /*  int num; ??? not used */

    if (++cur >= MAX)
	cur = 0;

    printf("%s:", cp_prompt);
    gets(str[cur]);
    expandNewLine(str[cur]);
    return str[cur];
}


/*
 *   Get a float from the console.
 *
 *   The prompt will be appended with ": ".
 *   use test_setPromptEnd() to override.
 *
 static double getf (char *cp_prompt)
 {
 char str[100];

 while (1)
 {
 printf("%s:", cp_prompt);
 gets(str);

 if (strIsDouble(str))
 return atof(str);
 else
 {
 printf("Invalid: '%s'\n", str);
 printf("Please enter in a valid float\n");
 }
 }
 }
 *
 *  Not currently used function.
 *
 */


/*
 *   Get a long from the console.
 *
 *   The prompt will be appended with ": ".
 *   use test_setPromptEnd() to override.
 *
 static long getl (char *cp_prompt)
 {
 char str[100];

 while (1)
 {
 printf("%s:", cp_prompt);
 gets(str);

 if (strIsInt(str))
 return atol(str);
 else
 {
 printf("Invalid: '%s'\n", str);
 printf("Please enter in a valid long\n");
 }
 }
 }
 *
 * Not currently used function
 */


/*
 *   Get a decimal integer from the console.
 *
 *   The prompt will be appended with ": ".
 *   use test_setPromptEnd() to override.
 */
int getd (char *cp_prompt)
{
    char str[100];

    while (1)
    {
	printf("%s:", cp_prompt);
	gets(str);

	if (strIsInt(str))
	    return atoi(str);
	else
	{
	    printf("Invalid: '%s'\n", str);
	    printf("Please enter in a valid integer\n");
	}
    }
}

/*
 *   Prompt the user for an integer within the
 *   specified range.  An error is printed an
 *   the user is re-prompted, if he/she enters
 *   a value outside of the range.
 *
 static int getrange (char *cp_prompt, int i_low, int i_high)
 {
 char prompt[100];
 int val;

 sprintf(prompt, "%s [%d..%d]", cp_prompt, i_low, i_high);

 while (1)
 {
 val = getd(prompt);

 if ((val < i_low) || (val > i_high))
 printf("Enter an integer between %d and %d.\n", i_low, i_high);
 else
 return val;
 }
 }
 *
 * Not currently used function
 */


/*
 *   Prompt the user with the passed prompt.
 *   No additional characters are displayed as
 *   part of the prompt, the passed prompt is
 *   displayed only.
 *
 static int getyes (char *cp_prompt)
 {
 char line[MAX_NAME];
 int c;

 while (1)
 {
 printf(cp_prompt);
 gets(line);

 if ((line[0] == 'n') || (line[0] == 'N') || (line[0] == '\0'))
 return 0;

 for (c = 0; (line[c] == 'y') || (line[c] == 'Y'); c++)
 ;

 if (c > 0)
 return c;

 printf("Enter 'y' for yes, 'n' for no.\n");
 }
 }
 *
 * Not currently used function
 */



/*
 *   Expand any '~' found in the string into
 *   newlines.
 */
static int expandNewLine (char *cp_str)
{
    char *cp;

    for (cp = cp_str; *cp != '\0'; cp++)
	if (*cp == '~')
	    *cp = '\n';

    return 0;
}
