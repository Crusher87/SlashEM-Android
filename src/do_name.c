/*	SCCS Id: @(#)do_name.c	3.4	2003/01/14	*/
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"

#ifdef OVLB

STATIC_DCL void FDECL(getpos_help, (BOOLEAN_P,const char *));
STATIC_PTR int FDECL(CFDECLSPEC cmp_coord_distu, (const genericptr,
						  const genericptr));
STATIC_DCL boolean FDECL(gather_locs_interesting, (int,int,int));
STATIC_DCL void FDECL(gather_locs, (coord **,int *,int));
STATIC_DCL boolean FDECL(auto_describe, (int,int,BOOLEAN_P));
STATIC_DCL void FDECL(getpos_hilite, (BOOLEAN_P));

#ifdef ANDROID
void FDECL(and_putstr_ex, (winid,int,const char *,int));
#endif

extern const char what_is_an_unknown_object[];		/* from pager.c */
extern const char travel_destination[];			/* from cmd.c */

/* Callback for getpos() telling which map spots are valid targets;
 * they can be highlighted with '$' and visited with 'z' and 'Z'
 * (from NetHack 3.6).  Callers set it just before calling getpos(),
 * which resets it when done.
 */
static boolean FDECL((*getpos_getvalid), (int,int)) =
					(boolean FDECL((*), (int,int))) 0;

void
getpos_sethilite(gp_getvalidf)
boolean FDECL((*gp_getvalidf), (int,int));
{
    getpos_getvalid = gp_getvalidf;
}

/* kinds of spots that getpos() can cycle through */
#define GLOC_MONS	0
#define GLOC_OBJS	1
#define GLOC_DOOR	2
#define GLOC_EXPLORE	3
#define GLOC_INTERESTING 4
#define GLOC_VALID	5
#define NUM_GLOCS	6

/* the keys for those: next and previous */
static const char gloc_keys[] = "mMoOdDxXaAzZ";

static const char *const gloc_descr[NUM_GLOCS] = {
    "monster", "object", "door or doorway", "unexplored location",
    "interesting thing", "valid location"
};

/* the response for '?' help request in getpos() */
STATIC_OVL void
getpos_help(force, goal)
boolean force;
const char *goal;
{
    char sbuf[BUFSZ];
    boolean doing_what_is;
    int i;
    winid tmpwin = create_nhwindow(NHW_MENU);

    Sprintf(sbuf, "Use [%s] to move the cursor to %s.",
	    iflags.num_pad ? "2468" : "hjkl", goal);
    putstr(tmpwin, 0, sbuf);
    putstr(tmpwin, 0, "Use [HJKL] to move the cursor 8 units at a time.");
    putstr(tmpwin, 0, "Or enter a background symbol (ex. <).");
    putstr(tmpwin, 0, "Use @ to move the cursor on yourself.");
    for (i = 0; i < NUM_GLOCS; i++) {
	if (i == GLOC_VALID && !getpos_getvalid) continue;
	Sprintf(sbuf, "Use %c or %c to move the cursor to the next or previous %s.",
		gloc_keys[2 * i], gloc_keys[2 * i + 1], gloc_descr[i]);
	putstr(tmpwin, 0, sbuf);
    }
    if (getpos_getvalid)
	putstr(tmpwin, 0, "Use $ to display the valid locations.");
    putstr(tmpwin, 0, "Use # to toggle automatic description.");
    /* disgusting hack; the alternate selection characters work for any
       getpos call, but they only matter for dowhatis (and doquickwhatis) */
    doing_what_is = (goal == what_is_an_unknown_object);
    Sprintf(sbuf, "Type a .%s when you are at the right place.",
            doing_what_is ? " or , or ; or :" : "");
    putstr(tmpwin, 0, sbuf);
    if (!force)
	putstr(tmpwin, 0, "Type Space or Escape when you're done.");
    putstr(tmpwin, 0, "");
    display_nhwindow(tmpwin, TRUE);
    destroy_nhwindow(tmpwin);
}

/* qsort comparison: nearer to the hero first, then top to bottom */
STATIC_PTR int CFDECLSPEC
cmp_coord_distu(a, b)
const genericptr a;
const genericptr b;
{
    const coord *c1 = (const coord *)a;
    const coord *c2 = (const coord *)b;
    int dx, dy, dist_1, dist_2;

    dx = u.ux - c1->x;
    dy = u.uy - c1->y;
    dist_1 = max(abs(dx), abs(dy));
    dx = u.ux - c2->x;
    dy = u.uy - c2->y;
    dist_2 = max(abs(dx), abs(dy));

    if (dist_1 == dist_2)
	return (c1->y != c2->y) ? (c1->y - c2->y) : (c1->x - c2->x);
    return dist_1 - dist_2;
}

#define is_cmap_wall_or_stone(c) ((c) >= S_stone && (c) <= S_trwall)
#define is_cmap_door_or_bridge(c) ((c) == S_ndoor || \
				   ((c) >= S_vodoor && (c) <= S_hcdoor) || \
				   ((c) >= S_vodbridge && (c) <= S_hcdbridge))
#define IS_UNEXPLORED_LOC(x,y) \
    (isok((x), (y)) && memory_glyph((x), (y)) == cmap_to_glyph(S_stone) && \
     !levl[(x)][(y)].seenv)

/* is the spot <x,y> one of the kind gloc that getpos() can jump to? */
STATIC_OVL boolean
gather_locs_interesting(x, y, gloc)
int x, y, gloc;
{
    int glyph = glyph_at(x, y), c = -1;

    if (glyph_is_cmap(glyph))
	c = glyph_to_cmap(glyph);

    switch (gloc) {
    default:
    case GLOC_MONS:
	/* this skips monsters shown by warning or remembered as unseen */
	return (glyph_is_monster(glyph)
		&& glyph != monnum_to_glyph(PM_LONG_WORM_TAIL));
    case GLOC_OBJS:
	return (glyph_is_object(glyph)
		&& glyph != objnum_to_glyph(BOULDER)
		&& glyph != objnum_to_glyph(ROCK));
    case GLOC_DOOR:
	return (c >= 0 && is_cmap_door_or_bridge(c));
    case GLOC_EXPLORE:
	return (c >= 0
		&& (is_cmap_door_or_bridge(c) || c == S_room
		    || c == S_corr || c == S_litcorr)
		&& (IS_UNEXPLORED_LOC(x + 1, y) || IS_UNEXPLORED_LOC(x - 1, y)
		    || IS_UNEXPLORED_LOC(x, y + 1)
		    || IS_UNEXPLORED_LOC(x, y - 1)));
    case GLOC_VALID:
	if (getpos_getvalid)
	    return (*getpos_getvalid)(x, y);
	/*FALLTHRU*/
    case GLOC_INTERESTING:
	return (c < 0 || is_cmap_door_or_bridge(c)
		|| !(is_cmap_wall_or_stone(c) || c == S_tree || c == S_bars
		     || c == S_ice || c == S_air || c == S_cloud
		     || c == S_lava || c == S_water || c == S_pool
		     || c == S_room || c == S_corr || c == S_litcorr));
    }
    /*NOTREACHED*/
    return FALSE;
}

