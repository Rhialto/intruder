#	$NetBSD$

PROG=	intruder
SRCS=	intruder.c
DPADD=	${LIBCURSES}
LDADD=	-lcurses
#HIDEGAME=hidegame
#SETGIDGAME=yes
MAN=	intruder.6
#WARNS=	2

intruder.ps: intruder.6
	groff -mandoc -Tps intruder.6 > intruder.ps.tmp && mv intruder.ps.tmp intruder.ps

intruder.dvi: intruder.6
	groff -mandoc -Tdvi intruder.6 > intruder.dvi.tmp && mv intruder.dvi.tmp intruder.dvi

tarfile:
	tar cvfz intruder.tar.gz README Makefile intruder.c intruder.6 intruder.cat6 intruder.basic intruder10* intruder_demo.png

.include <bsd.prog.mk>
