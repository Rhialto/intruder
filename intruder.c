/*
 * $Id: intruder.c,v 1.4 2008/04/22 19:22:45 rhialto Exp $
 *
 * Intruder.
 *
 * Based on "intruder          kosmos s#28 v29.03.83"
 * by Olaf 'Rhialto' Seibert.
 */

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <setjmp.h>
#include <unistd.h>
#include <curses.h>

/* Emulation of screen position by an x,y pair */
struct xy {
    int x;
    int y;
};

int maze_delay_option;
int no_autoplay_option;
int rogue_option;
int use_ascii_option;
#define DIAGONAL_AUTOPLAY_ON	1
#define DIAGONAL_AUTOPLAY_OFF	2
int diagonal_autoplay_option = DIAGONAL_AUTOPLAY_OFF;
int random_direction_option = 0;
chtype blockade_key;
chtype acs_plus, acs_lrcorner, acs_urcorner, acs_llcorner, acs_ulcorner,
       acs_hline, acs_ttee, acs_btee, acs_vline, acs_rtee, acs_ltee,
       acs_diamond, acs_ckboard, acs_block;

void intruder(int lineno);
void make_maze();
void opening_screen();
void maze_screen();
void maze_screen_rebuild();
void screenpoke(struct xy pos, chtype ch);
chtype screenpeek(struct xy pos);
chtype input_with_timeout(int jiffies);
void place_blockade();
void getdx();
void print_score();
void clear_topline(int jiffies);
void auto_pilot();
void line_2110();

int number_of_guards;		/* m% */
int hm, i, j, jy, sk, sp, me, pn, be, bl;
int vb, e1, ap;
chtype en;
int wd, ho, mr, pw, t, d0;
int inchar;
float ab;

static
bool
equal_xy(struct xy a, struct xy b)
{
    return a.x == b.x && a.y == b.y;
}

static
struct xy
add_xy(struct xy a, struct xy b)
{
    a.x += b.x;
    a.y += b.y;

    return a;
}

static
struct xy
add_xy_twice(struct xy a, struct xy b)
{
    a.x += b.x + b.x;
    a.y += b.y + b.y;

    return a;
}

static
struct xy
subtract_xy(struct xy a, struct xy b)
{
    a.x -= b.x;
    a.y -= b.y;

    return a;
}

static
struct xy
subtract_xy_twice(struct xy a, struct xy b)
{
    a.x -= b.x;
    a.x -= b.x;
    a.y -= b.y;
    a.y -= b.y;

    return a;
}

struct xy player_pos, dx, a_xy[10+26+1];
struct xy a_a[4] = {
    {  1,  0 },
    {  0, -1 },
    { -1,  0 },
    {  0,  1 },
};
struct xy b_a[8] = {
    {  1,  0 },
    {  1, -1 },
    {  0, -1 },
    { -1, -1 },
    { -1,  0 },
    { -1,  1 },
    {  0,  1 },
    {  1,  1 },
};

void
sigint(int signo)
{
    endwin();
    exit(1);
}

void
reset_globals()
{
    number_of_guards =		/* m% */
    hm = i = j = jy = sk = sp = me = pn = be = bl =
    en = vb = e1 = ap =
    wd = ho = mr = pw = t = d0 =
    inchar =
    ab = 0;
    player_pos.x = player_pos.y = 0;
}

jmp_buf rerun;

int
main(int argc, char **argv)
{
    int lineno;
    int ch;

    extern char *optarg;
    extern int optind;

    while ((ch = getopt(argc, argv, "afd:in:r")) != -1) {
	switch (ch) {
	case 'a':
	    use_ascii_option = 1;
	    break;
	case 'd':
	    maze_delay_option = atoi(optarg);
	    break;
	case 'f':
	    no_autoplay_option = 1;
	    break;
	case 'i':
	    diagonal_autoplay_option = DIAGONAL_AUTOPLAY_ON;
	    break;
	case 'n':
	    random_direction_option = atoi(optarg);
	    break;
	case 'r':
	    rogue_option = 1;
	    break;
	case '?':
	default:
	    /*usage();*/;
	}
    }
    argc -= optind;
    argv += optind;

    blockade_key = rogue_option ? 'B' : 'b';

    initscr();

    if (use_ascii_option) {
	acs_plus = '+';
	acs_ulcorner = '.';
	acs_urcorner = '.';
	acs_llcorner = '`';
	acs_lrcorner = '\'';
	acs_hline = '-';
	acs_btee = '-';
	acs_ttee = '-';
	acs_vline = '|';
	acs_rtee = '|';
	acs_ltee = '|';
	acs_diamond = 'Z';
	acs_ckboard = '#';
	acs_block = 'O';
    } else {
	/* must be used after initscr */
	acs_plus = ACS_PLUS;
	acs_ulcorner = ACS_ULCORNER;
	acs_urcorner = ACS_URCORNER;
	acs_llcorner = ACS_LLCORNER;
	acs_lrcorner = ACS_LRCORNER;
	acs_hline = ACS_HLINE;
	acs_btee = ACS_BTEE;
	acs_ttee = ACS_TTEE;
	acs_vline = ACS_VLINE;
	acs_rtee = ACS_RTEE;
	acs_ltee = ACS_LTEE;
	acs_diamond = ACS_DIAMOND;
	acs_ckboard = ACS_CKBOARD;
	acs_block = ACS_BLOCK;
    }

    srandom((unsigned long)time(NULL));
    signal(SIGINT, sigint);
    cbreak();
    noecho();
    curs_set(0);
    keypad(stdscr, TRUE);
    lineno = setjmp(rerun);
    reset_globals();
    intruder(lineno);
    endwin();
}

