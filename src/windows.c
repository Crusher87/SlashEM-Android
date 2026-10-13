/*	SCCS Id: @(#)windows.c	3.4	1996/05/19	*/
/* Copyright (c) D. Cohrs, 1993. */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"
#ifdef TTY_GRAPHICS
#include "wintty.h"
#endif
#ifdef X11_GRAPHICS
/* cannot just blindly include winX.h without including all of X11 stuff */
/* and must get the order of include files right.  Don't bother */
extern struct window_procs X11_procs;
extern void NDECL(win_X11_init);
#endif
#ifdef QT_GRAPHICS
extern struct window_procs Qt_procs;
#endif
#ifdef GTK_GRAPHICS
/*
 * GTK interface (By issei@guru.gr.jp)
 */
extern struct window_procs GTK_procs;
extern void NDECL(win_GTK_init);
#endif
#ifdef GEM_GRAPHICS
#include "wingem.h"
#endif
#ifdef MAC
extern struct window_procs mac_procs;
#endif
#ifdef BEOS_GRAPHICS
extern struct window_procs beos_procs;
extern void NDECL(be_win_init);
#endif
#ifdef AMIGA_INTUITION
extern struct window_procs amii_procs;
extern struct window_procs amiv_procs;
extern void NDECL(ami_wininit_data);
#endif
#ifdef WIN32_GRAPHICS
extern struct window_procs win32_procs;
#endif
#ifdef GNOME_GRAPHICS
#include "winGnome.h"
extern struct window_procs Gnome_procs;
#endif
#ifdef GL_GRAPHICS
#include "winGL.h"
extern struct window_procs sdlgl_hardw_procs;
#endif
#ifdef SDL_GRAPHICS
#include "winGL.h"
extern struct window_procs sdlgl_softw_procs;
#endif
#ifdef PROXY_GRAPHICS
#include "winproxy.h"
extern struct window_procs proxy_procs;
extern void NDECL(win_proxy_init);
#endif
#ifdef MSWIN_GRAPHICS
extern struct window_procs mswin_procs;
#endif
#ifdef ANDROID_GRAPHICS
extern struct window_procs and_procs;
#endif

STATIC_DCL void FDECL(def_raw_print, (const char *s));
#ifdef DUMPLOG
STATIC_DCL winid FDECL(dump_create_nhwindow, (int));
STATIC_DCL void FDECL(dump_clear_nhwindow, (winid));
STATIC_DCL void FDECL(dump_display_nhwindow, (winid, BOOLEAN_P));
STATIC_DCL void FDECL(dump_destroy_nhwindow, (winid));
STATIC_DCL void FDECL(dump_curs, (winid,int,int));
STATIC_DCL void FDECL(dump_putstr, (winid, int, const char *));
STATIC_DCL void FDECL(dump_start_menu, (winid));
STATIC_DCL void FDECL(dump_add_menu, (winid,int,const ANY_P *,
		CHAR_P,CHAR_P,int,const char *, BOOLEAN_P));
STATIC_DCL void FDECL(dump_end_menu, (winid, const char *));
STATIC_DCL int FDECL(dump_select_menu, (winid, int, MENU_ITEM_P **));
STATIC_DCL void FDECL(dump_status_curs, (winid,int,int));
STATIC_DCL void FDECL(dump_status_putstr, (winid, int, const char *));
#endif

NEARDATA struct window_procs windowprocs;

static
struct win_choices {
    struct window_procs *procs;
    void NDECL((*ini_routine));		/* optional (can be 0) */
} winchoices[] = {
#ifdef TTY_GRAPHICS
    { &tty_procs, win_tty_init },
#endif
#ifdef X11_GRAPHICS
    { &X11_procs, win_X11_init },
#endif
#ifdef QT_GRAPHICS
    { &Qt_procs, 0 },
#endif
#ifdef GTK_GRAPHICS
    { &GTK_procs, win_GTK_init },
#endif
#ifdef GEM_GRAPHICS
    { &Gem_procs, win_Gem_init },
#endif
#ifdef MAC
    { &mac_procs, 0 },
#endif
#ifdef BEOS_GRAPHICS
    { &beos_procs, be_win_init },
#endif
#ifdef AMIGA_INTUITION
    { &amii_procs, ami_wininit_data },		/* Old font version of the game */
    { &amiv_procs, ami_wininit_data },		/* Tile version of the game */
#endif
#ifdef WIN32_GRAPHICS
    { &win32_procs, 0 },
#endif
#ifdef GNOME_GRAPHICS
    { &Gnome_procs, 0 },
#endif
#ifdef GL_GRAPHICS
    { &sdlgl_hardw_procs, 0 },
#endif
#ifdef SDL_GRAPHICS
    { &sdlgl_softw_procs, 0 },
#endif
#ifdef PROXY_GRAPHICS
    { &proxy_procs, win_proxy_init },
#endif
#ifdef MSWIN_GRAPHICS
    { &mswin_procs, 0 },
#endif
#ifdef ANDROID_GRAPHICS
    { &and_procs, 0 },
#endif
    { 0, 0 }		/* must be last */
};

