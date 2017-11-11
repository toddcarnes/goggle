/*
 * File: message.c
 * Author: Robin Powell
 * $Id: message.c,v 1.1 1998/06/19 20:59:20 rlpowell Exp $
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "common.h"
#include "data.h"

#include "errno.h"
#include "main.h"
#include "message.h"

/* Numbers of messages we have so far */
static int pmessage_num=-1;
static int gmessage_num=-1;

/* Message holders, dynamic arrays of char *'s */
static char **pmessages=NULL, **gmessages=NULL;

/* Current message-type pointers */
static char **messages;
static int *message_num;
   
/* Starts a new message input segment.  Type is one of TYPE_PMESSAGE or
 * _GMESSAGE, for personal or global messages respectively.
 */
void new_message( int type )
{
    printf("\n\n*******Making new message.\n\n");
    /* Set up temporary variables as appropriate */ 
    if( type == TYPE_PMESSAGES )
    {
	messages = pmessages;
	message_num = &pmessage_num;
    }
    else
    {
	messages = gmessages;
	message_num = &gmessage_num;
    }

    /* Increment message_num and realloc pointers */	
    (*message_num)++;

#ifdef DEBUG
    printf( "Creating message %d of type %d.\n", *message_num, type );
#endif

    messages = realloc( messages, ( ((*message_num)+1) * sizeof( char * ) ) );
    
    messages[*message_num] = NULL;
    
#ifdef DEBUG
    printf( "Message pointer allocated.\n" );
#endif
} /* new_message() */



/* Adds a line to the current message */
void new_message_line( const char *line )
{
    int orig_len;

    if( ! messages[*message_num] )
    {
	orig_len=0;
    }
    else
    {
	orig_len=strlen( messages[*message_num] );
    }

    /* Allocate more space for the new line.  Note that a brand new message is
     * set to NULL
     */
    messages[*message_num] = (char *)realloc( (void *)messages[*message_num],
	    (orig_len + strlen( line ) + 1)*sizeof( char ) );
    
    messages[*message_num][orig_len] = '\0';

    /* Add line */
    sprintf( messages[*message_num], "%s%s\n", messages[*message_num], line );

#ifdef DEBUG
    printf( "Message: %s.\n", messages[*message_num] );
#endif
}
