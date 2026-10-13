/*	SCCS Id: @(#)pline.c	3.4	1999/11/28	*/
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/* NetHack may be freely redistributed.  See license for details. */

#define NEED_VARARGS /* Uses ... */	/* comment line for pre-compiled headers */
#include "hack.h"
#include "lev.h"	/* save_gamelog() */
#include "epri.h"
#ifdef WIZARD
#include "edog.h"
#endif

#ifdef OVLB

static boolean no_repeat = FALSE;

static char *FDECL(You_buf, (int));

/*VARARGS1*/
/* Note that these declarations rely on knowledge of the internals
 * of the variable argument handling stuff in "tradstdc.h"
 */

#if defined(USE_STDARG) || defined(USE_VARARGS)
static void FDECL(vpline, (const char *, va_list));

void
pline VA_DECL(const char *, line)
	VA_START(line);
	VA_INIT(line, char *);
	vpline(line, VA_ARGS);
	VA_END();
}

# ifdef USE_STDARG
static void
vpline(const char *line, va_list the_args) {
# else
static void
vpline(line, the_args) const char *line; va_list the_args; {
# endif

#else	/* USE_STDARG | USE_VARARG */

#define vpline pline

void
pline VA_DECL(const char *, line)
#endif	/* USE_STDARG | USE_VARARG */

	char pbuf[BUFSZ];
/* Do NOT use VA_START and VA_END in here... see above */

	if (!line || !*line) return;
	if (index(line, '%')) {
	    Vsprintf(pbuf,line,VA_ARGS);
	    line = pbuf;
	}
	if (!iflags.window_inited) {
	    raw_print(line);
	    return;
	}
#ifndef MAC
	if (no_repeat && !strcmp(line, toplines))
	    return;
#endif /* MAC */
	if (vision_full_recalc) vision_recalc(0);
	if (u.ux) flush_screen(1);		/* %% */
	putstr(WIN_MESSAGE, 0, line);
}

/*VARARGS1*/
void
Norep VA_DECL(const char *, line)
	VA_START(line);
	VA_INIT(line, const char *);
	no_repeat = TRUE;
	vpline(line, VA_ARGS);
	no_repeat = FALSE;
	VA_END();
	return;
}

/* work buffer for You(), &c and verbalize() */
static char *you_buf = 0;
static int you_buf_siz = 0;

static char *
You_buf(siz)
int siz;
{
	if (siz > you_buf_siz) {
		if (you_buf) free((genericptr_t) you_buf);
		you_buf_siz = siz + 10;
		you_buf = (char *) alloc((unsigned) you_buf_siz);
	}
	return you_buf;
}

void
free_youbuf()
{
	if (you_buf) free((genericptr_t) you_buf),  you_buf = (char *)0;
	you_buf_siz = 0;
}

/* `prefix' must be a string literal, not a pointer */
#define YouPrefix(pointer,prefix,text) \
 Strcpy((pointer = You_buf((int)(strlen(text) + sizeof prefix))), prefix)

#define YouMessage(pointer,prefix,text) \
 strcat((YouPrefix(pointer, prefix, text), pointer), text)

/*VARARGS1*/
void
You VA_DECL(const char *, line)
	char *tmp;
	VA_START(line);
	VA_INIT(line, const char *);
	vpline(YouMessage(tmp, "You ", line), VA_ARGS);
	VA_END();
}

/*VARARGS1*/
void
Your VA_DECL(const char *,line)
	char *tmp;
	VA_START(line);
	VA_INIT(line, const char *);
	vpline(YouMessage(tmp, "Your ", line), VA_ARGS);
	VA_END();
}

/*VARARGS1*/
void
You_feel VA_DECL(const char *,line)
	char *tmp;
	VA_START(line);
	VA_INIT(line, const char *);
	vpline(YouMessage(tmp, "You feel ", line), VA_ARGS);
	VA_END();
}


/*VARARGS1*/
void
You_cant VA_DECL(const char *,line)
	char *tmp;
	VA_START(line);
	VA_INIT(line, const char *);
	vpline(YouMessage(tmp, "You can't ", line), VA_ARGS);
	VA_END();
}

/*VARARGS1*/
void
pline_The VA_DECL(const char *,line)
	char *tmp;
	VA_START(line);
	VA_INIT(line, const char *);
	vpline(YouMessage(tmp, "The ", line), VA_ARGS);
	VA_END();
}