STATIC_OVL
void
def_raw_print(s)
const char *s;
{
    puts(s);
}

static int windows_lock = FALSE;

int
lock_windows(flag)
int flag;
{
    int retval = windows_lock;
    windows_lock = flag;
    return retval;
}

void
choose_windows(s)
const char *s;
{
    register int i;

    if (windows_lock)
	return;

    for(i=0; winchoices[i].procs; i++)
	if (!strcmpi(s, winchoices[i].procs->name)) {
	    windowprocs = *winchoices[i].procs;
	    if (winchoices[i].ini_routine) (*winchoices[i].ini_routine)();
	    return;
	}

    if (!windowprocs.win_raw_print)
	windowprocs.win_raw_print = def_raw_print;

    raw_printf("Window type %s not recognized.  Choices are:", s);
    for(i=0; winchoices[i].procs; i++)
	raw_printf("        %s", winchoices[i].procs->name);

    if (windowprocs.win_raw_print == def_raw_print)
	terminate(EXIT_SUCCESS);
    wait_synch();
}

/*
 * tty_message_menu() provides a means to get feedback from the
 * --More-- prompt; other interfaces generally don't need that.
 */
/*ARGSUSED*/
char
genl_message_menu(let, how, mesg)
char let;
int how;
const char *mesg;
{
#if defined(MAC_MPW)
# pragma unused ( how,let )
#endif
    pline("%s", mesg);
    return 0;
}

/*
 * Called when a saved game is restored, once for each message of the saved
 * message history (oldest first), then with a null pointer.  Window ports
 * that keep their own message history for ^P are given the messages.
 */
void
putmsghistory(msg)
const char *msg;
{
#ifdef TTY_GRAPHICS
    if (!strncmpi(windowprocs.name, "tty", 3)) {
	tty_putmsghistory(msg);
	return;
    }
#endif
#ifdef ANDROID
    if (!strcmp(windowprocs.name, "and")) {
	and_putmsghistory(msg);
	return;
    }
#endif
}

/*ARGSUSED*/
void
genl_preference_update(pref)
const char *pref;
{
	/* window ports are expected to provide
	   their own preference update routine
	   for the preference capabilities that
	   they support.
	   Just return in this genl one. */
}

#ifdef DUMPLOG
/*
 * End-of-game dump log.
 *
 * done() opens the file with dump_open_log().  For each part of the dump it
 * calls dump_redirect(TRUE), which swaps in window procedures that write
 * whatever is put into any window or menu to the file instead, so the code
 * that shows the end-of-game disclosure on the screen (inventory,
 * attributes, conduct, ...) also writes the dump; putstr(0, 0, text) writes
 * a single line.  dump_redirect(FALSE) puts the real window port back.
 */

static FILE *dumplog_file = (FILE *) 0;
static char dumplog_name[BUFSZ];	/* file name, for the player */
static struct window_procs dumplog_windowprocs_backup;

/* open the dump log for a game that ended at time 'now' */
void
dump_open_log(now)
time_t now;
{
	char basename[BUFSZ], *p;
	const char *n;

	dump_close_log();
	dumplog_name[0] = '\0';

	/* slashem-<name>-<YYYYMMDDhhmmss>.txt, without any characters in the
	   name that might upset a file system */
	Strcpy(basename, "slashem-");
	p = eos(basename);
	for (n = plname; *n && n < plname + PL_NSIZ; n++)
	    *p++ = ((*n >= 'a' && *n <= 'z') || (*n >= 'A' && *n <= 'Z') ||
		    (*n >= '0' && *n <= '9') || *n == '-') ? *n : '_';
	*p = '\0';
	if (!*plname) Strcpy(p, "unknown");
	Sprintf(eos(basename), "-%08ld%06ld.txt", yyyymmdd(now), hhmmss(now));

#ifdef ANDROID
	/* Documents/nethack, or the game directory if that fails */
	dumplog_file = and_open_dumplog(basename, dumplog_name);
#else
	if (strlen(DUMPLOG_DIR) + strlen(basename) < BUFSZ) {
	    Sprintf(dumplog_name, "%s%s", DUMPLOG_DIR, basename);
	    dumplog_file = fopen(dumplog_name, "w");
	}
#endif
	if (!dumplog_file) dumplog_name[0] = '\0';
}