void
intruder(int lineno)
{
    int dummy;

/*
intruder10,prg ==0401==
   10 d0=2:m%=1+rnd(1)*9:vb=1
   20 poke59468,12:print"{swuc}":goto110
  100 gosub8000
 */
    jy = '*';
    sp = ' ';	/* spatie: vrije plek */
    mr = acs_plus;	/* muur */
    pn = ':';	/* punt */
    be = '$';	/* bewaker */
    bl = acs_diamond;	/* blokkade */
    en = acs_block;	/* eind - dokumenten */
    if (lineno < 100 && !no_autoplay_option) {
	d0 = 4;
	number_of_guards = 1 + random() % 9;
	vb = 1;
	goto line_110;
    }
    opening_screen();
/*
  110 hm=32768:print"{clr}{down}a":wd=40:ifpeek(hm+wd)<>1thenwd=80
  120 i0=j0=i1=i=j=0:xy=hm+6*wd-10:jy=42:sk=10:sp=32:mr=91:pn=58:be=36:bl=90
  130 en=81:ab=2*m%:dimxy(9),d(4),a(3):d(1)=wd:d(2)=-1:d(3)=1:d(4)=-wd
 */
line_110:
    ab = 0; e1 = 0;
    hm = 32768;
    getmaxyx(stdscr, ho, wd);
    ho += 2;		/* fake extra lines, the code does not use them all */
    if (ho < 25 || wd < 40) {
	mvaddstr(0, 0, "Need a terminal of at least 40 x 23\n");
	refresh();
	return;
    }
    sk = 10;	/* skore */
    ab = 2 * number_of_guards;	/* aantal blokkades */
/*
  140 gosub 9000
 */
    maze_screen();

/*
  159 rem"{sret}{up}----------{rvon}plaats speler en bewakers
  160 pokexy,jy:fori=1tom%
  170 j=hm+3*wd+1+int((wd-9)*rnd(1))+wd*int(17*rnd(1)):ifpeek(j)<>pnthen170
  180 pokej,be:xy(i)=j:next
 */
    for (;;) {
	screenpoke(player_pos, jy);
	for (i = 1; i <= number_of_guards; i++) {
	    struct xy j;
	    do {
		j.x = 1 + random() % (wd - 9);
		j.y = 3 + random() % (ho - 8);
	    } while (screenpeek(j) != pn);
	    screenpoke(j, be);
	    a_xy[i] = j;
	}
    /*
    199 rem"{sret}{up}----------{rvon}beweeg speler
    200 pokexy,jy+128:ifvb=1thengosub2000:goto230
    205 dx=0:t=35:gosub1000:ifin$="q"then8510
    210 ifin$="b"thengosub2500:goto205
    220 gosub1100
    230 ifdxthensk=sk-1:ifsk<0then8540
    240 pw=peek(xy+dx):ifpw<>spthen500
    250 pokexy,sp:xy=xy+dx:pokexy,jy
    */
	for (;;) {
	/* line_200: */
	    screenpoke(player_pos,jy|A_REVERSE);
	    refresh();
	    if (vb) {
		auto_pilot();
	    } else {
	line_205:
		dx.x = dx.y = 0;
		inchar = input_with_timeout(40 * 1000 / 60);
		if (inchar == 'q')
		    goto line_8510;
		if (inchar == blockade_key) {
		    place_blockade();
		    goto line_205;
		}
		getdx();
	    }

	    if (dx.x || dx.y) {
		sk--;
		if (sk < 0)
		    goto line_8540;
	    }
	    pw = screenpeek(add_xy(player_pos, dx));
	    if (pw != sp) {
	    /*
	    499 rem"{sret}{up}----------{rvon}verwerk tekens
	    500 ifpw=blthenab=ab+.25:goto250
	    510 ifpw=bethensk=sk+150:pokexy+dx,be+128
	    530 ifpw=enande1=0then8520
	    540 ifpw=enthen8530
	    550 ifpw=pnthen700
	    600 dx=0:goto250
	    */
		if (pw == bl) {
		    ab += 1/4;
		} else if (pw == be) {
		    sk += 150;
		    screenpoke(add_xy(player_pos, dx), be|A_REVERSE);
		    refresh();
		    dx.x = dx.y = 0;
		} else if (pw == en && e1 == 0) {
		/*
		8520 print"{home}maak nu dat je weer buiten komt{home}{down}{down}{down}{down}{down}"tab(wd-10)"Q->uit":e1=1
		8521 sk=sk+100:gosub1050:t=60:gosub8990:goto250
		*/
		    mvaddstr(0, 0, "maak nu dat je weer buiten komt");
		    move(5, wd-10); addch(en); addstr("->uit");
		    /* refresh(); done in print_score */
		    e1 = 1;
		    sk += 100;
		    print_score();
		    clear_topline(60);
		} else if (pw == en) {
		    goto line_8530;
		} else if (pw == pn) {
		    goto line_700;
		} else {
		    dx.x = dx.y = 0;
		}
	    }
	line_250:
	    screenpoke(player_pos, sp);
	    player_pos = add_xy(player_pos, dx);
	    screenpoke(player_pos, jy);
	    refresh();
	    usleep(100*1000);
	/*
	259 rem"{sret}{up}----------{rvon}beweeg bewakers
	260 gosub1050:fori=1tom%:j=0
	280 j=j+1:dx=d(1+rnd(1)*4):pw=peek(xy(i)+dx):ifpw<>spandpw<>pnandj<5then280
	285 ifj>4thendx=0
	290 pokexy(i),sp:xy(i)=xy(i)+dx:pokexy(i),be
	300 next
	*/
	    print_score();
	    for (i = 1; i <= number_of_guards; i++) {
		j = 0;
		do {
		    j++;
		    dx = a_a[random() % 4];
		    pw = screenpeek(add_xy(a_xy[i], dx));
		} while (pw != sp && pw != pn && j < 5);
		if (j > 4)
		    dx.x = dx.y = 0;
		screenpoke(a_xy[i], sp);
		a_xy[i] = add_xy(a_xy[i], dx);
		screenpoke(a_xy[i], be);
	    }
	    refresh();
	/*
	309 rem"{sret}{up}----------{rvon}test aanwezigheid bewakers
	310 fori=1to4:dx=d(i):forj=1to10
	320 i0=xy+dx*j:pw=peek(i0):ifpw<>bethen400
	330 j0=0:fori1=1tom%:ifi0=xy(i1)thenj0=i1:i1=m%
	340 next:ifj0=0then400
	350 xy(j0)=xy(j0)-dx:pokei0,sp:pokei0-dx,be
	360 ifxy(j0)=xythen8500
	400 ifpw<>spandpw<>pnthenj=10
	410 nextj:nexti:fori=0to1
	*/
	    for (i = 0; i < 4; i++) {		/* i is the direction index */
		dx = a_a[i];
		struct xy i0 = player_pos;
		for (j = 1; j <= 10; j++) {	/* j is the distance */
		    i0 = add_xy(i0, dx);	/* i0 is the examined l0cation */
		    pw = screenpeek(i0);
		    if (pw == be) {
			int i1;
			int j0 = 0;

			for (i1 = 1; i1 <= number_of_guards; i1++) {
			    if (equal_xy(i0, a_xy[i1])) {
				j0 = i1;	/* j0 is the guard that saw us */
				i1 = number_of_guards;
			    }
			}
			if (j0) {
			    a_xy[j0] = subtract_xy(a_xy[j0], dx);
			    screenpoke(i0, sp);
			    screenpoke(a_xy[j0], be);
			    refresh();
			    if (equal_xy(a_xy[j0], player_pos))
				goto line_8500;
			}
		    } else if (pw != sp && pw != pn) {
			j = 10;
		    }
		}
	    }
	/*
	419 rem"{sret}{up}----------{rvon}verander doolhof
	420 j=hm+3*wd+1+int((wd-11)*rnd(1))+wd*int(17*rnd(1)):pw=peek(j)
	430 ifpw<>pnandpw<>mrthen450
	440 pokej,pn+i*(mr-pn)
	450 next:goto200
	*/
	    for (j = (ho / 15); j > 0; j--) {	/* when ho==25, j must be 1 */
		for (i = 0; i <= 1; i++) {
		    struct xy j;
		    j.x = 1 + random() % (wd - 11);
		    j.y = 3 + random() % (ho - 8);
		    pw = screenpeek(j);
		    if (pw == pn || pw == mr) {
			screenpoke(j, pn + i * (mr - pn));
		    }
		}
	    }
	    refresh();
	    /* goto line_200; */
	}
    /*
    699 rem"{sret}{up}----------{rvon}vrij spel???
    700 sk=sk+int(4+m%/2):ap=ap+1:ifap<4*wdthen250
    710 print"{home}wil je een vrij spel (j/n)? ";:t=1e9:ifvbthen720
    715 gosub1010:ifin$="n"thenprint"nee":goto8550
    716 ifin$<>"j"then715
    720 print"ja":xy=hm+6*wd-10:gosub9010:ap=0:e1=0:ab=ab+2*m%:gosub8995:goto160
    */
    line_700:
	sk += 4 + number_of_guards/2;
	ap++;
	if (ap < (ho / 6) * wd)		/* when ho==25, must be 4*wd */
	    goto line_250;
	mvaddstr(0, 0, "wil je een vrij spel (j/n)? ");
	if (!vb) {
	    do {
		inchar = input_with_timeout(-1);
		if (inchar == 'n') {
		    addstr("nee");
		    refresh();
		    ap = 0; goto line_250;	/* was 8550 */
		    goto line_8550;
		}
	    } while (inchar != 'j');
	}
	addstr("ja");
	refresh();
	maze_screen_rebuild();
	ap = 0;
	e1 = 0;
	ab += 2 * number_of_guards;
	clear_topline(0);
    }
/*
  999 rem"{rvs-shift-m}{up}{10 -}{rvs} *tekst*
 */
line_8500:;
/*
 8500 fori=0to99:pokexy,jy::::::pokexy,be+128:::::::next:sk=int(sk/2):gosub1050
 8505 print"{home}de bewaker heeft je gevangen: het spel  is afgelopen.":goto8550
 */
    for (i = 0; i < 10; i++) {
	screenpoke(player_pos, jy);
	refresh();
	usleep(100*1000);
	screenpoke(player_pos, be|A_REVERSE);
	refresh();
	usleep(100*1000);
    }
    mvaddstr(0, 0, "de bewaker heeft je gevangen: het spel  is afgelopen.");
    sk /= 2;
    print_score();
    goto line_8550;
line_8510:;
/*
 8510 print"{home}je hebt het opgegeven. jammer....":sk=int(sk/2):goto8550
 */
    mvaddstr(0, 0, "je hebt het opgegeven. jammer....");
    refresh();
    sk /= 2;
    goto line_8550;
line_8530:;
/*
 8530 print"{home}je opdrachtgevers zijn je zeer dankbaar!":pokexy,sp:pokexy+dx,jy
 8531 sk=sk+100:gosub1050:t=60:goto8550
 */
    mvaddstr(0, 0, "je opdrachtgevers zijn je zeer dankbaar!");
    refresh();
    screenpoke(player_pos, sp);
    screenpoke(add_xy(player_pos, dx), jy);
    sk += 100;
    print_score();
    t = 60;
    goto line_8550;
line_8540:;
/*
 8540 print"{home}je punten zijn op: je hebt verloren
 */
    mvaddstr(0, 0, "je punten zijn op: je hebt verloren");
    refresh();
line_8550:
/*
 8550 ifvb=1thent=120:gosub8990:run
 8555 print"{home}{down}{down}{down}{down}{down}{down}{down}{down}"tab(wd-9)"{rvon}wil je
 8560 printtab(wd-9)"{rvon}nog{down}{left}{left}{left}eens?{down}{left}{left}{left}{left}{left}(j/n)
 8570 t=1e9:gosub1010:ifin$="j"thenrun100
 8580 ifin$<>"n"then8570
 8590 print"{clr}":end
 */
    if (vb) {
	clear_topline(120);
	longjmp(rerun, 0);
    }
    attron(A_REVERSE);
    mvaddstr( 8, wd - 9, "wil je");
    mvaddstr( 9, wd - 9, "nog");
    mvaddstr(10, wd - 9, "eens?");
    mvaddstr(11, wd - 9, "(j/n)");
    attroff(A_REVERSE);
    do {
	inchar = input_with_timeout(-1);
	if (inchar == 'j') {
	    longjmp(rerun, 100);
	}
    } while (inchar != 'n');
    clear();
    return;
}
/*
 1000 ifpeek(158)=0thenpoke151,0
 1010 t1=ti
 1011 getin$:ifin$=""andti-t1<tthen1011
 1015 return
 */