/*VARARGS1*/
void
There VA_DECL(const char *,line)
	char *tmp;
	VA_START(line);
	VA_INIT(line, const char *);
	vpline(YouMessage(tmp, "There ", line), VA_ARGS);
	VA_END();
}

/*VARARGS1*/
void
You_hear VA_DECL(const char *,line)
	char *tmp;
	VA_START(line);
	VA_INIT(line, const char *);
	if (Underwater)
		YouPrefix(tmp, "You barely hear ", line);
	else if (u.usleep)
		YouPrefix(tmp, "You dream that you hear ", line);
	else
		YouPrefix(tmp, "You hear ", line);
	vpline(strcat(tmp, line), VA_ARGS);
	VA_END();
}

/*VARARGS1*/
void
verbalize VA_DECL(const char *,line)
	char *tmp;
	if (!flags.soundok) return;
	VA_START(line);
	VA_INIT(line, const char *);
	tmp = You_buf((int)strlen(line) + sizeof "\"\"");
	Strcpy(tmp, "\"");
	Strcat(tmp, line);
	Strcat(tmp, "\"");
	vpline(tmp, VA_ARGS);
	VA_END();
}

/*VARARGS1*/
/* Note that these declarations rely on knowledge of the internals
 * of the variable argument handling stuff in "tradstdc.h"
 */

#if defined(USE_STDARG) || defined(USE_VARARGS)
static void FDECL(vraw_printf,(const char *,va_list));

void
raw_printf VA_DECL(const char *, line)
	VA_START(line);
	VA_INIT(line, char *);
	vraw_printf(line, VA_ARGS);
	VA_END();
}

# ifdef USE_STDARG
static void
vraw_printf(const char *line, va_list the_args) {
# else
static void
vraw_printf(line, the_args) const char *line; va_list the_args; {
# endif

#else  /* USE_STDARG | USE_VARARG */

void
raw_printf VA_DECL(const char *, line)
#endif
/* Do NOT use VA_START and VA_END in here... see above */

	if(!index(line, '%'))
	    raw_print(line);
	else {
	    char pbuf[BUFSZ];
	    Vsprintf(pbuf,line,VA_ARGS);
	    raw_print(pbuf);
	}
}


/*VARARGS1*/
void
impossible VA_DECL(const char *, s)
	VA_START(s);
	VA_INIT(s, const char *);
	if (program_state.in_impossible)
		panic("impossible called impossible");
	program_state.in_impossible = 1;
	{
	    char pbuf[BUFSZ];
	    Vsprintf(pbuf,s,VA_ARGS);
	    paniclog("impossible", pbuf);
	}
	vpline(s,VA_ARGS);
	pline("Program in disorder - perhaps you'd better #quit.");
	program_state.in_impossible = 0;
	VA_END();
}

const char *
align_str(alignment)
    aligntyp alignment;
{
    switch ((int)alignment) {
	case A_CHAOTIC: return "chaotic";
	case A_NEUTRAL: return "neutral";
	case A_LAWFUL:	return "lawful";
	case A_NONE:	return "unaligned";
    }
    return "unknown";
}

