#Note: to get debugging lines, do 'make DEBUG=1'

CC=gcc
ifeq "$(strip $(DEBUG))" ""
    CFLAGS=-O0 -pedantic -ansi -Wall
else
    CFLAGS=-pedantic -ansi -Wall -g -DDEBUG
endif

GOGGLE_OBJS= 	battle.o     \
		battleshow.o \
		calc.o       \
		errno.o      \
		field.o      \
		fileshow.o   \
		group.o      \
		lex.o        \
		main.o       \
		menu.o       \
		msg.o        \
		message.o    \
		planet.o     \
		planetshow.o \
		prompt.o     \
		readdata.o   \
		route.o      \
		screen.o     \
		ship.o       \
		shipshow.o   \
		util.o       \
		version.o
BATTLE_OBJS= 	battle.o     \
		battlemain.o \
		errno.o      \
		field.o      \
		get.o        \
		group.o      \
		lex.o        \
		message.o	\
		planet.o     \
		readdata.o   \
		ship.o       \
		util.o       \
		version.o
GOGGLE_LIBS=-lcurses -ltermcap -lm
BATTLE_LIBS=-lm

all: goggle battle

goggle: $(GOGGLE_OBJS)
	$(CC) -o goggle $(CFLAGS) $(GOGGLE_OBJS) $(GOGGLE_LIBS)

battle: $(BATTLE_OBJS)
	$(CC) -o battle $(CFLAGS) $(BATTLE_OBJS) $(BATTLE_LIBS)