/* gather the map spots of kind gloc, nearest first; the hero's own spot
   is always included (as element 0) so that cycling through them all
   comes back to the hero */
STATIC_OVL void
gather_locs(arr_p, cnt_p, gloc)
coord **arr_p;
int *cnt_p;
int gloc;
{
    int x, y, pass, idx;

    *cnt_p = idx = 0;
    for (pass = 0; pass < 2; pass++) {
	for (x = 1; x < COLNO; x++)
	    for (y = 0; y < ROWNO; y++) {
		if ((x == u.ux && y == u.uy)
			|| gather_locs_interesting(x, y, gloc)) {
		    if (!pass) {
			++*cnt_p;
		    } else {
			(*arr_p)[idx].x = x;
			(*arr_p)[idx].y = y;
			++idx;
		    }
		}
	    }
	if (!pass)	/* end of first pass */
	    *arr_p = (coord *)alloc(*cnt_p * sizeof (coord));
	else		/* end of second pass */
	    qsort((genericptr_t)*arr_p, *cnt_p, sizeof (coord),
		  cmp_coord_distu);
    }
}

/* autodescribe: say what is at <cx,cy>; replace says that the last
   message shown was the previous description, which can be overwritten
   (on Android, so that the message log doesn't fill up with them) */
STATIC_OVL boolean
auto_describe(cx, cy, replace)
int cx, cy;
boolean replace;
{
    coord cc;
    char tmpbuf[BUFSZ], outbuf[BUFSZ];
    const char *firstmatch = "unknown";

    cc.x = cx;
    cc.y = cy;
    if (!do_screen_description(cc, TRUE, 0, tmpbuf, &firstmatch,
			       (struct permonst **)0))
	return FALSE;
    Sprintf(outbuf, "%.200s%s", firstmatch,
	    (getpos_getvalid && !(*getpos_getvalid)(cx, cy))
		? " (illegal)" : "");
    /* not pline(): this isn't worth keeping in any message history */
#ifdef ANDROID
    if (replace)
	and_putstr_ex(WIN_MESSAGE, 0, outbuf, -BUFSZ);
    else
#endif
	putstr(WIN_MESSAGE, 0, outbuf);
    curs(WIN_MAP, cx, cy);
    flush_screen(0);
    return TRUE;
}

/* show (on) or remove the highlighting of valid spots; this covers
   what is shown there, so autodescribe is skipped meanwhile */
STATIC_OVL void
getpos_hilite(on)
boolean on;
{
    int x, y;

    if (!on) {
	tmp_at(DISP_END, 0);
	return;
    }
    tmp_at(DISP_BEAM, cmap_to_glyph(S_flashbeam));
    for (x = 1; x < COLNO; x++)
	for (y = 0; y < ROWNO; y++)
	    if ((*getpos_getvalid)(x, y))
		tmp_at(x, y);
}