/*
 * The fancy non-typeahead autorepeat trick with the poke is
 * impossible to reproduce :-(
 * How it works:
 * peek(158) is the size of the input queue. If it is empty, the previously
 * held-down key is set to 0, so that it appears if you momentarily let
 * go of it, and a "new" press is registered into the input queue.
 * Of course when no key is pressed, it does nothing.
 */
chtype
input_with_timeout(int msec)
{
    chtype in;

    refresh();
    timeout(msec);
    for (;;) {
	in = getch();
	if (in != '\f')
	    break;
	refresh();
    }
    return in;
}

/*
 1050 print"{home}{down}{down}{down}{rvon}"tab(wd-9);right$("     "+mid$(str$(sk),2),5):return
 */

void
print_score()
{
    move(3, wd - 9);
    attron(A_REVERSE);
    printw("%5d", sk);
    attroff(A_REVERSE);
    refresh();
}

/*
 1100 ifin$="1"thendx=wd-1
 1110 ifin$="2"thendx=wd
 1120 ifin$="3"thendx=wd+1
 1130 ifin$="4"thendx=-1
 1140 ifin$="6"thendx=1
 1150 ifin$="7"thendx=-wd-1
 1160 ifin$="8"thendx=-wd
 1170 ifin$="9"thendx=-wd+1
 1180 return
*/
void
getdx()
{
    switch (inchar) {
	case '1': case 'b': case KEY_END:
	    dx.x = -1; dx.y =  1;
	    break;
	case '2': case 'j': case KEY_DOWN:
	    dx.x =  0; dx.y =  1;
	    break;
	case '3': case 'n': case KEY_NPAGE:
	    dx.x =  1; dx.y =  1;
	    break;
	case '4': case 'h': case KEY_LEFT:
	    dx.x = -1; dx.y =  0;
	    break;
	case '6': case 'l': case KEY_RIGHT:
	    dx.x =  1; dx.y =  0;
	    break;
	case '7': case 'y': case KEY_HOME:
	    dx.x = -1; dx.y = -1;
	    break;
	case '8': case 'k': case KEY_UP:
	    dx.x =  0; dx.y = -1;
	    break;
	case '9': case 'u': case KEY_PPAGE:
	    dx.x =  1; dx.y = -1;
	    break;
    }
    return;
}
/*
 1999 rem"{sret}{up}----------{rvon}auto spel
 2000 getin$:ifin$then run100
 2010 j=d0
 2020 dx=a(d0)/2:gosub2110:ifi=1then1050
 2025 d0=d0-1:ifd0<0thend0=3
 2030 ifd0<>jthen2020
 2100 print"{home}ik geef het  op,   want  ik  heb  geen  blokkades meer!":t=60:goto8990
 */
