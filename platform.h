#ifndef PLATFORM_H
#define PLATFORM_H
/*
 * File: platform.h
 * Author: Douglas Selph
 * Maintained by: Robin Powell
 * $Id: platform.h,v 1.4 1997/12/13 05:35:31 rlpowell Exp $
 * Purpose:
 *
 *   (1) Set the HELP_PATH variable below to be
 *     the directory where the README* help files
 *     may be found by goggle.  By default, just
 *     set this to the directory where the Goggle
 *     is made.
 *   (2)
 *	(a) If you are an HP system:
 *          Uncomment 'HP' and comment out the SUN4 define.
 *      (b) If you are an DEC system:
 *          Uncomment 'DEC' and comment out the SUN4 define.
 *	Otherwise:
 *          Just leave the SUN4 define.
 *   (3) If you are having problems with space usage,
 *     that is, if you are compiling on an IBM machine
 *     where your data segment size may be limited, then
 *     you may want to adjust the number of races.
 *     This will help a little bit in saving memory on
 *     the data segment.  The program will complain
 *     if the reduced value is too small.
 *	
 *	 Suggested minimum sizes for a 150x150 galaxy:
 *	    NUM_RACES   30	-- = # players + 2 --
 *	 Suggested minimum sizes for a 110x110 galaxy:
 *	    NUM_RACES   20	-- = # players + 2 --
 *    (4) If you get a link error saying ungetch is undefined,
 *	    uncomment the WRONG_CURSES define.  Please report
 *	    any other linker errors to me.
 */
/* (1) */
#define HELP_PATH "~/goggle"

/* (2) */
#define SUN4
/* #define HP */
/* #define DEC */

/* (3) */
#define NUM_RACES 60		/* the max number of players */

/* (4) */
/* #define WRONG_CURSES */

/* THAT'S IT! */

#define CONTROL(C) ((C)&037)

#ifdef SUN4

#define IsBackSpace(ch) ((ch) == erasechar())

#else

#define IsBackSpace(ch) ((ch) == CONTROL('h'))

#endif

#ifdef HP

typedef caddr_t pointer;

#else

typedef char *pointer;

#endif

/* using random() and srandom() is better
 *  but galaxy does not use this, so
 *  the simulator would be inaccurate I suppose
 */
#define RAND() rand()
#define SRAND(V) srand(V)

#endif