int
getpos(cc,force,goal)
coord *cc;
boolean force;
const char *goal;
{
    int result = 0;
    int cx, cy, i, c;
    int sidx, tx, ty;
    boolean msg_given = TRUE;	/* clear message window by default */
    boolean described = FALSE;	/* last message is an autodescription */
    boolean hilite_state = FALSE;
    static const char pick_chars[] = ".,;:";
    const char *cp;
    const char *sdp;
    coord *garr[NUM_GLOCS];
    int gcount[NUM_GLOCS];
    int gidx[NUM_GLOCS];

    if(iflags.num_pad) sdp = ndir; else sdp = sdir;	/* DICE workaround */
    for (i = 0; i < NUM_GLOCS; i++) {
	garr[i] = (coord *)0;
	gcount[i] = gidx[i] = 0;
    }

    if (flags.verbose) {
	pline("(For instructions type a ?)");
	msg_given = TRUE;
    }
    cx = cc->x;
    cy = cc->y;
#if defined(CLIPPING) && !defined(ANDROID)
    cliparound(cx, cy);
#endif
    curs(WIN_MAP, cx,cy);
    flush_screen(0);
#if defined(MAC) || defined(ANDROID)
    lock_mouse_cursor(TRUE);
#endif
    for (;;) {
	if (iflags.autodescribe && !msg_given && !hilite_state) {
	    if (auto_describe(cx, cy, described))
		described = TRUE;
	}

	c = nh_poskey(&tx, &ty, &sidx);

	if (hilite_state) {	/* shown until the next key */
	    getpos_hilite(FALSE);
	    hilite_state = FALSE;
	    curs(WIN_MAP, cx, cy);
	    flush_screen(0);
	}

	if (iflags.autodescribe)
	    msg_given = FALSE;

	if (c == '\033') {
	    cx = cy = -10;
	    msg_given = TRUE;	/* force clear */
	    result = -1;
	    break;
	}
	if(c == 0) {
	    if (!isok(tx, ty)) continue;
	    /* a mouse click event, just assign and return */
	    cx = tx;
	    cy = ty;
	    break;
	}
	if ((cp = index(pick_chars, c)) != 0) {
	    /* '.' => 0, ',' => 1, ';' => 2, ':' => 3 */
	    result = cp - pick_chars;
	    break;
	}
	/* "_ _" travels to the remembered travel destination again; once
	   the cursor has moved, '_' looks for an altar as usual */
	if (c == '_' && goal == travel_destination &&
		cx == cc->x && cy == cc->y &&
		cx == iflags.travelcc.x && cy == iflags.travelcc.y) {
	    result = 0;
	    break;
	}
	for (i = 0; i < 8; i++) {
	    int dx, dy;

	    if (sdp[i] == c) {
		/* a normal movement letter or digit */
		dx = xdir[i];
		dy = ydir[i];
	    } else if (sdir[i] == lowc((char)c)) {
		/* a shifted movement letter */
		dx = 8 * xdir[i];
		dy = 8 * ydir[i];
	    } else
		continue;

	    /* truncate at map edge; diagonal moves complicate this... */
	    if (cx + dx < 1) {
		dy -= sgn(dy) * (1 - (cx + dx));
		dx = 1 - cx;		/* so that (cx+dx == 1) */
	    } else if (cx + dx > COLNO-1) {
		dy += sgn(dy) * ((COLNO-1) - (cx + dx));
		dx = (COLNO-1) - cx;
	    }
	    if (cy + dy < 0) {
		dx -= sgn(dx) * (0 - (cy + dy));
		dy = 0 - cy;		/* so that (cy+dy == 0) */
	    } else if (cy + dy > ROWNO-1) {
		dx += sgn(dx) * ((ROWNO-1) - (cy + dy));
		dy = (ROWNO-1) - cy;
	    }
	    cx += dx;
	    cy += dy;
	    goto nxtc;
	}

	if(c == '?'){
	    getpos_help(force, goal);
	    pline("Move the cursor to %s:", goal);
	    msg_given = TRUE;
	    described = FALSE;
	} else if (c == '$' && getpos_getvalid) {
	    /* show the valid spots until the next key */
	    getpos_hilite(TRUE);
	    hilite_state = TRUE;
	    goto nxtc;
	} else if (c == '#') {
	    iflags.autodescribe = !iflags.autodescribe;
	    pline("Automatic description %sis %s.",
		  flags.verbose ? "of features under cursor " : "",
		  iflags.autodescribe ? "on" : "off");
	    msg_given = TRUE;
	    described = FALSE;
	    goto nxtc;
	} else if (c == '@') {
	    /* reset 'm&M', 'o&O', &c too */
	    for (i = 0; i < NUM_GLOCS; i++)
		gidx[i] = 0;
	    cx = u.ux;
	    cy = u.uy;
	    goto nxtc;
	} else if ((cp = index(gloc_keys, c)) != 0) {
	    /* nearest or farthest monster, object, door, &c */
	    int gtmp = (int)(cp - gloc_keys),	/* 0..11 */
		gloc = gtmp >> 1;		/* 0..5 */

	    if (!garr[gloc]) {
		gather_locs(&garr[gloc], &gcount[gloc], gloc);
		gidx[gloc] = 0;	/* garr[][0] is the hero's spot */
	    }
	    if (!(gtmp & 1)) {	/* 'm', 'o', 'd', 'x', 'a', 'z' */
		gidx[gloc] = (gidx[gloc] + 1) % gcount[gloc];
	    } else {		/* 'M', 'O', 'D', 'X', 'A', 'Z' */
		if (--gidx[gloc] < 0)
		    gidx[gloc] = gcount[gloc] - 1;
	    }
	    cx = garr[gloc][gidx[gloc]].x;
	    cy = garr[gloc][gidx[gloc]].y;
	    goto nxtc;
	} else {
	    if (!index(quitchars, c)) {
		char matching[MAXPCHARS];
		int pass, lo_x, lo_y, hi_x, hi_y, k = 0;
		(void)memset((genericptr_t)matching, 0, sizeof matching);
		for (sidx = 1; sidx < MAXPCHARS; sidx++)
		    if (c == defsyms[sidx].sym || c == (int)showsyms[sidx])
			matching[sidx] = (char) ++k;
		if (k) {
		    for (pass = 0; pass <= 1; pass++) {
			/* pass 0: just past current pos to lower right;
			   pass 1: upper left corner to current pos */
			lo_y = (pass == 0) ? cy : 0;
			hi_y = (pass == 0) ? ROWNO - 1 : cy;
			for (ty = lo_y; ty <= hi_y; ty++) {
			    lo_x = (pass == 0 && ty == lo_y) ? cx + 1 : 1;
			    hi_x = (pass == 1 && ty == hi_y) ? cx : COLNO - 1;
			    for (tx = lo_x; tx <= hi_x; tx++) {
				/* first, look at what is currently shown
				   (might be a monster or object) */
				k = glyph_at(tx, ty);
				if (glyph_is_cmap(k) &&
					matching[glyph_to_cmap(k)])
				    goto foundc;
				/* next, try the glyph remembered there
				   (might be a trap or object) */
				if (level.flags.hero_memory) {
				    k = memory_glyph(tx, ty);
				    if (glyph_is_cmap(k) &&
					    matching[glyph_to_cmap(k)])
					goto foundc;
#ifdef DISPLAY_LAYERS
				    /* and the trap and background remembered
				       under it */
				    if (levl[tx][ty].mem_trap &&
					    matching[levl[tx][ty].mem_trap - 1
						     + MAXDCHARS])
					goto foundc;
				    if (matching[levl[tx][ty].mem_bg])
					goto foundc;
#endif
				}
				/* last, the terrain there if it has been
				   seen (from NetHack 3.6) */
				if (levl[tx][ty].seenv) {
				    k = back_to_glyph(tx, ty);
				    if (glyph_is_cmap(k) &&
					    matching[glyph_to_cmap(k)])
					goto foundc;
				}
				continue;
			    foundc:
				cx = tx,  cy = ty;
				if (msg_given) {
				    clear_nhwindow(WIN_MESSAGE);
				    msg_given = FALSE;
				    described = FALSE;
				}
				goto nxtc;
			    }	/* column */
			}	/* row */
		    }		/* pass */
		    pline("Can't find dungeon feature '%c'.", c);
		    msg_given = TRUE;
		    described = FALSE;
		    goto nxtc;
		} else {
		    pline("Unknown direction: '%s' (%s).",
			  visctrl((char)c),
			  !force ? "aborted" :
			  iflags.num_pad ? "use 2468 or ." : "use hjkl or .");
		    msg_given = TRUE;
		    described = FALSE;
		} /* k => matching */
	    } /* !quitchars */
	    if (force) goto nxtc;
	    pline("Done.");
	    msg_given = FALSE;	/* suppress clear */
	    cx = -1;
	    cy = 0;
	    result = 0;	/* not -1 */
	    break;
	}
    nxtc:	;
#ifdef CLIPPING
	cliparound(cx, cy);
#endif
	curs(WIN_MAP,cx,cy);
	flush_screen(0);
    }
#if defined(MAC) || defined(ANDROID)
    lock_mouse_cursor(FALSE);
#endif
    if (msg_given) clear_nhwindow(WIN_MESSAGE);
    cc->x = cx;
    cc->y = cy;
    for (i = 0; i < NUM_GLOCS; i++)
	if (garr[i])
	    free((genericptr_t)garr[i]);
    getpos_getvalid = (boolean FDECL((*), (int,int))) 0;
    return result;
}