void
auto_pilot()
{
    inchar = input_with_timeout(1);
    if (inchar != ERR) {
	longjmp(rerun, 100);
    }
    if ((random() % 1000) < random_direction_option) {
	d0 = random() % 8;
	if (diagonal_autoplay_option == DIAGONAL_AUTOPLAY_OFF)
	    d0 &= ~1;
    }
#if 0
    if ((random() % 1000) < 0) {
	diagonal_autoplay_option = DIAGONAL_AUTOPLAY_ON +
		DIAGONAL_AUTOPLAY_OFF - diagonal_autoplay_option;
	d0 &= ~1;
    }
#endif
    j = d0;
    do {
	dx = b_a[d0];
/*
 2110 i=0:pw=peek(xy+dx):ifpw=sporpw=enorpw=pntheni=1
 2120 ifi=1thend0=d0+1:ifd0>3thend0=0
 2130 return
 */
	pw = screenpeek(add_xy(player_pos, dx));
	if (pw == sp || pw == en || pw == pn) {
	    /* It's just a step to the left */
	    d0 = (d0 + 4 - diagonal_autoplay_option) % 8;
	    return;
	}
	/* and then a jump to the right */
	d0 -= diagonal_autoplay_option;
	if (d0 < 0)
	    d0 += 8;
	/* let's do the timewarp again */
    } while (d0 != j);
    mvaddstr(0, 0, "ik geef het  op,   want  ik  heb  geen  blokkades meer!");
    refresh();
    clear_topline(60);
}

