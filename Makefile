#	$NetBSD$

all: intruder

intruder: intruder.o
	cc -o intruder intruder.o -lcurses -ggdb

intruder.o: intruder.c
	cc -c intruder.c -ggdb

PROG=	intruder
SRCS=	intruder.c
DPADD=	${LIBCURSES}
LDADD=	-lcurses
#HIDEGAME=hidegame
#SETGIDGAME=yes
MAN=	intruder.6
#WARNS=	2

.include <bsd.prog.mk>
.include <bsd.subdir.mk>