struct monst *
christen_monst(mtmp, name)
struct monst *mtmp;
const char *name;
{
	int lth;
	struct monst *mtmp2;
	char buf[PL_PSIZ];

	/* dogname & catname are PL_PSIZ arrays; object names have same limit */
	lth = *name ? (int)(strlen(name) + 1) : 0;
	if(lth > PL_PSIZ){
		lth = PL_PSIZ;
		name = strncpy(buf, name, PL_PSIZ - 1);
		buf[PL_PSIZ - 1] = '\0';
	}
	if (lth == mtmp->mnamelth) {
		/* don't need to allocate a new monst struct */
		if (lth) Strcpy(NAME(mtmp), name);
		return mtmp;
	}
	mtmp2 = newmonst(mtmp->mxlth + lth);
	*mtmp2 = *mtmp;
	(void) memcpy((genericptr_t)mtmp2->mextra,
		      (genericptr_t)mtmp->mextra, mtmp->mxlth);
	mtmp2->mnamelth = lth;
	if (lth) Strcpy(NAME(mtmp2), name);
	replmon(mtmp,mtmp2);
	return(mtmp2);
}

int
do_mname()
{
	char buf[BUFSZ];
	coord cc;
	register int cx,cy;
	register struct monst *mtmp;
	char qbuf[QBUFSZ];

	if (Hallucination) {
		You("would never recognize it anyway.");
		return 0;
	}
	cc.x = u.ux;
	cc.y = u.uy;
	if (getpos(&cc, FALSE, "the monster you want to name") < 0 ||
			(cx = cc.x) < 0)
		return 0;
	cy = cc.y;

	if (cx == u.ux && cy == u.uy) {
#ifdef STEED
	    if (u.usteed && canspotmon(u.usteed))
		mtmp = u.usteed;
	    else {
#endif
		pline("This %s creature is called %s and cannot be renamed.",
		ACURR(A_CHA) > 14 ?
		(flags.female ? "beautiful" : "handsome") :
		"ugly",
		plname);
		return(0);
#ifdef STEED
	    }
#endif
	} else
	    mtmp = m_at(cx, cy);

	if (!mtmp || (!sensemon(mtmp) &&
			(!(cansee(cx,cy) || see_with_infrared(mtmp)) || mtmp->mundetected
			|| mtmp->m_ap_type == M_AP_FURNITURE
			|| mtmp->m_ap_type == M_AP_OBJECT
			|| (mtmp->minvis && !See_invisible)))) {
		pline("I see no monster there.");
		return(0);
	}
	/* special case similar to the one in lookat() */
	(void) distant_monnam(mtmp, ARTICLE_THE, buf);
	Sprintf(qbuf, "What do you want to call %s?",
		safe_qbuf("", sizeof "What do you want to call ?",
			  buf, "it", "it"));
	getlin(qbuf,buf);
	if(!*buf || *buf == '\033') return(0);
	/* strip leading and trailing spaces; unnames monster if all spaces */
	(void)mungspaces(buf);
	/* names are limited to PL_PSIZ (christen_monst() would chop it) */
	if (strlen(buf) >= PL_PSIZ) buf[PL_PSIZ - 1] = '\0';

	if (mtmp->data->geno & G_UNIQ)
	    pline("%s doesn't like being called names!", Monnam(mtmp));
	else
	    (void) christen_monst(mtmp, buf);
	return(0);
}

/*
 * This routine changes the address of obj. Be careful not to call it
 * when there might be pointers around in unknown places. For now: only
 * when obj is in the inventory.
 */
void
do_oname(obj)
register struct obj *obj;
{
	char buf[BUFSZ], qbuf[QBUFSZ];
	const char *aname;
	short objtyp;

	Sprintf(qbuf, "What do you want to name %s ",
		is_plural(obj) ? "these" : "this");
	Sprintf(eos(qbuf), "%s?", safe_qbuf(qbuf, sizeof "?", xname(obj),
		is_plural(obj) ? "things" : "thing",
		is_plural(obj) ? "things" : "thing"));
	getlin(qbuf, buf);
	if(!*buf || *buf == '\033')	return;
	/* strip leading and trailing spaces; unnames item if all spaces */
	(void)mungspaces(buf);
	/* names are limited to PL_PSIZ (oname() would chop it) */
	if (strlen(buf) >= PL_PSIZ) buf[PL_PSIZ - 1] = '\0';

	if (obj->oartifact) {
		pline_The("artifact seems to resist the attempt.");
		return;
	}

	/* relax restrictions over proper capitalization for artifacts;
	   check the canonical spelling whatever the object's type, so that
	   a name which only matches after capitalization can't be used to
	   tell whether an unidentified object is the artifact's type */
	if ((aname = artifact_name(buf, &objtyp)) != 0 &&
		(restrict_name(obj, aname) || exist_artifact(obj->otyp, aname))) {
		int n;
		register char c1, c2;

		/* substitute canonical spelling before slippage */
		Strcpy(buf, aname);
		n = rn2((int)strlen(buf));
		c1 = lowc(buf[n]);
		do c2 = 'a' + rn2('z'-'a'); while (c1 == c2);
		buf[n] = (buf[n] == c1) ? c2 : highc(c2);  /* keep same case */
		pline("While engraving your %s slips.", body_part(HAND));
		display_nhwindow(WIN_MESSAGE, FALSE);
		You("engrave: \"%s\".",buf);
	} else if (aname && objtyp == obj->otyp) {
		/* naming will turn obj into this artifact, so use the
		   canonical capitalization (Sting or Orcrist) */
		Strcpy(buf, aname);
	}
	obj = oname(obj, buf);
	if (obj && obj->oartifact)	/* #chronicle */
	    livelog_printf(LL_ARTIFACT, "chose %s to be named \"%s\"",
			   an(simple_typename(obj->otyp)),
			   artiname(obj->oartifact));
}

/*
 * Allocate a new and possibly larger storage space for an obj.
 */