/*
 2500 ifab<1thenprint"{home}je hebt geen blokkades meer!":t=60:goto8990
 2505 print"{home}druk  richtingtoets  voor  blokkade  (0 voor geen)
 2510 t=1e9:gosub1010:dx=0:gosub1100
 2520 pw=peek(xy+dx):ifdx=0then2600
 2525 ifpw<>mrandpw<>spandpw<>pnthengosub8995:goto2500
 2530 pokexy+dx,bl:ab=ab-1
 2600 gosub8995:return
 */
void
place_blockade()
{
line_2500:
    if (ab < 1) {
	mvaddstr(0, 0, "je hebt geen blokkades meer!");
	refresh();
	clear_topline(60);
    } else {
	mvaddstr(0, 0, "druk  richtingtoets  voor  blokkade  (0 voor geen)");
	inchar = input_with_timeout(-1);
	getdx();
	pw = screenpeek(add_xy(player_pos, dx));
	if (dx.x || dx.y) {
	    if (pw != mr && pw != sp && pw != pn) {
		clear_topline(0);
		goto line_2500;
	    }
	    screenpoke(add_xy(player_pos, dx), bl);
	    ab--;
	}
	clear_topline(0);
    }
}

void
normalise_xy(struct xy *pos)
{
    if (pos->x < 0) {
	pos->x += wd;
	pos->y -= 1;
    }
}