void
mstatusline(mtmp)
register struct monst *mtmp;
{
	aligntyp alignment;
	char info[BUFSZ], monnambuf[BUFSZ];

	if (mtmp->ispriest || mtmp->data == &mons[PM_ALIGNED_PRIEST]
				|| mtmp->data == &mons[PM_ANGEL])
		alignment = EPRI(mtmp)->shralign;
	else
		alignment = mtmp->data->maligntyp;
	alignment = (alignment > 0) ? A_LAWFUL :
		(alignment < 0) ? A_CHAOTIC :
		A_NEUTRAL;

	info[0] = 0;
	if (mtmp->mtame) {	  Strcat(info, ", tame");
#ifdef WIZARD
	    if (wizard) {
		Sprintf(eos(info), " (%d", mtmp->mtame);
		if (!mtmp->isminion)
		    Sprintf(eos(info), "; hungry %ld; apport %d",
			EDOG(mtmp)->hungrytime, EDOG(mtmp)->apport);
		Strcat(info, ")");
	    }
#endif
	}
	else if (mtmp->mpeaceful) Strcat(info, ", peaceful");
	else if (mtmp->mtraitor)  Strcat(info, ", traitor");
	if (mtmp->meating)	  Strcat(info, ", eating");
	if (mtmp->mcan)		  Strcat(info, ", cancelled");
	if (mtmp->mconf)	  Strcat(info, ", confused");
	if (mtmp->mblinded || !mtmp->mcansee)
				  Strcat(info, ", blind");
	if (mtmp->mstun)	  Strcat(info, ", stunned");
	if (mtmp->msleeping)	  Strcat(info, ", asleep");
#if 0	/* unfortunately mfrozen covers temporary sleep and being busy
	   (donning armor, for instance) as well as paralysis */
	else if (mtmp->mfrozen)	  Strcat(info, ", paralyzed");
#else
	else if (mtmp->mfrozen || !mtmp->mcanmove)
				  Strcat(info, ", can't move");
#endif
				  /* [arbitrary reason why it isn't moving] */
	else if (mtmp->mstrategy & STRAT_WAITMASK)
				  Strcat(info, ", meditating");
	else if (mtmp->mflee) {	  Strcat(info, ", scared");
#ifdef WIZARD
	    if (wizard)		  Sprintf(eos(info), " (%d)", mtmp->mfleetim);
#endif
	}
	if (mtmp->mtrapped)	  Strcat(info, ", trapped");
	if (mtmp->mspeed)	  Strcat(info,
					mtmp->mspeed == MFAST ? ", fast" :
					mtmp->mspeed == MSLOW ? ", slow" :
					", ???? speed");
	if (mtmp->mundetected)	  Strcat(info, ", concealed");
	if (mtmp->minvis)	  Strcat(info, ", invisible");
	if (mtmp == u.ustuck)	  Strcat(info,
			(sticks(youmonst.data)) ? ", held by you" :
				u.uswallow ? (is_animal(u.ustuck->data) ?
				", swallowed you" :
				", engulfed you") :
				", holding you");
#ifdef STEED
	if (mtmp == u.usteed)	  Strcat(info, ", carrying you");
#endif

	/* avoid "Status of the invisible newt ..., invisible" */
	/* and unlike a normal mon_nam, use "saddled" even if it has a name */
	Strcpy(monnambuf, x_monnam(mtmp, ARTICLE_THE, (char *)0,
	    (SUPPRESS_IT|SUPPRESS_INVISIBLE), FALSE));

	pline("Status of %s (%s):  Level %d  HP %d(%d)  Pw %d(%d)  AC %d%s.",
		monnambuf,
		align_str(alignment),
		mtmp->m_lev,
		mtmp->mhp,
		mtmp->mhpmax,
		mtmp->m_en,
		mtmp->m_enmax,
		find_mac(mtmp),
		info);
}

void
ustatusline()
{
	char info[BUFSZ];

	info[0] = '\0';
	if (Sick) {
		Strcat(info, ", dying from");
		if (u.usick_type & SICK_VOMITABLE)
			Strcat(info, " food poisoning");
		if (u.usick_type & SICK_NONVOMITABLE) {
			if (u.usick_type & SICK_VOMITABLE)
				Strcat(info, " and");
			Strcat(info, " illness");
		}
	}
	if (Stoned)		Strcat(info, ", solidifying");
	if (Slimed)		Strcat(info, ", becoming slimy");
	if (Strangled)		Strcat(info, ", being strangled");
	if (Vomiting)		Strcat(info, ", nauseated"); /* !"nauseous" */
	if (Confusion)		Strcat(info, ", confused");
	if (Blind) {
	    Strcat(info, ", blind");
	    if (u.ucreamed) {
		if ((long)u.ucreamed < Blinded || Blindfolded
						|| !haseyes(youmonst.data))
		    Strcat(info, ", cover");
		Strcat(info, "ed by sticky goop");
	    }	/* note: "goop" == "glop"; variation is intentional */
	}
	if (Stunned)		Strcat(info, ", stunned");
#ifdef STEED
	if (!u.usteed)
#endif
	if (Wounded_legs) {
	    const char *what = body_part(LEG);
	    if ((Wounded_legs & BOTH_SIDES) == BOTH_SIDES)
		what = makeplural(what);
				Sprintf(eos(info), ", injured %s", what);
	}
	if (Glib)		Sprintf(eos(info), ", slippery %s",
					makeplural(body_part(HAND)));
	if (u.utrap)		Strcat(info, ", trapped");
	if (Fast)		Strcat(info, Very_fast ?
						", very fast" : ", fast");
	if (u.uundetected)	Strcat(info, ", concealed");
	if (Invis)		Strcat(info, ", invisible");
	if (u.ustuck) {
	    if (sticks(youmonst.data))
		Strcat(info, ", holding ");
	    else
		Strcat(info, ", held by ");
	    Strcat(info, mon_nam(u.ustuck));
	}

	pline("Status of %s (%s%s):  Level %d  HP %d(%d)  Pw %d(%d)  AC %d%s.",
		plname,
		    (u.ualign.record >= 20) ? "piously " :
		    (u.ualign.record > 13) ? "devoutly " :
		    (u.ualign.record > 8) ? "fervently " :
		    (u.ualign.record > 3) ? "stridently " :
		    (u.ualign.record == 3) ? "" :
		    (u.ualign.record >= 1) ? "haltingly " :
		    (u.ualign.record == 0) ? "nominally " :
					    "insufficiently ",
		align_str(u.ualign.type),
		Upolyd ? mons[u.umonnum].mlevel : u.ulevel,
		Upolyd ? u.mh : u.uhp,
		Upolyd ? u.mhmax : u.uhpmax,
		u.uen,
		u.uenmax,
		u.uac,
		info);
}

