/*
 * Header file for message.c
 * $Id*
 */

void new_message( int type );	/* Starts a new message input segment.  Type
				   is one of TYPE_PMESSAGE or _GMESSAGE, for
				   personal or global messages respectively */
void new_message_line( const char *line );	/* Adds a line to the 
						   current message */