void
dump_close_log()
{
	dump_redirect(FALSE);
	if (dumplog_file) {
	    (void) fclose(dumplog_file);
	    dumplog_file = (FILE *) 0;
	}
}

/* the name of the dump log written for this game, or null if none */
const char *
dumplog_filename()
{
	return dumplog_name[0] ? (const char *) dumplog_name : (const char *) 0;
}

/* send output to the dump log (TRUE) or back to the screen (FALSE);
   iflags.in_dumplog tells whether it's going to the dump log */
void
dump_redirect(onoff_flag)
boolean onoff_flag;
{
	if (onoff_flag) {
	    if (!dumplog_file || iflags.in_dumplog) return;
	    dumplog_windowprocs_backup = windowprocs;
	    windowprocs.win_create_nhwindow = dump_create_nhwindow;
	    windowprocs.win_clear_nhwindow = dump_clear_nhwindow;
	    windowprocs.win_display_nhwindow = dump_display_nhwindow;
	    windowprocs.win_destroy_nhwindow = dump_destroy_nhwindow;
	    windowprocs.win_curs = dump_curs;
	    windowprocs.win_putstr = dump_putstr;
	    windowprocs.win_start_menu = dump_start_menu;
	    windowprocs.win_add_menu = dump_add_menu;
	    windowprocs.win_end_menu = dump_end_menu;
	    windowprocs.win_select_menu = dump_select_menu;
	    iflags.in_dumplog = TRUE;
	} else if (iflags.in_dumplog) {
	    windowprocs = dumplog_windowprocs_backup;
	    iflags.in_dumplog = FALSE;
	}
}

/* putstr() to the real window (unless no_forward) and the dump log; for
   use while output is not redirected */
void
dump_forward_putstr(win, attr, str, no_forward)
winid win;
int attr;
const char *str;
boolean no_forward;
{
	if (dumplog_file && !iflags.in_dumplog)
	    (void) fprintf(dumplog_file, "%s\n", str);
	if (!no_forward)
	    putstr(win, attr, str);
}

/*ARGSUSED*/
STATIC_OVL winid
dump_create_nhwindow(type)
int type;
{
	return (winid) type;	/* anything but WIN_ERR */
}

/*ARGSUSED*/
STATIC_OVL void
dump_clear_nhwindow(win)
winid win;
{
	return;
}

/*ARGSUSED*/
STATIC_OVL void
dump_display_nhwindow(win, blocking)
winid win;
boolean blocking;
{
	return;
}

/*ARGSUSED*/
STATIC_OVL void
dump_destroy_nhwindow(win)
winid win;
{
	return;
}

/*ARGSUSED*/
STATIC_OVL void
dump_curs(win, x, y)
winid win;
int x, y;
{
	return;
}

/*ARGSUSED*/
STATIC_OVL void
dump_putstr(win, attr, str)
winid win;
int attr;
const char *str;
{
	if (dumplog_file)
	    (void) fprintf(dumplog_file, "%s\n", str);
}

/*ARGSUSED*/
STATIC_OVL void
dump_start_menu(win)
winid win;
{
	return;
}

/*ARGSUSED*/
STATIC_OVL void
dump_add_menu(win, glyph, identifier, ch, gch, attr, str, preselected)
winid win;
int glyph;
const anything *identifier;
char ch, gch;
int attr;
const char *str;
boolean preselected;
{
	if (dumplog_file) {
	    if (glyph == NO_GLYPH)
		(void) fprintf(dumplog_file, " %s\n", str);
	    else
		(void) fprintf(dumplog_file, "  %c - %s\n", ch ? ch : ' ', str);
	}
}

/*ARGSUSED*/
STATIC_OVL void
dump_end_menu(win, prompt)
winid win;
const char *prompt;
{
	if (dumplog_file) {
	    if (prompt && *prompt)
		(void) fprintf(dumplog_file, "%s\n", prompt);
	    else
		(void) fputs("\n", dumplog_file);
	}
}

/*ARGSUSED*/
STATIC_OVL int
dump_select_menu(win, how, menu_list)
winid win;
int how;
menu_item **menu_list;
{
	*menu_list = (menu_item *) 0;
	return 0;
}

