/*
 * File: errno.c
 * Author: Douglas Selph
 * Maintained by: Robin Powell
 * $Id: errno.c,v 1.3 1997/12/11 00:43:44 rlpowell Exp $
 */
#include <errno.h>
#include <stdio.h>
#include "common.h"

#include "errno.h"

#ifdef VAX_VMS
/*
 *  'errno' is set to this strange number below when
 *  a bad file name is received.  I may be wrong.
 */
#define BAD_NAME 65535

#else

#define BAD_NAME 0

extern int errno;
extern int sys_nerr;
extern char *sys_errlist[];

#endif
/**
 **  Local Structures and Defines
 **/
/**
 ** Functions:
 **/


int errnoMsg (char *cp_msg, char *cp_name)
{
#ifdef VAX_VMS
    if (errnoIsBadVal() && (cp_name != NULL))
    {
	sprintf(cp_msg, "Bad file name : \"%s\"", cp_name);
	return TRUE;
    }
#endif
    if (errno < sys_nerr)
    {
	if (cp_name == NULL)
	    strcpy(cp_msg, sys_errlist[errno]);
	else
	    switch (errno)
	    {
		case ENOENT :
		case EIO :
		case ENXIO :
		case EACCES :
		case EEXIST :
		case ENODEV :
		case ENOTDIR :
		case EISDIR :
		case ETXTBSY :
		case EFBIG :
		    sprintf(cp_msg, "%s : \"%s\"", sys_errlist[errno], cp_name);
		    break;
		default :
		    strcpy(cp_msg, sys_errlist[errno]);
		    break;
	    }
	return TRUE;
    }
    sprintf(cp_msg, "(Unknown error number %d) : \"%s\"", errno, cp_name);
    return FALSE;
}