void
self_invis_message()
{
	pline("%s %s.",
	    Hallucination ? "Far out, man!  You" : "Gee!  All of a sudden, you",
	    See_invisible ? "can see right through yourself" :
		"can't see yourself");
}

/*
 * #chronicle: a log of the major events of the game, adapted from
 * NetHack 5.0's gamelog (which also feeds its "livelog").  The log is
 * saved with the game; show_gamelog() lists it in-game and at the end of
 * the game (for the dump log).
 */
struct gamelog_line {
	struct gamelog_line *next;
	long turn;		/* when the event happened */
	long flags;		/* LL_xxx kind of event */
	char *text;
};

static struct gamelog_line *gamelog = 0, *gamelog_last = 0;
static long gamelog_done = 0L;	/* GLF_xxx: one-time events already logged */

/* add an event to the log */
void
gamelog_add(glflags, gltime, str)
long glflags, gltime;
const char *str;
{
	struct gamelog_line *tmp;

	tmp = (struct gamelog_line *) alloc(sizeof (struct gamelog_line));
	tmp->next = (struct gamelog_line *) 0;
	tmp->turn = gltime;
	tmp->flags = glflags;
	tmp->text = strcpy((char *) alloc(strlen(str) + 1), str);
	if (gamelog_last)
	    gamelog_last->next = tmp;
	else
	    gamelog = tmp;
	gamelog_last = tmp;
}

/*VARARGS2*/
/* log an event that happens now; callers keep the arguments short */
void
livelog_printf VA_DECL2(long, ll_type, const char *, line)
	char buf[BUFSZ * 2];
	VA_START(line);
	VA_INIT(line, const char *);
	Vsprintf(buf, line, VA_ARGS);
	VA_END();
	gamelog_add(ll_type, moves, buf);
}

/* for events that are only logged the first time they happen: returns
   TRUE (and remembers it) if 'what' (GLF_xxx) hasn't been logged yet */
boolean
gamelog_first(what)
long what;
{
	if (gamelog_done & what)
	    return FALSE;
	gamelog_done |= what;
	return TRUE;
}

/* log the first violation of each conduct; called every turn, so that
   the many places which break conducts don't all need to do it (the
   ones that can say more, like the first wish, log it themselves) */
void
gamelog_conduct()
{
	if (u.uconduct.food && gamelog_first(GLF_FOOD))
	    livelog_printf(LL_CONDUCT, "ate for the first time");
	if (u.uconduct.unvegan && gamelog_first(GLF_UNVEGAN))
	    livelog_printf(LL_CONDUCT,
			   "consumed animal products for the first time");
	if (u.uconduct.unvegetarian && gamelog_first(GLF_UNVEGETARIAN))
	    livelog_printf(LL_CONDUCT, "tasted meat for the first time");
	if (u.uconduct.gnostic && gamelog_first(GLF_GNOSTIC))
	    livelog_printf(LL_CONDUCT, "rejected atheism");
	if (u.uconduct.weaphit && gamelog_first(GLF_WEAPHIT))
	    livelog_printf(LL_CONDUCT,
			   "hit with a wielded weapon for the first time");
	if (u.uconduct.killer && gamelog_first(GLF_KILLER))
	    livelog_printf(LL_CONDUCT, "killed for the first time");
	if (u.uconduct.literate && gamelog_first(GLF_LITERATE))
	    livelog_printf(LL_CONDUCT, "became literate");
	if (u.uconduct.polypiles && gamelog_first(GLF_POLYPILE))
	    livelog_printf(LL_CONDUCT, "polymorphed an object for the first time");
	if (u.uconduct.polyselfs && gamelog_first(GLF_POLYSELF))
	    livelog_printf(LL_CONDUCT, "changed form for the first time");
	if (u.uconduct.wishes && gamelog_first(GLF_WISH))
	    livelog_printf(LL_CONDUCT, "made a wish for the first time");
	if (u.uconduct.celibacy && gamelog_first(GLF_CELIBACY))
	    livelog_printf(LL_CONDUCT, "gave up celibacy");
}