/* write the map, as the hero last saw it, to the dump log */
void
dump_map()
{
	int x, y, i, glyph, ch, color, lastnonblank, skippedrows;
	unsigned special;
	char buf[COLNO + 1];
	boolean toprow = TRUE;
	uchar save_showsyms[MAXPCHARS], save_oc_syms[MAXOCLASSES],
	      save_monsyms[MAXMCLASSES];

	if (!iflags.in_dumplog) return;

	/* use the plain symbols rather than DEC or IBM graphics */
	for (i = 0; i < MAXPCHARS; i++)
	    save_showsyms[i] = showsyms[i], showsyms[i] = defsyms[i].sym;
	for (i = 0; i < MAXOCLASSES; i++)
	    save_oc_syms[i] = oc_syms[i], oc_syms[i] = def_oc_syms[i];
	for (i = 0; i < MAXMCLASSES; i++)
	    save_monsyms[i] = monsyms[i], monsyms[i] = def_monsyms[i];

	/* leave out blank rows at the top, and all but one at the bottom */
	skippedrows = 0;
	for (y = 0; y < ROWNO; y++) {
	    lastnonblank = -1;
	    for (x = 1; x < COLNO; x++) {
		if (!u.uswallow)
		    glyph = glyph_at(x, y);
		else if (x == u.ux && y == u.uy)	/* show the engulfer */
		    glyph = mon_to_glyph(u.ustuck);
		else				/* and what's around it */
		    glyph = memory_glyph(x, y);
		mapglyph(glyph, &ch, &color, &special, x, y);
		buf[x - 1] = (char) ch;
		if (ch != ' ') lastnonblank = x - 1;
	    }
	    if (lastnonblank >= 0) {
		buf[lastnonblank + 1] = '\0';
		if (toprow) skippedrows = 0, toprow = FALSE;
		for (; skippedrows > 0; skippedrows--)
		    putstr(0, 0, "");
		putstr(0, 0, buf);
	    } else
		skippedrows++;
	}
	if (skippedrows) putstr(0, 0, "");

	for (i = 0; i < MAXPCHARS; i++) showsyms[i] = save_showsyms[i];
	for (i = 0; i < MAXOCLASSES; i++) oc_syms[i] = save_oc_syms[i];
	for (i = 0; i < MAXMCLASSES; i++) monsyms[i] = save_monsyms[i];
}

static int dump_status_row;
static char dump_status_buf[2][BUFSZ];

/*ARGSUSED*/
STATIC_OVL void
dump_status_curs(win, x, y)
winid win;
int x, y;
{
	dump_status_row = y;
}

/*ARGSUSED*/
STATIC_OVL void
dump_status_putstr(win, attr, str)
winid win;
int attr;
const char *str;
{
	if (dump_status_row == 0 || dump_status_row == 1) {
	    (void) strncpy(dump_status_buf[dump_status_row], str, BUFSZ - 1);
	    dump_status_buf[dump_status_row][BUFSZ - 1] = '\0';
	}
}

/* write the two status lines to the dump log */
void
dump_status()
{
	void FDECL((*save_curs), (winid,int,int));
	void FDECL((*save_putstr), (winid,int,const char *));
	boolean save_botl = flags.botl, save_botlx = flags.botlx;
#if defined(STATUS_COLORS) && defined(TEXTCOLOR)
	boolean save_colors = iflags.use_status_colors;
#endif

	if (!iflags.in_dumplog) return;

	/* let bot() format the lines but catch them instead of showing
	   them; it may put out a line in several pieces, but the last piece
	   put out for a line always holds all of it */
	save_curs = windowprocs.win_curs;
	save_putstr = windowprocs.win_putstr;
	windowprocs.win_curs = dump_status_curs;
	windowprocs.win_putstr = dump_status_putstr;
	dump_status_row = 0;
	dump_status_buf[0][0] = dump_status_buf[1][0] = '\0';
#if defined(STATUS_COLORS) && defined(TEXTCOLOR)
	iflags.use_status_colors = FALSE;
#endif
	bot();
#if defined(STATUS_COLORS) && defined(TEXTCOLOR)
	iflags.use_status_colors = save_colors;
#endif
	windowprocs.win_curs = save_curs;
	windowprocs.win_putstr = save_putstr;
	flags.botl = save_botl, flags.botlx = save_botlx;

	if (*dump_status_buf[0]) putstr(0, 0, dump_status_buf[0]);
	if (*dump_status_buf[1]) putstr(0, 0, dump_status_buf[1]);
}

/* write the latest messages to the dump log */
void
dump_msghistory()
{
	char buf[BUFSZ];
	const char *msg;
	int i;

	putstr(0, 0, "Latest messages:");
	buf[0] = ' ';			/* indent them by one */
	for (i = 0; i < MSGHISTORY_COUNT; i++)
	    if ((msg = msghist_get(i)) != 0) {
		(void) strncpy(&buf[1], msg, BUFSZ - 2);
		buf[BUFSZ - 1] = '\0';
		putstr(0, 0, buf);
	    }
}
#endif /* DUMPLOG */

/*windows.c*/