struct obj *
realloc_obj(obj, oextra_size, oextra_src, oname_size, name)
struct obj *obj;
int oextra_size;		/* storage to allocate for oextra            */
genericptr_t oextra_src;
int oname_size;			/* size of name string + 1 (null terminator) */
const char *name;
{
	struct obj *otmp;

	otmp = newobj(oextra_size + oname_size);
	*otmp = *obj;	/* the cobj pointer is copied to otmp */
	if (oextra_size) {
	    if (oextra_src)
		(void) memcpy((genericptr_t)otmp->oextra, oextra_src,
							oextra_size);
	} else {
	    otmp->oattached = OATTACHED_NOTHING;
	}
	otmp->oxlth = oextra_size;

	otmp->onamelth = oname_size;
	otmp->timed = 0;	/* not timed, yet */
	otmp->lamplit = 0;	/* ditto */
	/* __GNUC__ note:  if the assignment of otmp->onamelth immediately
	   precedes this `if' statement, a gcc bug will miscompile the
	   test on vax (`insv' instruction used to store bitfield does
	   not set condition codes, but optimizer behaves as if it did).
	   gcc-2.7.2.1 finally fixed this.... */
	if (oname_size) {
	    if (name)
		Strcpy(ONAME(otmp), name);
	}

	if (obj->owornmask) {
		boolean save_twoweap = u.twoweap;
		/* unwearing the old instance will clear dual-wield mode
		   if this object is either of the two weapons */
		setworn((struct obj *)0, obj->owornmask);
		setworn(otmp, otmp->owornmask);
		u.twoweap = save_twoweap;
	}

	/* replace obj with otmp */
	replace_object(obj, otmp);

	/* fix ocontainer pointers */
	if (Has_contents(obj)) {
		struct obj *inside;

		for(inside = obj->cobj; inside; inside = inside->nobj)
			inside->ocontainer = otmp;
	}

	/* move timers and light sources from obj to otmp */
	if (obj->timed) obj_move_timers(obj, otmp);
	if (obj->lamplit) obj_move_light_source(obj, otmp);

	/* objects possibly being manipulated by multi-turn occupations
	   which have been interrupted but might be subsequently resumed */
	if (obj->oclass == FOOD_CLASS)
	    food_substitution(obj, otmp);	/* eat food or open tin */
	else if (obj->oclass == SPBOOK_CLASS)
	    book_substitution(obj, otmp);	/* read spellbook */

	/* obfree(obj, otmp);	now unnecessary: no pointers on bill */
	dealloc_obj(obj);	/* let us hope nobody else saved a pointer */
	return otmp;
}

struct obj *
oname(obj, name)
struct obj *obj;
const char *name;
{
	int lth;
	char buf[PL_PSIZ];

	lth = *name ? (int)(strlen(name) + 1) : 0;
	if (lth > PL_PSIZ) {
		lth = PL_PSIZ;
		name = strncpy(buf, name, PL_PSIZ - 1);
		buf[PL_PSIZ - 1] = '\0';
	}
	/* If named artifact exists in the game, do not create another.
	 * Also trying to create an artifact shouldn't de-artifact
	 * it (e.g. Excalibur from prayer). In this case the object
	 * will retain its current name. */
	if (obj->oartifact || (lth && exist_artifact(obj->otyp, name)))
		return obj;

	if (lth == obj->onamelth) {
		/* no need to replace entire object */
		if (lth) Strcpy(ONAME(obj), name);
	} else {
		obj = realloc_obj(obj, obj->oxlth,
			      (genericptr_t)obj->oextra, lth, name);
	}
	if (lth) artifact_exists(obj, name, TRUE);
	if (obj->oartifact) {
	    /* can't dual-wield with artifact as secondary weapon */
	    if (obj == uswapwep) untwoweapon();
	    /* activate warning if you've just named your weapon "Sting" */
	    if (obj == uwep) set_artifact_intrinsic(obj, TRUE, W_WEP);
	}
	if (carried(obj)) update_inventory();
	return obj;
}

static NEARDATA const char callable[] = {
	SCROLL_CLASS, POTION_CLASS, WAND_CLASS, RING_CLASS, AMULET_CLASS,
	GEM_CLASS, SPBOOK_CLASS, ARMOR_CLASS, TOOL_CLASS, 0 };

int
ddocall()
{
	register struct obj *obj;
#ifdef REDO
	char	ch;
#endif
	char allowall[2];

	switch(
#ifdef REDO
		ch =
#endif
		ynq("Name an individual object?")) {
	case 'q':
		break;
	case 'y':
#ifdef REDO
		savech(ch);
#endif
		allowall[0] = ALL_CLASSES; allowall[1] = '\0';
		obj = getobj(allowall, "name");
		if(obj) do_oname(obj);
		break;
	default :
#ifdef REDO
		savech(ch);
#endif
		obj = getobj(callable, "call");
		if (obj) {
			/* behave as if examining it in inventory;
			   this might set dknown if it was picked up
			   while blind and the hero can now see */
			(void) xname(obj);

			if (!obj->dknown) {
				You("would never recognize another one.");
				return 0;
			}
			docall(obj);
		}
		break;
	}
	return 0;
}

void
docall(obj)
register struct obj *obj;
{
	char buf[BUFSZ], qbuf[QBUFSZ];
	struct obj otemp;
	register char **str1;

	if (!obj->dknown) return; /* probably blind */
	otemp = *obj;
	otemp.quan = 1L;
	otemp.onamelth = 0;
	otemp.oxlth = 0;
	if (objects[otemp.otyp].oc_class == POTION_CLASS && otemp.fromsink)
	    /* kludge, meaning it's sink water */
	    Sprintf(qbuf,"Call a stream of %s fluid:",
		    OBJ_DESCR(objects[otemp.otyp]));
	else
	    Sprintf(qbuf, "Call %s:", safe_qbuf("", sizeof "Call :",
				an(xname(&otemp)), "it", "it"));
	getlin(qbuf, buf);
	if(!*buf || *buf == '\033')
		return;
	/* type names are limited to PL_PSIZ, like other player-given names;
	   a longer one could overflow the buffers used by xname() */
	if (strlen(buf) >= PL_PSIZ) buf[PL_PSIZ - 1] = '\0';

	/* clear old name */
	str1 = &(objects[obj->otyp].oc_uname);
	if(*str1) free((genericptr_t)*str1);

	/* strip leading and trailing spaces; uncalls item if all spaces */
	(void)mungspaces(buf);
	if (!*buf) {
	    if (*str1) {	/* had name, so possibly remove from disco[] */
		/* strip name first, for the update_inventory() call
		   from undiscover_object() */
		*str1 = (char *)0;
		undiscover_object(obj->otyp);
	    }
	} else {
	    *str1 = strcpy((char *) alloc((unsigned)strlen(buf)+1), buf);
	    discover_object(obj->otyp, FALSE, TRUE); /* possibly add to disco[] */
	}
}

