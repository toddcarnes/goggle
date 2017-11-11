/*
 * File: field.c
 * Author: Douglas Selph
 * Maintained by: Robin Powell
 * $Id: field.c,v 1.3 1997/12/11 00:43:44 rlpowell Exp $
 */
#include <stdio.h>
#include <ctype.h>
#include "common.h"

#include "util.h"
#include "field.h"

/**
 **  Local Structures and Defines
 **/
extern char g_msg[MAX_MSG];

#define MAX_FIELDS 14

static struct global_field
{
  struct fields
  {
    int maxlen;
  } field[MAX_FIELDS];
} gField;

static char g_field_line[256];

static char *field_Justify (int i_field, char *cp_str, int i_justify);
static void fieldMeasureStr (int i_field, char *cp_str);
static char *fieldGetStr (int i_field, char *cp_str, int i_justify);

/**
 ** Functions:
 **/

void fieldReset (void)
{
  int i;

  for (i = 0; i < MAX_FIELDS; i++)
    gField.field[i].maxlen = 0;
}

void fieldPrintInt (FILE *fp, int i_field, int i_val, char *cp_post, int i_justify)
{
  char line[128];

  sprintf(line, "%d", i_val);
  fieldPrintStr(fp, i_field, line, cp_post, i_justify);
}

void fieldPrintFloat (FILE *fp, int i_field, float f_val, char *cp_post, int i_justify)
{
  char line[128];

  sprintf(line, "%.2f", f_val);
  strStripZeros(line);
  fieldPrintStr(fp, i_field, line, cp_post, i_justify);
}

void fieldPrintStr (FILE *fp, int i_field, char *cp_str, char *cp_post, int i_justify)
{
  if (fp)
    fprintf(fp, "%s%s", fieldGetStr(i_field, cp_str, i_justify), cp_post);
  else
    fieldMeasureStr(i_field, cp_str);
}

static void fieldMeasureStr (int i_field, char *cp_str)
{
  int len;

  if ((len = strlen(cp_str)) > gField.field[i_field].maxlen)
    gField.field[i_field].maxlen = len;
}

static char *fieldGetStr (int i_field, char *cp_str, int i_justify)
{
  return field_Justify(i_field, cp_str, i_justify);
}

static char *field_Justify (int i_field, char *cp_str, int i_justify)
     /* i_justify	LEFT_JUSTIFY or RIGHT_JUSTIFY */
{
  int i, j;
  int len;
  int diff;

  len = strlen(cp_str);
  diff = gField.field[i_field].maxlen - len;

  if (i_justify == RIGHT_JUSTIFY)
    {
      for (i = 0; i < diff; i++)
	g_field_line[i] = ' ';
      j = i;
      for (i = 0; i < len; i++, j++)
	g_field_line[j] = cp_str[i];
      g_field_line[j] = '\0';
    }
  else
    {
      for (i = 0; i < len; i++)
	g_field_line[i] = cp_str[i];
      j = i;
      for (i = 0; i < diff; i++, j++)
	g_field_line[j] = ' ';
      g_field_line[j] = '\0';
    }
  return g_field_line;
}