void
screenpoke(struct xy pos, chtype ch)
{
    struct xy old_cursor_pos;

    getyx(stdscr, old_cursor_pos.y, old_cursor_pos.x);
    /*normalise_xy(&pos);*/
    mvaddch(pos.y, pos.x, ch);
    if (maze_delay_option >= 0)
	refresh();
    move(old_cursor_pos.y, old_cursor_pos.x);
}

chtype
screenpeek(struct xy pos)
{
    struct xy old_cursor_pos;
    chtype ch;

    getyx(stdscr, old_cursor_pos.y, old_cursor_pos.x);
    normalise_xy(&pos);
    move(pos.y, pos.x);
    ch = inch();
    move(old_cursor_pos.y, old_cursor_pos.x);

    return ch;
}

/*
 7500 a(0)=2:a(1)=-2*wd:a(2)=-2:a(3)=2*wd
 7520 a=xy-1:pokea,4:rem begin doolhof
 7550 j=int(rnd(1)*4):x=j
 7560 b=a+a(j):ifpeek(b)=mrthenpokeb,j:pokea+a(j)/2,pn:a=b:goto7550
 7570 j=(j+1)*-(j<3):ifj<>xthen7560
 7580 j=peek(a):pokea,pn:ifj<4thena=a-a(j):goto7550
 7590 return
 */
void
make_maze()
{
    struct xy a, b;
    int j, x;

    a = player_pos;
    a.x--;
    screenpoke(a, '@' + 4);
    for (;;) {
top:
	if (maze_delay_option > 0)
	    usleep(maze_delay_option * 100);
	x = j = random() % 4;
	do {
	    b = add_xy_twice(a, a_a[j]);
	    if (screenpeek(b) == mr) {
		screenpoke(b, '@' + j);
		screenpoke(add_xy(a, a_a[j]), pn);
		a = b;
		goto top;
	    }
	    j = (j + 1) % 4;
	} while (j != x);
	j = screenpeek(a) - '@';
	screenpoke(a, pn);
	if (j >= 4)
	    break;
	a = subtract_xy_twice(a, a_a[j]);
    }
}

/*
 8000 print"{clr}intruder          kosmos s#28 v29.03.83"
 8010 forj=1to39:print"{CBM-T}";:next:print
 8015 print"       {CBM-@}{CBM-@}{CBM-@}{CBM-@}{CBM-@}{CBM-@}{CBM-@}{CBM-@}{CBM-@}{CBM-@}{CBM-@}{CBM-@}{CBM-@}
 8017 print"       {rvon}puntentelling{rvof}:{down}
 8020 print"         jij *
 8030 print"    blokkade Z houdt bewakers tegen
 8035 print"dubbele punt : 3-7 afh van # bewakers
 8040 print"  dokumenten Q 75 of 100
 8050 print"     bewaker $ 150
 8060 print"{down}{down}spel afgelopen als:
 8070 print"{down}-Q bij uitgang gepakt, of
 8080 print"-geen punten meer, of
 8090 print"-$ grijpt jou (je skore halveert dan!)
 8100 print"de dan behaalde skore telt.
 8190 print"{CBM-@}{CBM-@}{CBM-@}{CBM-@}{CBM-@}{CBM-@}{CBM-@}{CBM-@}{CBM-@}{CBM-@}{CBM-@}{CBM-@}{CBM-@}{CBM-@}{CBM-@}{CBM-@}{CBM-@}{CBM-@}{CBM-@}{CBM-@}{CBM-@}{CBM-@}{CBM-@}{CBM-@}{CBM-@}{CBM-@}{CBM-@}{CBM-@}{CBM-@}{CBM-@}
 8200 print"{rvon}hoeveel bewakers wil je [1-9]?
 8210 t=1e9:gosub1010:m%=val(in$):ifm%<1orm%>9then8210
 8220 return
 */