#endif /*OVLB*/
#ifdef OVL0

static const char * const ghostnames[] = {
	/* these names should have length < PL_NSIZ */
	/* Capitalize the names for aesthetics -dgk */
	"Adri", "Andries", "Andreas", "Bert", "David", "Dirk", "Emile",
	"Frans", "Fred", "Greg", "Hether", "Jay", "John", "Jon", "Karnov",
	"Kay", "Kenny", "Kevin", "Maud", "Michiel", "Mike", "Peter", "Robert",
	"Ron", "Tom", "Wilmar", "Nick Danger", "Phoenix", "Jiro", "Mizue",
	"Stephan", "Lance Braccus", "Shadowhawk"
};

/* ghost names formerly set by x_monnam(), now by makemon() instead */
const char *
rndghostname()
{
    return rn2(7) ? ghostnames[rn2(SIZE(ghostnames))] : (const char *)plname;
}

/* Monster naming functions:
 * x_monnam is the generic monster-naming function.
 *		  seen	      unseen	   detected		  named
 * mon_nam:	the newt	it	the invisible orc	Fido
 * noit_mon_nam:the newt (as if detected) the invisible orc	Fido
 * l_monnam:	newt		it	invisible orc		dog called fido
 * Monnam:	The newt	It	The invisible orc	Fido
 * noit_Monnam: The newt (as if detected) The invisible orc	Fido
 * Adjmonnam:	The poor newt	It	The poor invisible orc	The poor Fido
 * Amonnam:	A newt		It	An invisible orc	Fido
 * a_monnam:	a newt		it	an invisible orc	Fido
 * m_monnam:	newt		xan	orc			Fido
 * y_monnam:	your newt     your xan	your invisible orc	Fido
 */

/* Bug: if the monster is a priest or shopkeeper, not every one of these
 * options works, since those are special cases.
 */
char *
x_monnam(mtmp, article, adjective, suppress, called)
register struct monst *mtmp;
int article;
/* ARTICLE_NONE, ARTICLE_THE, ARTICLE_A: obvious
 * ARTICLE_YOUR: "your" on pets, "the" on everything else
 *
 * If the monster would be referred to as "it" or if the monster has a name
 * _and_ there is no adjective, "invisible", "saddled", etc., override this
 * and always use no article.
 */
const char *adjective;
int suppress;
/* SUPPRESS_IT, SUPPRESS_INVISIBLE, SUPPRESS_HALLUCINATION, SUPPRESS_SADDLE.
 * EXACT_NAME: combination of all the above
 */
boolean called;
{
#ifdef LINT	/* static char buf[BUFSZ]; */
	char buf[BUFSZ];
#else
	static char buf[BUFSZ];
#endif
	struct permonst *mdat = mtmp->data;
	boolean do_hallu, do_invis, do_it, do_saddle;
	boolean name_at_start, has_adjectives;
	char *bp;

	if (program_state.gameover)
	    suppress |= SUPPRESS_HALLUCINATION;
	if (article == ARTICLE_YOUR && !mtmp->mtame)
	    article = ARTICLE_THE;

	do_hallu = Hallucination && !(suppress & SUPPRESS_HALLUCINATION);
	do_invis = mtmp->minvis && !(suppress & SUPPRESS_INVISIBLE);
	do_it = !canspotmon(mtmp) && 
	    article != ARTICLE_YOUR &&
	    !program_state.gameover &&
#ifdef STEED
	    mtmp != u.usteed &&
#endif
	    !(u.uswallow && mtmp == u.ustuck) &&
	    !(suppress & SUPPRESS_IT);
	do_saddle = !(suppress & SUPPRESS_SADDLE);

	buf[0] = 0;

	/* unseen monsters, etc.  Use "it" */
	if (do_it) {
	    Strcpy(buf, "it");
	    return buf;
	}

	/* priests and minions: don't even use this function */
	if (mtmp->ispriest || mtmp->isminion) {
	    char priestnambuf[BUFSZ];
	    char *name;
	    long save_prop = EHalluc_resistance;
	    unsigned save_invis = mtmp->minvis;

	    /* when true name is wanted, explicitly block Hallucination */
	    if (!do_hallu) EHalluc_resistance = 1L;
	    if (!do_invis) mtmp->minvis = 0;
	    name = priestname(mtmp, priestnambuf);
	    EHalluc_resistance = save_prop;
	    mtmp->minvis = save_invis;
	    if (article == ARTICLE_NONE && !strncmp(name, "the ", 4))
		name += 4;
	    return strcpy(buf, name);
	}

	/* Shopkeepers: use shopkeeper name.  For normal shopkeepers, just
	 * "Asidonhopo"; for unusual ones, "Asidonhopo the invisible
	 * shopkeeper" or "Asidonhopo the blue dragon".  If hallucinating,
	 * none of this applies.
	 */
	if (mtmp->isshk && !do_hallu) {
	    if (adjective && article == ARTICLE_THE) {
		/* pathological case: "the angry Asidonhopo the blue dragon"
		   sounds silly */
		Strcpy(buf, "the ");
		Strcat(strcat(buf, adjective), " ");
		Strcat(buf, shkname(mtmp));
		return buf;
	    }
	    Strcat(buf, shkname(mtmp));
	    if (mdat == &mons[PM_SHOPKEEPER] && !do_invis)
		return buf;
	    Strcat(buf, " the ");
	    if (do_invis)
		Strcat(buf, "invisible ");
	    Strcat(buf, mdat->mname);
	    return buf;
	}

	/* Put the adjectives in the buffer */
	if (adjective)
	    Strcat(strcat(buf, adjective), " ");
	if (do_invis)
	    Strcat(buf, "invisible ");
#ifdef STEED
	if (do_saddle && (mtmp->misc_worn_check & W_SADDLE) &&
	    !Blind && !Hallucination)
	    Strcat(buf, "saddled ");
#endif
	if (buf[0] != 0)
	    has_adjectives = TRUE;
	else
	    has_adjectives = FALSE;

	/* Put the actual monster name or type into the buffer now */
	/* Be sure to remember whether the buffer starts with a name */
	if (do_hallu) {
	    Strcat(buf, rndmonnam());
	    name_at_start = FALSE;
	} else if (mtmp->mnamelth) {
	    char *name = NAME(mtmp);

	    if (mdat == &mons[PM_GHOST]) {
		Sprintf(eos(buf), "%s ghost", s_suffix(name));
		name_at_start = TRUE;
	    } else if (called) {
		Sprintf(eos(buf), "%s called %s", mdat->mname, name);
		name_at_start = (boolean)type_is_pname(mdat);
	    } else if (is_mplayer(mdat) && (bp = strstri(name, " the ")) != 0) {
		/* <name> the <adjective> <invisible> <saddled> <rank> */
		char pbuf[BUFSZ];

		Strcpy(pbuf, name);
		pbuf[bp - name + 5] = '\0'; /* adjectives right after " the " */
		if (has_adjectives)
		    Strcat(pbuf, buf);
		Strcat(pbuf, bp + 5);	/* append the rest of the name */
		Strcpy(buf, pbuf);
		article = ARTICLE_NONE;
		name_at_start = TRUE;
	    } else {
		Strcat(buf, name);
		name_at_start = TRUE;
	    }
	} else if (is_mplayer(mdat) && !In_endgame(&u.uz)) {
	    char pbuf[BUFSZ];
	    Strcpy(pbuf, rank_of((int)mtmp->m_lev,
				 monsndx(mdat),
				 (boolean)mtmp->female));
	    Strcat(buf, lcase(pbuf));
	    name_at_start = FALSE;
	} else {
	    Strcat(buf, mdat->mname);
	    name_at_start = (boolean)type_is_pname(mdat);
	}

	if (name_at_start && (article == ARTICLE_YOUR || !has_adjectives)) {
	    if (mdat == &mons[PM_WIZARD_OF_YENDOR])
		article = ARTICLE_THE;
	    else
		article = ARTICLE_NONE;
	} else if ((mdat->geno & G_UNIQ) && article == ARTICLE_A) {
	    article = ARTICLE_THE;
	}

	{
	    char buf2[BUFSZ];

	    switch(article) {
		case ARTICLE_YOUR:
		    Strcpy(buf2, "your ");
		    Strcat(buf2, buf);
		    Strcpy(buf, buf2);
		    return buf;
		case ARTICLE_THE:
		    Strcpy(buf2, "the ");
		    Strcat(buf2, buf);
		    Strcpy(buf, buf2);
		    return buf;
		case ARTICLE_A:
		    return(an(buf));
		case ARTICLE_NONE:
		default:
		    return buf;
	    }
	}
}