/* #chronicle command */
int
do_gamelog()
{
	gamelog_conduct();	/* in case one was broken this turn */
	if (gamelog)
	    show_gamelog(0);
	else
	    pline("No chronicled events.");
	return 0;
}

/* events shown at the end of the game (and in the dump log) */
#define LL_majors (LL_WISH | LL_ACHIEVE | LL_UMONST | LL_DIVINEGIFT \
		   | LL_LIFESAVE | LL_ARTIFACT | LL_GENOCIDE | LL_DUMP)

/* list the chronicle: final is 0 for #chronicle, or 1 (hero alive) or
   2 (hero dead) at the end of the game, when only the major events are
   listed; the dump log can call this */
void
show_gamelog(final)
int final;
{
	struct gamelog_line *llmsg;
	winid win;
	char buf[BUFSZ];
	int eventcnt = 0;

	win = create_nhwindow(NHW_TEXT);
	Sprintf(buf, "%s events:", final ? "Major" : "Logged");
	putstr(win, 0, buf);
	for (llmsg = gamelog; llmsg; llmsg = llmsg->next) {
	    if (final && !(llmsg->flags & LL_majors))
		continue;
	    /* don't give away what the hero hasn't found out yet */
	    if (!final && (llmsg->flags & LL_SPOILER) != 0
#ifdef WIZARD
		&& !wizard
#endif
		)
		continue;
	    if (!eventcnt++)
		putstr(win, 0, " Turn");
	    Sprintf(buf, "%5ld: ", llmsg->turn);
	    (void) strncat(buf, llmsg->text, BUFSZ - 1 - strlen(buf));
	    putstr(win, 0, buf);
	}
	if (!eventcnt)
	    putstr(win, 0, " none");
	display_nhwindow(win, TRUE);
	destroy_nhwindow(win);
}

#undef LL_majors

void
save_gamelog(fd, mode)
int fd, mode;
{
	struct gamelog_line *llmsg;
	int count, len;

	if (perform_bwrite(mode)) {
	    for (count = 0, llmsg = gamelog; llmsg; llmsg = llmsg->next)
		count++;
	    bwrite(fd, (genericptr_t) &count, sizeof count);
	    bwrite(fd, (genericptr_t) &gamelog_done, sizeof gamelog_done);
	    for (llmsg = gamelog; llmsg; llmsg = llmsg->next) {
		bwrite(fd, (genericptr_t) &llmsg->turn, sizeof llmsg->turn);
		bwrite(fd, (genericptr_t) &llmsg->flags, sizeof llmsg->flags);
		len = (int) strlen(llmsg->text);
		bwrite(fd, (genericptr_t) &len, sizeof len);
		bwrite(fd, (genericptr_t) llmsg->text, (unsigned) len);
	    }
	}
	if (release_data(mode))
	    free_gamelog();
}

/* forget the whole chronicle */
void
free_gamelog()
{
	struct gamelog_line *llmsg, *next;

	for (llmsg = gamelog; llmsg; llmsg = next) {
	    next = llmsg->next;
	    free((genericptr_t) llmsg->text);
	    free((genericptr_t) llmsg);
	}
	gamelog = gamelog_last = (struct gamelog_line *) 0;
	gamelog_done = 0L;
}

void
restore_gamelog(fd)
int fd;
{
	int count, len;
	long turn, glflags;
	char buf[BUFSZ * 2];

	free_gamelog();		/* start from an empty log */
	mread(fd, (genericptr_t) &count, sizeof count);
	mread(fd, (genericptr_t) &gamelog_done, sizeof gamelog_done);
	while (count-- > 0) {
	    mread(fd, (genericptr_t) &turn, sizeof turn);
	    mread(fd, (genericptr_t) &glflags, sizeof glflags);
	    mread(fd, (genericptr_t) &len, sizeof len);
	    if (len < 0 || len >= (int) sizeof buf)
		panic("restore_gamelog: bad length (%d)", len);
	    mread(fd, (genericptr_t) buf, (unsigned) len);
	    buf[len] = '\0';
	    gamelog_add(glflags, turn, buf);
	}
}

#endif /* OVLB */
/*pline.c*/