void
opening_screen()
{
    int ch;

    clear();
    addstr("intruder          kosmos s#28 v29.03.83\n");
    addstr("---------------------------------------\n");
    addstr("       _____________\n");
    addstr("       ");
    attron(A_REVERSE);
    addstr("puntentelling");
    attroff(A_REVERSE);
    addstr(":\n\n");
    addstr("         jij "); addch(jy); addstr("\n");
    addstr("    blokkade "); addch(bl); addstr(" houdt bewakers tegen\n");
    addstr("dubbele punt "); addch(pn); addstr(" 3-7 afh van # bewakers\n");
    addstr("  dokumenten "); addch(en); addstr(" 75 of 100\n");
    addstr("     bewaker "); addch(be); addstr(" 150\n");
    addstr("\n");
    addstr("\n");
    addstr("spel afgelopen als:\n");
    addch('-'); addch(en); addstr(" bij uitgang gepakt, of\n");
    addstr("-geen punten meer, of\n");
    addch('-'); addch(be); addstr(" grijpt jou (je skore halveert dan!)\n");
    addstr("de dan behaalde skore telt.\n");
    addstr("______________________________\n");
    attron(A_REVERSE);
    addstr("hoeveel bewakers wil je [1-9]?");
    attroff(A_REVERSE);
    addstr(" ");

    refresh();
    do {
	ch = input_with_timeout(-1);

	number_of_guards = ch <= '9' ?  ch - '0' : ch - 'a';
    } while (number_of_guards < 1 || number_of_guards > 9+26);
}

/*
 8990 :t1=ti:fori=-1to0:i=(ti-t1<t):next
 8995 print"{home}";:fori=1to35:print"  ";:next:print:return
 */
void
clear_topline(int jiffies)
{
    if (jiffies) {
	while (jiffies >= 60) {
	    sleep(1);
	    jiffies -= 60;
	}
	usleep(1000*1000L * jiffies / 60);
    }
    mvaddstr(0, 0, "                                                                      ");
    refresh();
}

/*
 9000 print"{clr}ga  naar  de  kluis  en  steel  geheime dokumenten (Q)...
 */

void
maze_screen()
{
    clear();
    addstr("ga  naar  de  kluis  en  steel  geheime dokumenten (");
    addch(en);
    addstr(")...\n");
    maze_screen_rebuild();
}

/*
 9010 in$="{CBM-Q}[[[[[[[[[[[[[[[[[[[[[[[[[[[[[
 9011 ifwd=80thenin$=in$+"[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[
 9020 print"{home}{down}{down}{CBM-A}";:fori=1to(wd-10)/10:print"{CBM-R}{CBM-R}{CBM-R}{CBM-R}{CBM-R}{CBM-R}{CBM-R}{CBM-R}{CBM-R}{CBM-R}";:next:print"{left}{CBM-S}{CBM-@}{CBM-@}{CBM-@}{CBM-@}{CBM-@}
 9025 printin$;"{CBM-W}":gosub1050
 9030 printin$;"[@@@@@{CBM-S}
 9040 printin$;" <- in]
 9050 printin$;"[{CBM-S}gang]
 9060 printin$;"{CBM-W}{CBM-Z}@@@@{CBM-X}
 9070 printin$;"{CBM-W}
 9080 printin$;"{CBM-W}
 9090 printin$;"{CBM-W}
 9100 printin$;"{CBM-W}
 9110 printin$;"{CBM-W}
 9120 printin$;"{CBM-W}{rvon}b{rvof}lok-
 9130 printin$;"{CBM-W} kade
 9140 printin$;"{CBM-W}{rvon}q{rvof}uit
 9150 printin$;"{CBM-W}  8
 9160 printin$;"{CBM-W} {CBM-A}{CBM-E}{CBM-S}
 9170 printin$;"{CBM-W}4{CBM-W}{rvon}*{rvof}{CBM-Q}6
 9180 printin$;"{CBM-W} {CBM-Z}{CBM-R}{CBM-X}
 9190 print"{CBM-Z}";:fori=1to(wd-10)/10:print"{CBM-E}{CBM-E}{CBM-E}{CBM-E}{CBM-E}{CBM-E}{CBM-E}{CBM-E}{CBM-E}{CBM-E}";:next:print"{left}{CBM-X}  2
 9195 i=(wd-40)/2
 9200 printtab(i)"$=bewaker      *=jij     Q=doel
 9205 printtab(i)"     ***{rvon}even geduld aub{rvof}***{up}":gosub7500
 9210 printtab(i);:forj=1to19:print"  ";:next:print
 9215 ifvb=1thenprinttab(i)"***{rvon}voorbeeld{rvof}***{rvon}druk 'n toets{rvof}***
 9220 print"{home}{down}{down}":fori=1to12*rnd(1):print"{down}";:next:i=1+(wd-17)*rnd(1)
 9230 printtab(i)"{CBM-+}{CBM-+}{CBM-+} {CBM-+}{CBM-+}{CBM-+}
 9240 printtab(i)"{CBM-+} Z Z {CBM-+}
 9250 printtab(i)"   Q   "
 9260 printtab(i)"{CBM-+} Z Z {CBM-+}
 9270 printtab(i)"{CBM-+}{CBM-+}{CBM-+} {CBM-+}{CBM-+}{CBM-+}
 9280 ifrnd(1)>.02then9490
 9290 be=sp+64:gosub8995:print"{home}pech: de bewakers zijn onzichtbaar deze keer
 9300 sk=sk+90:gosub1050
 9490 ti$="000000":t=60:goto8990
*/