#endif /* OVL0 */
#ifdef OVLB

char *
l_monnam(mtmp)
register struct monst *mtmp;
{
	return(x_monnam(mtmp, ARTICLE_NONE, (char *)0, 
		mtmp->mnamelth ? SUPPRESS_SADDLE : 0, TRUE));
}

#endif /* OVLB */
#ifdef OVL0

char *
mon_nam(mtmp)
register struct monst *mtmp;
{
	return(x_monnam(mtmp, ARTICLE_THE, (char *)0,
		mtmp->mnamelth ? SUPPRESS_SADDLE : 0, FALSE));
}

/* print the name as if mon_nam() was called, but assume that the player
 * can always see the monster--used for probing and for monsters aggravating
 * the player with a cursed potion of invisibility
 */
char *
noit_mon_nam(mtmp)
register struct monst *mtmp;
{
	return(x_monnam(mtmp, ARTICLE_THE, (char *)0,
		mtmp->mnamelth ? (SUPPRESS_SADDLE|SUPPRESS_IT) :
		    SUPPRESS_IT, FALSE));
}

char *
Monnam(mtmp)
register struct monst *mtmp;
{
	register char *bp = mon_nam(mtmp);

	*bp = highc(*bp);
	return(bp);
}

char *
noit_Monnam(mtmp)
register struct monst *mtmp;
{
	register char *bp = noit_mon_nam(mtmp);

	*bp = highc(*bp);
	return(bp);
}

/* monster's own name */
char *
m_monnam(mtmp)
struct monst *mtmp;
{
	return x_monnam(mtmp, ARTICLE_NONE, (char *)0, EXACT_NAME, FALSE);
}

/* pet name: "your little dog" */
char *
y_monnam(mtmp)
struct monst *mtmp;
{
	int prefix, suppression_flag;

	prefix = mtmp->mtame ? ARTICLE_YOUR : ARTICLE_THE;
	suppression_flag = (mtmp->mnamelth
#ifdef STEED
			    /* "saddled" is redundant when mounted */
			    || mtmp == u.usteed
#endif
			    ) ? SUPPRESS_SADDLE : 0;

	return x_monnam(mtmp, prefix, (char *)0, suppression_flag, FALSE);
}

#endif /* OVL0 */
#ifdef OVLB

char *
Adjmonnam(mtmp, adj)
register struct monst *mtmp;
register const char *adj;
{
	register char *bp = x_monnam(mtmp, ARTICLE_THE, adj,
		mtmp->mnamelth ? SUPPRESS_SADDLE : 0, FALSE);

	*bp = highc(*bp);
	return(bp);
}

char *
a_monnam(mtmp)
register struct monst *mtmp;
{
	return x_monnam(mtmp, ARTICLE_A, (char *)0,
		mtmp->mnamelth ? SUPPRESS_SADDLE : 0, FALSE);
}

char *
Amonnam(mtmp)
register struct monst *mtmp;
{
	register char *bp = a_monnam(mtmp);

	*bp = highc(*bp);
	return(bp);
}

/* used for monster ID by the '/', ';', and 'C' commands to block remote
   identification of the endgame altars via their attending priests */
char *
distant_monnam(mon, article, outbuf)
struct monst *mon;
int article;	/* only ARTICLE_NONE and ARTICLE_THE are handled here */
char *outbuf;
{
    /* high priest(ess)'s identity is concealed on the Astral Plane,
       unless you're adjacent (overridden for hallucination which does
       its own obfuscation) */
    if (mon->data == &mons[PM_HIGH_PRIEST] && !Hallucination &&
	    Is_astralevel(&u.uz) && distu(mon->mx, mon->my) > 2) {
	Strcpy(outbuf, article == ARTICLE_THE ? "the " : "");
	Strcat(outbuf, mon->female ? "high priestess" : "high priest");
    } else {
	Strcpy(outbuf, x_monnam(mon, article, (char *)0, 0, TRUE));
    }
    return outbuf;
}

static const char * const bogusmons[] = {
	"jumbo shrimp", "giant pigmy", "gnu", "killer penguin",
	"giant cockroach", "giant slug", "pterodactyl",
	"tyrannosaurus rex", "rot grub", "bookworm", "master lichen",
	"hologram", "jester", "attorney", "sleazoid",
	"killer tomato", "amazon", "robot", "battlemech",
	"rhinovirus", "harpy", "lion-dog", "rat-ant", "Y2K bug",
						/* misc. */
	"grue", "Christmas-tree monster", "luck sucker", "paskald",
	"brogmoid", "dornbeast",		/* Quendor (Zork, &c.) */
	"Ancient Multi-Hued Dragon", "Evil Iggy",
						/* Moria */
	"emu", "kestrel", "xeroc", "venus flytrap",
						/* Rogue */
	"creeping coins",			/* Wizardry */
	"siren",                                /* Greek legend */
	"killer bunny",				/* Monty Python */
	"rodent of unusual size",		/* The Princess Bride */
	"Smokey the bear",	/* "Only you can prevent forest fires!" */
	"Luggage",				/* Discworld */
	"Ent",					/* Lord of the Rings */
	"tangle tree", "wiggle",                /* Xanth */
	"white rabbit", "snark",		/* Lewis Carroll */
	"pushmi-pullyu",			/* Dr. Doolittle */
	"smurf",				/* The Smurfs */
	"tribble", "Klingon", "Borg",		/* Star Trek */
	"Ewok",					/* Star Wars */
	"Totoro",				/* Tonari no Totoro */
	"ohmu",					/* Nausicaa */
	"youma",				/* Sailor Moon */
	"nyaasu",				/* Pokemon (Meowth) */
	"Godzilla", "King Kong",		/* monster movies */
	"earthquake beast",			/* old L of SH */
	"Invid",				/* Robotech */
	"Terminator",				/* The Terminator */
	"boomer",				/* Bubblegum Crisis */
	"Dalek",				/* Dr. Who ("Exterminate!") */
	"microscopic space fleet", "Ravenous Bugblatter Beast of Traal",
						/* HGttG */
	"teenage mutant ninja turtle",		/* TMNT */
	"samurai rabbit",			/* Usagi Yojimbo */
	"aardvark",				/* Cerebus */
	"Audrey II",				/* Little Shop of Horrors */
	"witch doctor", "one-eyed one-horned flying purple people eater",
						/* 50's rock 'n' roll */
	"Barney the dinosaur",			/* saccharine kiddy TV */
	"Azog the Orc King", "Morgoth",		/* Angband */

	/*[Tom] new wacky names */
	"commando", "green beret", "sherman tank", 
						/* Military */
	"Jedi knight", "tie fighter", "protocol droid", "R2 unit", "Emperor",
						/* Star Wars */
	"Vorlon",				/* Babylon 5 */
	"keg","Diet Pepsi",
						/* drinks */
	"questing beast",		/* King Arthur */
	"Predator",				/* Movie */
	"green light", "automobile", "invisible Wizard of Yendor",
	"piece of yellowish-brown glass", "wand of nothing",
	"ocean","ballpoint pen","paper cut",	
						/* misc */
	"Rune", "Gurk", "Yuval",		/* people I know */
	"mother-in-law"				/* common pest */
};

/* Return a random monster name, for hallucination.
 * KNOWN BUG: May be a proper name (Godzilla, Barney), may not
 * (the Terminator, a Dalek).  There's no elegant way to deal
 * with this without radically modifying the calling functions.
 */
const char *
rndmonnam()
{
	int name;

	do {
	    name = rn1(SPECIAL_PM + SIZE(bogusmons) - LOW_PM, LOW_PM);
	} while (name < SPECIAL_PM &&
	    (type_is_pname(&mons[name]) || (mons[name].geno & G_NOGEN)));

	if (name >= SPECIAL_PM) return bogusmons[name - SPECIAL_PM];
	return mons[name].mname;
}

#ifdef REINCARNATION
const char *
roguename() /* Name of a Rogue player */
{
	char *i, *opts;

	if ((opts = nh_getenv("ROGUEOPTS")) != 0) {
		for (i = opts; *i; i++)
			if (!strncmp("name=",i,5)) {
				char *j;
				if ((j = index(i+5,',')) != 0)
					*j = (char)0;
				return i+5;
			}
	}
	return rn2(3) ? (rn2(2) ? "Michael Toy" : "Kenneth Arnold")
		: "Glenn Wichman";
}
#endif /* REINCARNATION */
#endif /* OVLB */

#ifdef OVL2

static NEARDATA const char * const hcolors[] = {
	"ultraviolet", "infrared", "bluish-orange",
	"reddish-green", "dark white", "light black", "sky blue-pink",
	"salty", "sweet", "sour", "bitter",
	"striped", "spiral", "swirly", "plaid", "checkered", "argyle",
	"paisley", "blotchy", "guernsey-spotted", "polka-dotted",
	"square", "round", "triangular",
	"cabernet", "sangria", "fuchsia", "wisteria",
	"lemon-lime", "strawberry-banana", "peppermint",
	"romantic", "incandescent"
};

const char *
hcolor(colorpref)
const char *colorpref;
{
	return (Hallucination || !colorpref) ?
		hcolors[rn2(SIZE(hcolors))] : colorpref;
}

/* return a random real color unless hallucinating */
const char *
rndcolor()
{
	int k = rn2(CLR_MAX);
	return Hallucination ? hcolor((char *)0) : (k == NO_COLOR) ?
		"colorless" : c_obj_colors[k];
}

/* Aliases for road-runner nemesis
 */
static const char * const coynames[] = {
	"Carnivorous Vulgaris","Road-Runnerus Digestus",
	"Eatibus Anythingus"  ,"Famishus-Famishus",
	"Eatibus Almost Anythingus","Eatius Birdius",
	"Famishius Fantasticus","Eternalii Famishiis",
	"Famishus Vulgarus","Famishius Vulgaris Ingeniusi",
	"Eatius-Slobbius","Hardheadipus Oedipus",
	"Carnivorous Slobbius","Hard-Headipus Ravenus",
	"Evereadii Eatibus","Apetitius Giganticus",
	"Hungrii Flea-Bagius","Overconfidentii Vulgaris",
	"Caninus Nervous Rex","Grotesques Appetitus",
	"Nemesis Riduclii","Canis latrans"
};
	
char *
coyotename(mtmp, buf)
struct monst *mtmp;
char *buf;
{
    if (mtmp && buf) {
	Sprintf(buf, "%s - %s",
	    x_monnam(mtmp, ARTICLE_NONE, (char *)0, 0, TRUE),
	    mtmp->mcan ? coynames[SIZE(coynames)-1] : coynames[rn2(SIZE(coynames)-1)]);
    }
    return buf;
}
#endif /* OVL2 */

/*do_name.c*/