void
print_row()	/* helper function to print in$ */
{
    int i;

    addch(acs_ltee);
    for (i = 0; i < wd - 11; i++)
	addch(mr);
}

void
maze_screen_rebuild()
{
    player_pos.y = 5;
    player_pos.x = wd - 10;

    mvaddch(2, 0, acs_ulcorner);
    for (i = 0; i < wd - 11; i++) {
	addch(acs_ttee);
    }
    addch(acs_urcorner);
    addch('\n');
    print_row(); addch(acs_rtee); print_score(); addch('\n');
    print_row(); addch(acs_btee);
	         addch(acs_hline);
	         addch(acs_hline);
	         addch(acs_hline);
	         addch(acs_hline);
	         addch(acs_hline);
	         addch(acs_urcorner);
	         addch('\n');
    print_row(); addstr(" <- in"); addch(acs_vline); addch('\n');
    print_row(); addch(acs_ttee); addch(acs_urcorner);
		 addstr("gang"); addch(acs_vline); addch('\n');
    print_row(); addch(acs_rtee);
		 addch(acs_llcorner);
	         addch(acs_hline);
	         addch(acs_hline);
	         addch(acs_hline);
	         addch(acs_hline);
		 addch(acs_lrcorner);
	         addch('\n');
    for (i = 0; i < ho - 20; i++) {
	print_row(); addch(acs_rtee); addch('\n');
    }
    print_row(); addch(acs_rtee); addch('b'|A_REVERSE); addstr("lok-\n");
    print_row(); addch(acs_rtee); addstr(" kade\n");
    print_row(); addch(acs_rtee); addch('q'|A_REVERSE); addstr("uit\n");
    print_row(); addch(acs_rtee); addstr("7 8 9\n");
    print_row(); addch(acs_rtee); addch(' ');
				  addch(acs_ulcorner);
				  addch(acs_btee);
				  addch(acs_urcorner);
				  addch('\n');
    print_row(); addch(acs_rtee); addch('4');
				  addch(acs_rtee);
			          addch('*'|A_REVERSE);
				  addch(acs_ltee);
				  addstr("6\n");
    print_row(); addch(acs_rtee); addch(' ');
				  addch(acs_llcorner);
				  addch(acs_ttee);
				  addch(acs_lrcorner);
				  addch('\n');
    addch(acs_llcorner);
    for (i = 0; i < wd - 11; i++) {
	addch(acs_btee);
    }
    addch(acs_lrcorner);
    addstr("1 2 3\n");

    i = (wd - 40) / 2;
    mvaddstr(getcury(stdscr), i, "$=bewaker      *=jij     ");addch(en);addstr("=doel\n");
    mvaddstr(getcury(stdscr), i, "     ***");
	    attron(A_REVERSE); addstr("even geduld aub"); attroff(A_REVERSE);
	    addstr("***");

    make_maze();

    i = (wd - 40) / 2;
    mvaddstr(getcury(stdscr), i, "                               ");
    if (vb) {
	mvaddstr(getcury(stdscr), i, "***");
	    attron(A_REVERSE); addstr("voorbeeld"); attroff(A_REVERSE);
	    addstr("***");
	    attron(A_REVERSE); addstr("druk 'n toets"); attroff(A_REVERSE);
	    addstr("***");
    }

    i = 1 + random() % (wd - 17);
    j = 3 + random() % (ho - 14);
    move(j  ,i); addch(acs_ckboard);addch(acs_ckboard);addch(acs_ckboard);
	         addch(' ');
                 addch(acs_ckboard);addch(acs_ckboard);addch(acs_ckboard);
    move(j+1,i); addch(acs_ckboard);addch(' ');addch(bl);
	         addch(' ');
                 addch(bl);addch(' ');addch(acs_ckboard);
    mvaddstr(j+2, i, "   "); addch(en); addstr("   ");
    move(j+3,i); addch(acs_ckboard);addch(' ');addch(bl);
	         addch(' ');
                 addch(bl);addch(' ');addch(acs_ckboard);
    move(j+4,i); addch(acs_ckboard);addch(acs_ckboard);addch(acs_ckboard);
	         addch(' ');
                 addch(acs_ckboard);addch(acs_ckboard);addch(acs_ckboard);

    refresh();

    clear_topline(60);
}
